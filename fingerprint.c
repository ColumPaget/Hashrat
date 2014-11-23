#include "fingerprint.h"

TFingerprint *FingerprintRead(STREAM *S)
{
char *Tempstr=NULL, *Name=NULL, *Value=NULL, *ptr;
TFingerprint *FP;

FP=(TFingerprint *) calloc(1,sizeof(TFingerprint));
FP->Path=CopyStr(FP->Path,"");
FP->Hash=CopyStr(FP->Hash,"");
FP->Flags=0;
memset(&FP->FStat,0,sizeof(struct stat));

Tempstr=STREAMReadLine(Tempstr,S);
if (! Tempstr) return(NULL);

StripTrailingWhitespace(Tempstr);
if (strncmp(Tempstr,"hash=",5) ==0)
{
//Native format

	ptr=GetNameValuePair(Tempstr," ","=",&Name,&Value);
	while (ptr)
	{
		if (StrLen(Name))
		{
		if (strcmp(Name,"path")==0) FP->Path=CopyStr(FP->Path,Value);
		if (strcmp(Name,"size")==0) 
		{
			FP->Flags |= FP_HASSTAT;
			FP->FStat.st_size=(size_t) strtol(Value,NULL,10);
		}
		if (strcmp(Name,"mode")==0) 
		{
			FP->Flags |= FP_HASSTAT;
			FP->FStat.st_mode=strtol(Value,NULL,8);
		}
		if (strcmp(Name,"mtime")==0)
		{
			FP->Flags |= FP_HASSTAT;
			FP->FStat.st_mtime=strtol(Value,NULL,10);
		}
		if (strcmp(Name,"inode")==0) 
		{
			FP->Flags |= FP_HASSTAT;
			FP->FStat.st_ino=(ino_t) strtol(Value,NULL,10);
		}
		if (strcmp(Name,"uid")==0) 
		{
			FP->Flags |= FP_HASSTAT;
			FP->FStat.st_uid=strtol(Value,NULL,10);
		}
		if (strcmp(Name,"gid")==0) 
		{
			FP->Flags |= FP_HASSTAT;
			FP->FStat.st_gid=strtol(Value,NULL,10);
		}
		if (strcmp(Name,"hash")==0)
		{
			 FP->Hash=CopyStr(FP->Hash,GetToken(Value,":",&FP->HashType,0));
		}
		}
		ptr=GetNameValuePair(ptr," ","=",&Name,&Value);
	}

}
else
{
	ptr=GetToken(Tempstr," ",&FP->Hash,0);
	while (isspace(*ptr)) ptr++;
	FP->Path=CopyStr(FP->Path,ptr);
}

DestroyString(Tempstr);
DestroyString(Name);
DestroyString(Value);

return(FP);
}




void FingerprintDestroy(void *p_FP)
{
TFingerprint *FP;

FP=(TFingerprint *) p_FP;
DestroyString(FP->Path);
DestroyString(FP->Hash);
DestroyString(FP->HashType);
free(FP);
}



int FingerprintCompare(const void *v1, const void *v2)
{
const TFingerprint *FP1, *FP2;

FP1=(TFingerprint *) v1;
FP2=(TFingerprint *) v2;

if (! FP1->Hash) return(FALSE);
if (! FP2->Hash) return(TRUE);
if (strcmp(FP1->Hash,FP2->Hash) < 0) return(TRUE);

return(FALSE);
}

