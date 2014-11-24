#include "xattr.h"

#ifdef USE_XATTR
 #include <sys/types.h>
 #include <sys/xattr.h>
#endif



char **XAttrList=NULL;



void SetupXAttrList(char *Arg)
{
char *Token=NULL, *ptr;
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

DestroyString(Token);
}


void HashRatSetXAttr(STREAM *Out, char *Path, struct stat *Stat, char *HashType, char *Hash)
{
char *Tempstr=NULL, *Attr=NULL;
char **ptr;
int result;

#ifdef USE_XATTR
	if ((Flags & FLAG_XATTR_ROOT) && (getuid()==0)) Attr=MCopyStr(Attr,"trusted.","hashrat:",HashType,NULL);
	else Attr=MCopyStr(Attr,"user.","hashrat:",HashType,NULL);
	Tempstr=FormatStr(Tempstr,"%lu:%llu:%s",(unsigned long) time(NULL),(unsigned long long) Stat->st_size,Hash);
	result=setxattr(Path, Attr, Tempstr, StrLen(Tempstr), 0);

	if (XAttrList)
	{
		for(ptr=XAttrList; *ptr !=NULL; ptr++)
		{
		setxattr(Path, *ptr, Hash, StrLen(Hash), 0);
		}
	}
#else
printf("XATTR Support not compiled in.\n");
exit(1);

#endif

DestroyString(Tempstr);
DestroyString(Attr);
}


int XAttrGetHash(HashratCtx *Ctx, char *XattrType, char *HashType, char *Path, struct stat *FStat, char **Hash)
{
char *Tempstr=NULL, *ptr;
int result=FALSE, len;

#ifdef USE_XATTR

Tempstr=MCopyStr(Tempstr,XattrType, ".hashrat:",HashType,NULL);
len=getxattr(Path, Tempstr, *Hash, 1024); 
if (len > 0)
{
	ptr=*Hash;
	FStat->st_mtime=(time_t) strtol(ptr,&ptr,10);
	if (*ptr==':') ptr++;
	FStat->st_size=(off_t) strtol(ptr,&ptr,10);
	if (*ptr==':') ptr++;
	memmove(*Hash,ptr,StrLen(ptr)+1);
	result=TRUE;
}

#endif

DestroyString(Tempstr);
return(result);
}



TFingerprint *XAttrLoadHash(HashratCtx *Ctx, char *Path)
{
char *HashTypes[]={"md5","sha","sha1","sha256","sha512","whirl","whirlpool",NULL};
char *XattrTypes[]={"trusted","user",NULL};
TFingerprint *FP=NULL;
char *Tempstr=NULL;
int i, j, found=FALSE;


FP=(TFingerprint *) calloc(1, sizeof(TFingerprint));
FP->Path=CopyStr(FP->Path, Path);
FP->Hash=SetStrLen(FP->Hash,1024);
for (j=0; XattrTypes[j]; j++)
{
	for (i=0; HashTypes[i] != NULL; i++)
	{
		found=XAttrGetHash(Ctx, XattrTypes[j], HashTypes[i], Path, &FP->FStat, &FP->Hash);
		if (found > 0)
		{
			FP->HashType=CopyStr(FP->HashType,HashTypes[i]);
			break;
		}
	}
}

DestroyString(Tempstr);

if (! found)
{
	FingerprintDestroy(FP);
	return(NULL);
}

return(FP);
}


