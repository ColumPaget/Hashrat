#include "find.h"
#include "files.h"
#include "memcached.h"
#include "fingerprint.h"
#include <search.h>


void *Tree;



int MatchCompareFunc(const void *i1, const void *i2)
{
TFingerprint *M1, *M2;

M1=(TFingerprint *) i1;
M2=(TFingerprint *) i2;

return(strcmp(M1->Hash, M2->Hash));
}


int MatchAdd(TFingerprint *FP, const char *Path, int Flags)
{
TFingerprint *Item;
int result=FALSE;
char *ptr;

	if (Flags & FLAG_MEMCACHED)
	{
		ptr=FP->Data;
		if (StrLen(ptr)==0) ptr=FP->Path;
		if (StrLen(ptr)==0) ptr=Path;
		if (MemcachedSet(FP->Hash, 0, ptr)) result=TRUE;
	}
	else
	{
		tsearch(FP, &Tree, MatchCompareFunc);
		result=TRUE;
	}

	return(result);
}




TFingerprint *CheckForMatch(HashratCtx *Ctx, const char *Path, struct stat *FStat, const char *HashStr)
{
TFingerprint *Lookup, *Found, *Result=NULL;
void *ptr;


Lookup=TFingerprintCreate(HashStr,"","",Path);
if (Ctx->Action==ACT_FINDMATCHES_MEMCACHED)
{
		Lookup->Data=MemcachedGet(Lookup->Data, Lookup->Hash);
		if (StrLen(Lookup->Data)) Result=TFingerprintCreate(Lookup->Hash, Lookup->HashType, Lookup->Data, "");
}
else
{
		ptr=tfind(Lookup, &Tree, MatchCompareFunc);
		if (ptr)
		{
			Found=*(TFingerprint **) ptr;
			Result=TFingerprintCreate(Found->Hash, Found->HashType, Found->Data, Found->Path);
		}
}

TFingerprintDestroy(Lookup);

return(Result);
}




int LoadFromIOC(const char *XML, int Flags)
{
char *TagType=NULL, *TagData=NULL, *ptr;
char *ID=NULL;
int count=0;
TFingerprint *FP;

ptr=XMLGetTag(XML,NULL,&TagType,&TagData);
while (ptr)
{
	if (strcasecmp(TagType,"short_description")==0) ptr=XMLGetTag(ptr,NULL,&TagType,&ID);
	if (
			(strcasecmp(TagType,"content")==0) &&
			(strcasecmp(TagData,"type=\"md5\"")==0)
		)
	{
		ptr=XMLGetTag(ptr,NULL,&TagType,&TagData);
		FP=TFingerprintCreate(TagData, "md5", ID, "");
		if (MatchAdd(FP, ID, Flags)) count++;
	}
	ptr=XMLGetTag(ptr,NULL,&TagType,&TagData);
}

DestroyString(ID);
DestroyString(TagType);
DestroyString(TagData);

return(count);
}




void *MatchesLoad(int Flags)
{
char *Line=NULL, *Tempstr=NULL, *Type=NULL, *ptr;
TFingerprint *FP;
STREAM *S;
int count=0;


S=STREAMFromFD(0);
STREAMSetTimeout(S,1);
Line=STREAMReadLine(Line,S);
if (! StrLen(Line)) return(NULL);

if (strncasecmp(Line,"<?xml ",6)==0) 
{
	//xml document. Must be an OpenIOC fileq
	while (Line)
	{
	StripTrailingWhitespace(Line);
	Tempstr=CatStr(Tempstr,Line);
	Line=STREAMReadLine(Line,S);
	}
	count=LoadFromIOC(Tempstr,Flags);
}
else
{
	while (Line)
	{
	StripTrailingWhitespace(Line);
	FP=TFingerprintParse(Line);
	if (MatchAdd(FP, "", Flags)) count++;
	Line=STREAMReadLine(Line, S);
	}
}

if (Flags & FLAG_MEMCACHED) printf("Stored %d hashes in memcached server\n", count);

DestroyString(Tempstr);
DestroyString(Line);
DestroyString(Type);

return(Tree);
}

