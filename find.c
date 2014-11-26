#include "find.h"
#include "files.h"
#include "memcached.h"



typedef struct
{
char *ID;
char *Data;
char *Path;
} TMatch;

void *Tree;


int MatchCompareFunc(void *i1, void *i2)
{
TMatch *M1, *M2;

M1=(TMatch *) i1;
M2=(TMatch *) i2;

return(strcmp(M1->ID, M2->ID));
}

void *MatchesLoad()
{
char *Tempstr=NULL, *Token=NULL, *ptr;
TMatch *Item;
STREAM *S;


S=STREAMFromFD(0);
STREAMSetTimeout(S,1);
Tempstr=STREAMReadLine(Tempstr,S);
if (! StrLen(Tempstr)) return(NULL);
while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	strlwr(Tempstr);
	Item=(TMatch *) calloc(1,sizeof(TMatch));
	ptr=GetToken(Tempstr,"\\S",&Item->ID,0);
	Item->Data=CopyStr(Item->Data, ptr);
	tsearch(Item, &Tree, MatchCompareFunc);
	
	Tempstr=STREAMReadLine(Tempstr, S);
}

DestroyString(Tempstr);
DestroyString(Token);

return(Tree);
}


void CheckForMatch(HashratCtx *Ctx, char *Path, struct stat *FStat)
{
char *Tempstr=NULL;
TMatch *Found;
TMatch Lookup;
void *ptr;

memset(&Lookup, 0, sizeof(TMatch));
if (S_ISREG(FStat->st_mode))
{
	Lookup.Path=CopyStr(Lookup.Path,Path);
	if  (HashratHashSingleFile(&Lookup.ID, Ctx, FT_FILE, Path, FStat))
	{	
		if (Ctx->Action==ACT_FINDMATCHES_MEMCACHED)
		{
		Tempstr=MemcachedGet(Tempstr, Lookup.ID);
		if (StrLen(Tempstr)) printf("LOCATED: %s  %s  %s (memcached)\n",Lookup.ID,Lookup.Path,Tempstr);
		}
		else
		{
			ptr=tfind(&Lookup, &Tree, MatchCompareFunc);
			if (ptr)
			{
				Found=*(TMatch **) ptr;
				printf("LOCATED: %s  %s  %s\n",Lookup.ID,Lookup.Path,Found->Data);
			}
		}
	}
}

DestroyString(Tempstr);
}




void FindMatches(HashratCtx *Ctx, int argc, char *argv[])
{
ListNode *Node;
struct stat FStat;
int i, val;
TMatch Item; 

if (! (Flags & FLAG_MEMCACHED)) Tree=MatchesLoad();

for (i=1; i < argc; i++)
{
	if (StrLen(argv[i]))
	{
		val=StatFile(Ctx, argv[i], &FStat);
		CheckForMatch(Ctx, argv[i], &FStat);
	}
}

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

