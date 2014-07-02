#include "common.h"


int Flags=0;
char *DiffHook=NULL;
char *Key=NULL;





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


int HashratOutputInfo(HashratCtx *Ctx, char *Path, struct stat *Stat, char *Hash)
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

#ifdef _LARGEFILE64_SOURCE
	Tempstr=FormatStr(Tempstr,"hash='%s:%s' mode='%o' uid='%lu' gid='%lu' size='%llu' mtime='%lu' inode='%llu' path='%s'",Ctx->HashType,Hash,Stat->st_mode,Stat->st_uid,Stat->st_gid,Stat->st_size,Stat->st_mtime,Stat->st_ino,Path);
#else
	Tempstr=FormatStr(Tempstr,"hash='%s:%s' mode='%o' uid='%lu' gid='%lu' size='%lu' mtime='%lu' inode='%lu' path='%s'",Ctx->HashType,Hash,Stat->st_mode,Stat->st_uid,Stat->st_gid,Stat->st_size,Stat->st_mtime,Stat->st_ino,Path);
#endif
}

Tempstr=CatStr(Tempstr,"\n");

STREAMWriteString(Tempstr,Ctx->Out);

DestroyString(Tempstr);
}


void HandleCheckFail(char *Path, char *ErrorMessage)
{
char *Tempstr=NULL;

  printf("%s: FAILED. %s.\n",Path,ErrorMessage);
  if (StrLen(DiffHook))
  {
    Tempstr=MCopyStr(Tempstr,DiffHook," '",Path,"'",NULL);
    system(Tempstr);
  }

  DestroyString(Tempstr);
}



int HashratCheckFile(HashratCtx *Ctx,TFingerprint *FP)
{
int result=FALSE;
char *HashStr=NULL;
struct stat Stat;
int enc;

		if (Flags & FLAG_XATTR) XAttrLoadHash(FP);

    //Set encoding from args
    if (Ctx->Flags & CTX_BASE8) enc = ENCODE_OCTAL;
    else if (Ctx->Flags & CTX_BASE10) enc = ENCODE_DECIMAL;
    else if (Ctx->Flags & CTX_HEXUPPER) enc = ENCODE_HEXUPPER;
    else if (Ctx->Flags & CTX_BASE64) enc = ENCODE_BASE64;
    else enc= ENCODE_HEX;

		HashStr=CopyStr(HashStr,"");

		if (access(FP->Path,F_OK)!=0) fprintf(stderr,"\rERROR: No such file '%s'\n",FP->Path);
		else if (FP->Flags & FP_HASSTAT)
		{
			if (StatFile(FP->Path,&Stat)==0)
			{
					//printf("%llu %llu\n",FP->FStat.st_size,Stat.st_size);
				if ((FP->FStat.st_size > 0) && ((size_t) Stat.st_size != (size_t) FP->FStat.st_size)) HandleCheckFail(FP->Path, "Filesize changed");
				else if ((Flags & FLAG_FULLCHECK) && (Stat.st_ino != FP->FStat.st_ino)) HandleCheckFail(FP->Path, "Inode changed");
				else if ((Flags & FLAG_FULLCHECK) && (Stat.st_uid != FP->FStat.st_uid)) HandleCheckFail(FP->Path, "Owner changed");
				else if ((Flags & FLAG_FULLCHECK) && (Stat.st_gid != FP->FStat.st_gid)) HandleCheckFail(FP->Path, "Group changed");
				else if ((Flags & FLAG_FULLCHECK) && (Stat.st_mode != FP->FStat.st_mode)) HandleCheckFail(FP->Path, "Mode changed");
				else if ((Flags & FLAG_FULLCHECK) && (Stat.st_mtime != FP->FStat.st_mtime)) HandleCheckFail(FP->Path, "MTime changed");
				else
				{
					HashFile(&HashStr, FP->HashType, FP->Path, enc);
					if (strcasecmp(FP->Hash,HashStr)!=0) HandleCheckFail(FP->Path, "Hash mismatch");
					else if (! (Flags & FLAG_OUTPUT_FAILS))
					{
						printf("%s: OKAY\n",FP->Path);
						result=TRUE;
					}
				}
			}
			else fprintf(stderr,"\rERROR: Failed to open '%s'\n",FP->Path);
		}
		else
		{
				HashFile(&HashStr, FP->HashType, FP->Path, enc);
				if (strcasecmp(FP->Hash,HashStr)!=0) HandleCheckFail(FP->Path, "Hash mismatch");
				else if (! (Flags & FLAG_OUTPUT_FAILS)) printf("%s: OKAY\n",FP->Path);
		}

DestroyString(HashStr);
return(result);
}



int HashratAction(HashratCtx *Ctx, char *Path, struct stat *Stat, char *Hash)
{
	switch (Ctx->Action)
	{
	case ACT_HASH:
	default:
	HashratOutputInfo(Ctx, Path, Stat, Hash);
	//if CTX_CACHED is set, then unset. Otherwise update XATTR for this item
	printf("CACH: %d\n",Ctx->Flags & CTX_CACHED);

	if (Ctx->Flags & CTX_CACHED) Ctx->Flags &= ~CTX_CACHED;
	else if (Flags & FLAG_XATTR) HashRatSetXAttr(Ctx->Out, Path, Stat, Ctx->HashType, Hash);
	break;

	case ACT_CHECK:
//		if (Flags & FLAG_XATTR) XAttrLoadHash(FP);
//        if (strcasecmp(FP->Hash,HashStr)!=0) HandleCheckFail(FP->Path, "Hash mismatch");
//        else if (! (Flags & FLAG_OUTPUT_FAILS)) printf("%s: OKAY\n",FP->Path);
	break;
	}
}


