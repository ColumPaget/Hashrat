#include "common.h"


int Flags=0;
char *DiffHook=NULL;
char *Key=NULL;
char *LocalHost=NULL;

char *HashratHashTypes[]={"md5","sha1","sha256","sha512","whirl","whirlpool","jh-224","jh-256","jh-384","jh-512",NULL};



void TFingerprintDestroy(void *p_Fingerprint)
{
  TFingerprint *Fingerprint;

	if (! p_Fingerprint) return;
  Fingerprint=(TFingerprint *) p_Fingerprint;
  DestroyString(Fingerprint->Hash);
  DestroyString(Fingerprint->Data);
  DestroyString(Fingerprint->Path);
  DestroyString(Fingerprint->HashType);

  free(Fingerprint);
}



TFingerprint *TFingerprintCreate(const char *Hash, const char *HashType, const char *Data, const char *Path)
{
TFingerprint *Item;

  Item=(TFingerprint *) calloc(1,sizeof(TFingerprint));
  Item->Hash=CopyStr(Item->Hash, Hash);
 	strlwr(Item->Hash);
  Item->HashType=CopyStr(Item->HashType, HashType);
  Item->Data=CopyStr(Item->Data, Data);
  Item->Path=CopyStr(Item->Path, Path);

  return(Item);
}



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
char *Tempstr=NULL; 

if (Flags & FLAG_TRAD_OUTPUT) Tempstr=MCopyStr(Tempstr,Hash, "  ", Path,NULL);
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

#if _FILE_OFFSET_BITS == 64
	Tempstr=FormatStr(Tempstr,"hash='%s:%s' mode='%o' uid='%lu' gid='%lu' size='%llu' mtime='%lu' inode='%llu' path='%s'",Ctx->HashType,Hash,Stat->st_mode,Stat->st_uid,Stat->st_gid,Stat->st_size,Stat->st_mtime,Stat->st_ino,Path);
#else
	Tempstr=FormatStr(Tempstr,"hash='%s:%s' mode='%o' uid='%lu' gid='%lu' size='%lu' mtime='%lu' inode='%lu' path='%s'",Ctx->HashType,Hash,Stat->st_mode,Stat->st_uid,Stat->st_gid,Stat->st_size,Stat->st_mtime,Stat->st_ino,Path);
#endif
}

Tempstr=CatStr(Tempstr,"\n");

STREAMWriteString(Tempstr,Out);

DestroyString(Tempstr);
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


int HandleCompareResult(char *Path, char *Status, int Flags, char *ErrorMessage)
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

