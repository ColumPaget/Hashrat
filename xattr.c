#include "xattr.h"

#define USE_XATTR 1

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


int XAttrGetHash(HashratCtx *Ctx, char *Path, struct stat *FStat, char **Hash)
{
char *Tempstr=NULL, *ptr;
int result=FALSE, len;

#ifdef USE_XATTR
*Hash=SetStrLen(*Hash,1024);

Tempstr=MCopyStr(Tempstr,"user.hashrat:",Ctx->HashType,NULL);
len=getxattr(Path, Tempstr, *Hash, 1024); 
if (len > 0)
{
	ptr=*Hash;
	if (strtol(ptr,&ptr,10) > FStat->st_mtime)
	{
		if (*ptr==':') ptr++;
		if (strtol(ptr,&ptr,10) == FStat->st_size)
		{
			if (*ptr==':') ptr++;
			memmove(*Hash,ptr,StrLen(ptr)+1);
			result=TRUE;
		}
	}
}

if (! result) printf("XATTR LOAD: [%s] [%s]\n", Path, *Hash);

#endif

DestroyString(Tempstr);
return(result);
}



void XAttrLoadHash(TFingerprint *FP)
{
char *HashTypes[]={"md5","sha","sha1","sha256","sha512","whirl","whirlpool",NULL};
char *XattrTypes[]={"trusted","user",NULL};
char *Tempstr=NULL;
int i, j, len;

FP->Hash=SetStrLen(FP->Hash,1024);

for (j=0; XattrTypes[j]; j++)
{
	for (i=0; HashTypes[i] != NULL; i++)
	{
		Tempstr=MCopyStr(Tempstr,XattrTypes[j],".hashrat:",HashTypes[i],NULL);
		len=getxattr(FP->Hash, Tempstr, FP->Hash, 1024); 
		printf("XATTR LOAD: %s %s\n",Tempstr,FP->Hash);
		if (len > 0)
		{
			FP->HashType=CopyStr(FP->HashType,HashTypes[i]);
			break;
		}
	}
}

DestroyString(Tempstr);
}


int CheckHashesFromXAttr(ListNode *Vars,char *HashType)
{
int result;

return(result);
}
