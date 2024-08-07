#include "xattr.h"
#include "fingerprint.h"

#ifdef USE_XATTR
#include <sys/types.h>
#include <sys/xattr.h>
#endif

#ifdef USE_EXTATTR
#include <sys/types.h>
#include <sys/extattr.h>
#endif



char **XAttrList=NULL;



void SetupXAttrList(const char *Arg)
{
    char *Token=NULL;
    const char *ptr;
//count starts at 1 because even if the first string is an empty string, we need to alloc space
//for it.
    int count=1;



    ptr=strchr(Arg, ',');
    while (ptr)
    {
        count++;
        ptr++;
        ptr=strchr(ptr, ',');
    }

//count+1 to allow room for terminating NULL
    XAttrList=calloc(count+1,sizeof(char *));

    count=0;
    ptr=GetToken(Arg, ",",&Token,0);
    while (ptr)
    {
        XAttrList[count]=CopyStr(XAttrList[count],Token);
        ptr=GetToken(ptr, ",",&Token,0);
        count++;
    }

    Destroy(Token);
}


void HashRatSetXAttr(HashratCtx *Ctx, const char *Path, struct stat *Stat, const char *HashType, const char *Hash)
{
    char *Tempstr=NULL, *Attr=NULL;
    char **ptr;
    int result;

    Tempstr=FormatStr(Tempstr,"%llu:%llu:%s",(unsigned long long) time(NULL), (unsigned long long) Stat->st_size, Hash);


#ifdef USE_XATTR
    if ((Ctx->Flags & CTX_XATTR_ROOT) && (getuid()==0)) Attr=MCopyStr(Attr,"trusted.","hashrat:",HashType,NULL);
    else Attr=MCopyStr(Attr,"user.","hashrat:",HashType,NULL);

    result=setxattr(Path, Attr, Tempstr, StrLen(Tempstr)+1, 0);
    if (result != 0) fprintf(stderr,"ERROR: Failed to store hash in extended attributes for '%s'  error was '%s'\n", Path, strerror(errno));
#elif defined USE_EXTATTR
    Attr=MCopyStr(Attr,"hashrat:",HashType,NULL);
    if ((Ctx->Flags & CTX_XATTR_ROOT) && (getuid()==0)) result=extattr_set_file(Path, EXTATTR_NAMESPACE_SYSTEM, Attr, Tempstr, StrLen(Tempstr)+1);
    else result=extattr_set_file(Path, EXTATTR_NAMESPACE_USER, Attr, Tempstr, StrLen(Tempstr)+1);
    if (result < 1) fprintf(stderr,"ERROR: Failed to store hash in extended attributes for '%s'  error was '%s'\n", Path, strerror(errno));
#else
    fprintf(stderr,"XATTR Support not compiled in.\n");
    exit(1);
#endif



    if (XAttrList)
    {
        for(ptr=XAttrList; *ptr !=NULL; ptr++)
        {
#ifdef USE_XATTR
            setxattr(Path, *ptr, Hash, StrLen(Hash), 0);
#elif defined USE_EXTATTR
            extattr_set_file(Path, EXTATTR_NAMESPACE_USER, Attr, Tempstr, StrLen(Tempstr)+1);
#endif
        }
    }

    Destroy(Tempstr);
    Destroy(Attr);

}


int XAttrGetHash(HashratCtx *Ctx, const char *XattrType, const char *HashType, const char *Path, struct stat *FStat, char **Hash)
{
    int result=FALSE;
    char *Tempstr=NULL;
    const char *ptr;
    int len;

    *Hash=SetStrLen(*Hash,255);
#ifdef USE_XATTR
    Tempstr=MCopyStr(Tempstr,XattrType, ".hashrat:",HashType,NULL);
    len=getxattr(Path, Tempstr, *Hash, 255);
#elif defined USE_EXTATTR
    Tempstr=MCopyStr(Tempstr,"hashrat:",HashType,NULL);
    if ((strcmp(XattrType,"trusted")==0) || (strcmp(XattrType,"system")==0)) len=extattr_get_file(Path, EXTATTR_NAMESPACE_SYSTEM, Tempstr, *Hash, 255);
    else len=extattr_get_file(Path, EXTATTR_NAMESPACE_USER, Tempstr, *Hash, 255);
#else
    fprintf(stderr,"XATTR Support not compiled in.\n");
#endif


    if (len > 0)
    {
        (*Hash)[len]='\0';

        ptr=*Hash;
        ptr=GetToken(ptr, ":", &Tempstr, 0);
        if (FStat) FStat->st_mtime=(time_t) strtoll(Tempstr,NULL,10);
        ptr=GetToken(ptr, ":", &Tempstr, 0);
        if (FStat) FStat->st_size=(off_t) strtoll(Tempstr,NULL,10);
        len=StrLen(ptr);
        memmove(*Hash,ptr,len+1);
        result=TRUE;
    }

    Destroy(Tempstr);

    return(result);
}



TFingerprint *XAttrLoadHash(HashratCtx *Ctx, const char *Path)
{
    char *XattrTypes[]= {"trusted","user",NULL};
    TFingerprint *FP=NULL;
    char *Tempstr=NULL;
    int i, j, found=FALSE;


    FP=(TFingerprint *) calloc(1, sizeof(TFingerprint));
    FP->Path=CopyStr(FP->Path, Path);
    FP->Hash=SetStrLen(FP->Hash,1024);
    for (j=0; XattrTypes[j]; j++)
    {
        if (StrValid(Ctx->HashType))
        {
            found=XAttrGetHash(Ctx, XattrTypes[j], Ctx->HashType, Path, &FP->FStat, &FP->Hash);
            FP->HashType=CopyStr(FP->HashType,Ctx->HashType);
        }
        else for (i=0; HashratHashTypes[i] != NULL; i++)
            {
                found=XAttrGetHash(Ctx, XattrTypes[j], HashratHashTypes[i], Path, &FP->FStat, &FP->Hash);
                if (found > 0)
                {
                    FP->HashType=CopyStr(FP->HashType,HashratHashTypes[i]);
                    break;
                }
            }
        if (found) break;
    }

    Destroy(Tempstr);

    if (! found)
    {
        TFingerprintDestroy(FP);
        return(NULL);
    }

    return(FP);
}


