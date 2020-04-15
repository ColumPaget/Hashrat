#include "common.h"
#include "xattr.h"
#include "memcached.h"

int Flags=0;
char *DiffHook=NULL;
char *Key=NULL;
char *LocalHost=NULL;
ListNode *IncludeExclude=NULL;
int MatchCount=0, DiffCount=0;
time_t Now;
uint64_t HashStartTime;

const char *HashratHashTypes[]={"md5","sha1","sha256","sha512","whirl","whirlpool","jh-224","jh-256","jh-384","jh-512",NULL};




void HashratCtxDestroy(void *p_Ctx)
{
HashratCtx *Ctx;

Ctx=(HashratCtx *) p_Ctx;
STREAMClose(Ctx->Out);
Destroy(Ctx->HashType);
Destroy(Ctx->HashStr);
Destroy(Ctx->Targets);
HashDestroy(Ctx->Hash);
free(Ctx);
}


char *ReformatHash(char *RetStr, const char *Str, int OutputLen, int SegmentSize, char SegmentChar)
{
int ipos=0, opos=0;

if (OutputLen==0) OutputLen=StrLen(Str);

for (ipos=0; ipos < OutputLen; ipos++)
{
	//don't increment pos, the loop does this one
	RetStr=AddCharToBuffer(RetStr, opos++, Str[ipos]);
	if (Str[ipos]=='\0') break;

	if ((SegmentSize > 0) && (ipos > 0) && (OutputLen-ipos > 1) && (((ipos+1) % SegmentSize)==0)) 
	{
		RetStr=AddCharToBuffer(RetStr, opos++, SegmentChar);
	}

}

return(RetStr);
}


int HashratOutputInfo(HashratCtx *Ctx, STREAM *Out, const char *Path, struct stat *Stat, const char *iHash)
{
char *Line=NULL, *Tempstr=NULL, *Hash=NULL, *ptr; 
const char *p_Type="unknown";
uint64_t diff;

//printf("SEG: %d %d\n", Ctx->OutputLength, Ctx->SegmentLength);

if (Ctx->SegmentLength > 0) Hash=ReformatHash(Hash, iHash, Ctx->OutputLength, Ctx->SegmentLength, Ctx->SegmentChar);
else Hash=CopyStr(Hash, iHash);

if (Ctx->OutputLength > 0) StrTrunc(Hash, Ctx->OutputLength);

if (Flags & FLAG_TRAD_OUTPUT) Line=MCopyStr(Line,Hash, "  ", Path,"\n",NULL);
else if (Flags & FLAG_BSD_OUTPUT) 
{
	Line=CopyStr(Line,Ctx->HashType);
	for (ptr=Line; *ptr != '\0'; ptr++) *ptr=toupper(*ptr);
	Line=MCatStr(Line, " (", Path, ") = ", Hash, "\n",NULL);
}
else
{
	/*
		struct stat {
    dev_t     st_dev;     // ID of device containing file 
    ino_t     st_ino;     // inode number 
    mode_t    st_mode;    // protection 
    nlink_t   st_nlink;   // number of hard links 
    uid_t     st_uid;     // user ID of owner
    gid_t     st_gid;     // group ID of owner
    dev_t     st_rdev;    // device ID (if special file) 
    off_t     st_size;    // total size, in bytes 
    blksize_t st_blksize; // blocksize for file system I/O 
    blkcnt_t  st_blocks;  // number of 512B blocks allocated 
    time_t    st_atime;   // time of last access 
    time_t    st_mtime;   // time of last modification 
    time_t    st_ctime;   // time of last status change 
	*/

	if (S_ISREG(Stat->st_mode)) p_Type="file";
	else if (S_ISDIR(Stat->st_mode)) p_Type="dir";
	else if (S_ISLNK(Stat->st_mode)) p_Type="link";
	else if (S_ISSOCK(Stat->st_mode)) p_Type="socket";
	else if (S_ISFIFO(Stat->st_mode)) p_Type="fifo";
	else if (S_ISCHR(Stat->st_mode)) p_Type="chrdev";
	else if (S_ISBLK(Stat->st_mode)) p_Type="blkdev";
	
	Line=FormatStr(Line,"hash='%s:%s' type='%s' mode='%o' uid='%lu' gid='%lu' ", Ctx->HashType,Hash,p_Type,Stat->st_mode,Stat->st_uid,Stat->st_gid);
	
	//This dance is to handle the fact that on some 32-bit OS, like openbsd, stat with have 64-bit members
	//even if we've not asked for 'largefile' support, while on Linux it will have 32-bit members

	//Let's hope the compiler optimizes the fuck out of this

	if (sizeof(Stat->st_size)==sizeof(unsigned long long)) Tempstr=FormatStr(Tempstr, "size='%llu' ",Stat->st_size);
	else Tempstr=FormatStr(Tempstr, "size='%lu' ",Stat->st_size);
	Line=CatStr(Line, Tempstr);

	if (sizeof(Stat->st_mtime)==sizeof(unsigned long long)) Tempstr=FormatStr(Tempstr, "mtime='%llu' ",Stat->st_mtime);
	else Tempstr=FormatStr(Tempstr, "mtime='%lu' ",Stat->st_mtime);
	Line=CatStr(Line, Tempstr);
	
	if (sizeof(Stat->st_ino)==sizeof(unsigned long long)) Tempstr=FormatStr(Tempstr, "inode='%llu' ",Stat->st_ino);
	else Tempstr=FormatStr(Tempstr, "inode='%lu' ",Stat->st_ino);
	Line=CatStr(Line, Tempstr);

	if (Flags & FLAG_VERBOSE)
	{
		diff=GetTime(TIME_MILLISECS) - HashStartTime;
		if (diff > 1000) Tempstr=FormatStr(Tempstr, "hash-time='%0.2fs' ", (double) diff / 1000.0	);
		else Tempstr=FormatStr(Tempstr, "hash-time='%llums' ", (unsigned long long) diff	);
		Line=CatStr(Line, Tempstr);
	}
	
	//we must quote out apostrophes
	Tempstr=QuoteCharsInStr(Tempstr, Path, "'");
	Line=MCatStr(Line,"path='",Tempstr,"'\n",NULL);
}

STREAMWriteString(Line,Out);

Destroy(Tempstr);
Destroy(Line);
Destroy(Hash);

return(TRUE);
}




void HashratStoreHash(HashratCtx *Ctx, const char *Path, struct stat *Stat, const char *Hash)
{
char *Tempstr=NULL;

	//if CTX_CACHED is set, then unset. Otherwise update XATTR for this item
//	if (Ctx->Flags & CTX_CACHED) Ctx->Flags &= ~CTX_CACHED;
//	else 

	
	if (Ctx->Flags & CTX_STORE_XATTR) HashRatSetXAttr(Ctx, Path, Stat, Ctx->HashType, Hash);

	if (Ctx->Flags & CTX_STORE_MEMCACHED)
	{
		if (Flags & FLAG_NET) Tempstr=MCopyStr(Tempstr, Path);
		else Tempstr=MCopyStr(Tempstr,"hashrat://",LocalHost,Path,NULL);
		MemcachedSet(Hash, 0, Tempstr);
		MemcachedSet(Tempstr, 0, Hash);
	}

	if (Ctx->Aux) HashratOutputInfo(Ctx, Ctx->Aux, Path, Stat, Hash);
	Destroy(Tempstr);
}


void RunHookScript(const char *Hook, const char *Path, const char *Other)
{
char *Tempstr=NULL, *QuotedPath=NULL, *QuotedOther=NULL;
STREAM *S;

  if (StrValid(Hook))
  {
		//must quote twice to get through system comamnd
    QuotedPath=QuoteCharsInStr(QuotedPath, Path,"\"'`!|;<> 	");
    QuotedOther=QuoteCharsInStr(QuotedOther, Other,"\"'`!|;<> 	");
		S=STREAMSpawnCommand("/bin/sh",0);
		if (S)
		{
    	Tempstr=MCopyStr(Tempstr, DiffHook," ",QuotedPath, " ", QuotedOther, ";exit\n",NULL);
			STREAMWriteLine(Tempstr,S);
			STREAMFlush(S);

			Tempstr=STREAMReadLine(Tempstr,S);
			while (Tempstr)
			{
			printf("%s\n",Tempstr);
			Tempstr=STREAMReadLine(Tempstr,S);
			}
		}
  }

Destroy(Tempstr);
Destroy(QuotedPath);
Destroy(QuotedOther);
}



