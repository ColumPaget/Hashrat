#include "common.h"
#include "xattr.h"
#include "memcached.h"

int Flags=0;
char *DiffHook=NULL;
char *Key=NULL;
char *LocalHost=NULL;
ListNode *IncludeExclude=NULL;
int MatchCount=0, DiffCount=0;

char *HashratHashTypes[]={"md5","sha1","sha256","sha512","whirl","whirlpool","jh-224","jh-256","jh-384","jh-512",NULL};




void HashratCtxDestroy(void *p_Ctx)
{
HashratCtx *Ctx;

Ctx=(HashratCtx *) p_Ctx;
STREAMClose(Ctx->Out);
DestroyString(Ctx->HashType);
DestroyString(Ctx->ListPath);
DestroyString(Ctx->HashStr);
HashDestroy(Ctx->Hash);
free(Ctx);
}



int HashratOutputInfo(HashratCtx *Ctx, STREAM *Out, char *Path, struct stat *Stat, char *Hash)
{
char *Line=NULL, *Tempstr=NULL, *ptr; 
char *p_Type="unknown";

if (Flags & FLAG_TRAD_OUTPUT) Line=MCopyStr(Line,Hash, "  ", Path,NULL);
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
	
	Line=MCatStr(Line,"path='",Path,"'\n",NULL);
}

STREAMWriteString(Line,Out);

DestroyString(Tempstr);
DestroyString(Line);

return(TRUE);
}




void HashratStoreHash(HashratCtx *Ctx, char *Path, struct stat *Stat, char *Hash)
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
	DestroyString(Tempstr);
}


void HandleCompareResult(char *Path, char *Status, int Flags, char *ErrorMessage)
{
char *Tempstr=NULL;
int Color=0;

  if (Flags & FLAG_COLOR) 
	{
		switch (Flags & FLAG_RESULT_MASK)
		{
			case RESULT_FAIL: Color=ANSI_RED; break;
			case RESULT_PASS: Color=ANSI_GREEN; break;
			case RESULT_WARN: Color=ANSI_YELLOW; break;
		}
	}

	if (Color > 0) printf("%s%s: %s. '%s'.%s\n",ANSICode(ANSI_RED, 0, 0),Status, Path, ErrorMessage, ANSI_NORM);
  else printf("%s: %s. %s.\n",Status, Path,ErrorMessage);

  if ((Flags & RESULT_RUNHOOK) && StrLen(DiffHook))
  {
    Tempstr=MCopyStr(Tempstr,DiffHook," '",Path,"'",NULL);
    system(Tempstr);
  }

  DestroyString(Tempstr);
}

