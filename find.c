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


void MatchAdd(const char *ID, const char *HashType, const char *Data, const char *Path)
{
TFingerprint *Item;

	Item=TFingerprintCreate(ID, HashType, Data, Path);
	tsearch(Item, &Tree, MatchCompareFunc);
}
	

void *MatchesLoad()
{
char *Tempstr=NULL, *Token=NULL, *ptr;
STREAM *S;


S=STREAMFromFD(0);
STREAMSetTimeout(S,1);
Tempstr=STREAMReadLine(Tempstr,S);
if (! StrLen(Tempstr)) return(NULL);
while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	ptr=GetToken(Tempstr,"\\S",&Token,0);
	MatchAdd(Token,"", ptr,"");
	Tempstr=STREAMReadLine(Tempstr, S);
}

DestroyString(Tempstr);
DestroyString(Token);

return(Tree);
}


TFingerprint *CheckForMatch(HashratCtx *Ctx, char *Path, struct stat *FStat, char *HashStr)
{
TFingerprint *Lookup, *Found, *Result=NULL;
void *ptr;


Lookup=TFingerprintCreate(HashStr,"","",Path);
if (Ctx->Action==ACT_FINDMATCHES_MEMCACHED)
{
		Lookup->Data=MemcachedGet(Lookup->Data, Lookup->Hash);
		if (StrLen(Lookup->Data)) Result=TFingerprintCreate(Lookup->Hash, Lookup->HashType, Lookup->Data, Lookup->Path);
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






void LoadMatchesToMemcache()
{
char *Tempstr=NULL, *Token=NULL, *ptr;
STREAM *S;
int stored=0;

S=STREAMFromFD(0);
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	strlwr(Tempstr);
	ptr=GetToken(Tempstr,"\\S",&Token,0);
	if (MemcachedSet(Token, 0, ptr)) stored++;
	Tempstr=STREAMReadLine(Tempstr, S);
}

printf("Stored %d hashes in memcached server\n", stored);

DestroyString(Tempstr);
DestroyString(Token);
}

