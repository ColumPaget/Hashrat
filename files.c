#include "files.h"
#include "ssh.h"
#include "memcached.h"
#include "fingerprint.h"
#include "xattr.h"
#include "check-hash.h"
#include "find.h"
#include <string.h>
#include <fnmatch.h>


dev_t StartingFS=0;





int FileType(const char *Path, int FTFlags, struct stat *Stat)
{

	if ((FTFlags & FLAG_NET))
	{
		if (strncasecmp(Path,"ssh:",4)==0) return(FT_SSH);
		if (strncasecmp(Path,"http:",5)==0) return(FT_HTTP);
		if (strncasecmp(Path,"https:",6)==0) return(FT_HTTP);
	}

	if ((! Stat) || (Stat->st_mode==0)) 
	{
	return(FT_FILE);
	}

	//if we are in devmode then just open the following things as files
	if (FTFlags & FLAG_DEVMODE)  return(FT_FILE);

	//otherwise treat them specially
	if (S_ISDIR(Stat->st_mode))  return(FT_DIR);
	if( S_ISCHR(Stat->st_mode))  return(FT_CHR);
	if (S_ISBLK(Stat->st_mode))  return(FT_BLK);
	if (S_ISFIFO(Stat->st_mode)) return(FT_FIFO);
	if (S_ISSOCK(Stat->st_mode)) return(FT_SOCK);
	if (S_ISLNK(Stat->st_mode))  return(FT_LNK);

	return(FT_FILE);
}


int IsIncluded(HashratCtx *Ctx, const char *Path, struct stat *FStat)
{
ListNode *Curr;
const char *mptr, *dptr;
int result=TRUE;

if (Ctx->Flags & CTX_EXCLUDE) result=FALSE;
if (FStat && S_ISDIR(FStat->st_mode)) result=TRUE;

Curr=ListGetNext(IncludeExclude);
while (Curr)
{
	mptr=(char *) Curr->Item;
	dptr=Path;

	//if match pattern doesn't start with '/' then we want to strip that off the current path
	//so that we can match against it. However if the match pattern contains no '/' at all, then
	//it's a file name rather than a path, in which case we should use basename on both it and 
	//the current path
	if (*mptr != '/') 
	{
		if (strchr(mptr,'/'))
		{
			if (*dptr=='/') dptr++;
		}
		else
		{
		mptr=GetBasename(mptr);
		dptr=GetBasename(Path);
		}
	}
	
	switch (Curr->ItemType)
	{
	case CTX_INCLUDE:
	if (fnmatch(mptr,dptr,0)==0) result=TRUE;
	break;

	case CTX_EXCLUDE:
	if (fnmatch(mptr,dptr,0)==0) result=FALSE;
	break;

/*
	case INEX_INCLUDE_DIR:
	if (strncmp(mptr,dptr,StrLen(mptr))==0) result=TRUE;
	break;

	case INEX_EXCLUDE_DIR:
	if (strncmp(mptr,dptr,StrLen(mptr))==0) result=FALSE;
	printf("FNMD: [%s] [%s] %d\n",mptr,dptr,result);
	break;
*/
	}

	Curr=ListGetNext(Curr);
}

return(result);
}




int StatFile(HashratCtx *Ctx, const char *Path, struct stat *Stat)
{
int val;

  //Pass NULL for stat, because we are only checking for 'net' type paths
  val=FileType(Path,Flags, NULL);
  if ((val !=FT_FILE) && (val !=FT_DIR) && (val != FT_LNK))
  {
    return(0);
  }

  if (Ctx->Flags & CTX_DEREFERENCE) val=stat(Path,Stat);
  else val=lstat(Path,Stat);

  return(val);
}




int GlobFiles(HashratCtx *Ctx, const char *Path, int FType, ListNode *Dirs)
{
char *Tempstr=NULL;
glob_t Glob;
int i, count=0;
struct stat *Stat;

switch (FType)
{
	case FT_SSH: return(SSHGlob(Ctx, Path, Dirs)); break;
}

Tempstr=MCopyStr(Tempstr,Path,"/*",NULL);
glob(Tempstr,0,0,&Glob);
for (i=0; i < Glob.gl_pathc; i++)
{
	if (
				(strcmp(Glob.gl_pathv[i],".") !=0) &&
				(strcmp(Glob.gl_pathv[i],"..") !=0)
		)
	{
		if (Dirs)
		{
			Stat=(struct stat *) calloc(1,sizeof(struct stat));
			if (StatFile(Ctx,Glob.gl_pathv[i], Stat) != -1) 
			{
				ListAddNamedItem(Dirs, Glob.gl_pathv[i], Stat);
			}
			else 
			{
				fprintf(stderr,"ERROR: Cannot stat file %s\n",Glob.gl_pathv[i]);
				free(Stat);
			}
		}
		count++;
	}
}

DestroyString(Tempstr);
globfree(&Glob);

return(count);
}






int HashratHashFile(HashratCtx *Ctx, THash *Hash, int Type, const char *Path, off_t FileSize)
{
STREAM *S;
char *Tempstr=NULL, *User=NULL, *Pass=NULL;
int result, val, RetVal=FALSE;
off_t bytes_read=0, oval;

switch (Type)
{
	case FT_HTTP:
		User=CopyStr(User,GetVar(Ctx->Vars,"HTTPUser"));
		Pass=CopyStr(Pass,GetVar(Ctx->Vars,"HTTPPass"));
		S=HTTPGet(Path,User,Pass); 
	break;

	case FT_SSH:
		S=SSHGet(Ctx, Path);
	break;

	default:
		if ((! StrValid(Path)) || (strcmp(Path,"-")==0)) S=STREAMFromFD(0);
		else
		{
			if (Ctx->Flags & CTX_DEREFERENCE) S=STREAMOpenFile(Path,SF_RDONLY|SF_SYMLINK_OK);
			else S=STREAMOpenFile(Path,SF_RDONLY);
		}
	break;
}

if (S) 
{
Tempstr=SetStrLen(Tempstr,BUFSIZ);

oval=FileSize;
if ((oval==0) || ( oval > BUFSIZ)) val=BUFSIZ;
else val=(int)oval;
result=STREAMReadBytes(S,Tempstr,val);
while (result > 0)
{
	Tempstr[result]='\0';

	if (result > 0) Hash->Update(Hash ,Tempstr, result);
	bytes_read+=result;

	if (FileSize > 0)
	{
	if ((Type != FT_HTTP) && (bytes_read >= FileSize)) break;
	oval=FileSize - bytes_read;
	if (oval > BUFSIZ) val=BUFSIZ;
	else val=(int)oval;
	}
	result=STREAMReadBytes(S,Tempstr,val);
}

if (Type != FT_SSH) STREAMClose(S);
RetVal=TRUE;
}

DestroyString(Tempstr);
DestroyString(User);
DestroyString(Pass);

//for now we close any connection after a 'get'
//Ctx->NetCon=NULL;

return(RetVal);
}



void HashratFinishHash(char **RetStr, HashratCtx *Ctx, THash *Hash)
{
int val;
const char *ptr;

		HashFinish(Hash,Ctx->Encoding,RetStr);

		ptr=GetVar(Ctx->Vars,"Output:Length");
		if (StrValid(ptr))
		{
			val=atoi(ptr);
			if ((val > 0) && (StrLen(*RetStr) > val)) (*RetStr)[val]='\0';
		}
}



int HashratHashSingleFile(HashratCtx *Ctx, const char *HashType, int FileType, const char *Path, struct stat *FStat, char **RetStr)
{
THash *Hash;
struct stat XattrStat;
const char *ptr;
off_t size=0;

		*RetStr=CopyStr(*RetStr,"");

		#ifdef USE_XATTR
		if (Ctx->Flags & CTX_XATTR_CACHE)
		{
			XAttrGetHash(Ctx, "user", Ctx->HashType, Path, &XattrStat, RetStr);
			//only use the hash cached in the xattr address if it's younger than the mtime
			if ( ((XattrStat.st_mtime - FStat->st_mtime) < 10) ) *RetStr=CopyStr(*RetStr,"");	
		}


		if (! StrValid(*RetStr))
		#endif
		{
			Hash=HashInit(HashType);
			if (Hash)
			{
			//If we're not doing HMAC then this doesn't do anything
			ptr=GetVar(Ctx->Vars,"EncryptionKey");
			if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

			if (FStat) size=FStat->st_size;
			if (! HashratHashFile(Ctx,Hash,FileType,Path, size)) return(FALSE);

			HashratFinishHash(RetStr, Ctx, Hash);
			}
		}

		if (StrValid(*RetStr)) return(TRUE);
		return(FALSE);
}



//This is used to processs small pieces of data like device IDs
void ProcessData(char **RetStr, HashratCtx *Ctx, const char *Data, int DataLen)
{
const char *ptr;
THash *Hash;

		if (! Ctx->Hash) 
		{
			Hash=HashInit(Ctx->HashType);
			if (Hash)
			{
			ptr=GetVar(Ctx->Vars,"EncryptionKey");
			if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

			Hash->Update(Hash ,Data, DataLen);
			HashratFinishHash(RetStr, Ctx, Hash);
			}
		} 
		else Ctx->Hash->Update(Ctx->Hash ,Data, DataLen);
}


int ConsiderItem(HashratCtx *Ctx, const char *Path, struct stat *FStat)
{
	int Type;

	Type=FileType(Path, Flags, FStat);
	if (! IsIncluded(Ctx, Path, FStat)) return(CTX_EXCLUDE);

	switch (Type)
	{
		case FT_SSH:
			if (SSHGlob(Ctx, Path, NULL) > 1) return(CTX_RECURSE);
		break;

		case FT_DIR:
		if (Ctx->Flags & CTX_RECURSE) return(CTX_RECURSE);
		break;
	}

	if (FStat)
	{
	if (Ctx->Flags & CTX_ONE_FS)
	{
		if (StartingFS==0) StartingFS=FStat->st_dev;
		else if (FStat->st_dev != StartingFS) return(CTX_ONE_FS);
	}
	if ((Ctx->Flags & CTX_EXES) && (! (FStat->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))) return(CTX_EXCLUDE);
	}

	return(0);
}


int HashItem(HashratCtx *Ctx, const char *HashType, const char *Path, struct stat *FStat, char **HashStr)
{
int Type=FT_FILE, val;
char *Tempstr=NULL;


	Type=FileType(Path, Flags, FStat);
	switch (Type)
	{
		case FT_HTTP:
			return(HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr));
		break;

		case FT_SSH:
			return(HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr));
		break;

		case FT_DIR:
		if (Ctx->Flags & CTX_RECURSE) return(CTX_RECURSE);
		else ProcessData(HashStr, Ctx, (const char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_CHR:
			ProcessData(HashStr, Ctx, (const char *) &FStat->st_rdev, sizeof(dev_t));
		break;

		case FT_BLK:
			ProcessData(HashStr, Ctx, (const char *) &FStat->st_rdev, sizeof(dev_t));
		break;

		case FT_FIFO:
			ProcessData(HashStr, Ctx, (const char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_SOCK:
		 ProcessData(HashStr, Ctx, (const char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_LNK:
		if (Ctx->Flags & CTX_DEREFERENCE)
		{
			Tempstr=SetStrLen(Tempstr,PATH_MAX);
			val=readlink(Path, Tempstr,PATH_MAX);
			if (val > 0)
			{
				Tempstr[val]='\0';
				ProcessData(HashStr, Ctx, Tempstr, val);
			}
		}
		else fprintf(stderr,"WARN: Not following symbolic link %s\n",Path);
		break;

		default:
		if (! Ctx->Hash) 
		{
			if (! HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr)) return(FALSE);
		} else if (! HashratHashFile(Ctx, Ctx->Hash,Type,Path, FStat->st_size)) return(FALSE);
		break;
	}

DestroyString(Tempstr);

return(TRUE);
}



//HashratAction returns true on a significant event, which is either an item found in search
//or a check failing in hash-checking mode
int HashratAction(HashratCtx *Ctx, const char *Path, struct stat *Stat)
{
char *HashStr=NULL;
int Type, result=FALSE;
TFingerprint *FP=NULL;

switch (Ctx->Action)
{
case ACT_HASHDIR:
			Type=FileType(Path, Flags, Stat);
			//we return TRUE if hash succeeded
			if (HashratHashFile(Ctx, Ctx->Hash, Type, Path, Stat->st_size)) result=TRUE;
break;

case ACT_HASH:
	if (HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr))
	{
	HashratOutputInfo(Ctx, Ctx->Out, Path, Stat, HashStr);
	HashratStoreHash(Ctx, Path, Stat, HashStr);
	result=TRUE;
	}
break;

case ACT_CHECK:
	if (S_ISREG(Stat->st_mode))
	{
		if (Stat->st_size > 0)
		{
			HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr);
			FP=CheckForMatch(Ctx, Path, Stat, HashStr);
			if (FP && HashratCheckFile(Ctx, Path, Stat, HashStr, FP)) MatchCount++;
			else 
			{
					HandleCheckFail(Path, "Changed or new");
					//we return TRUE on FAILURE, as we are signaling a significant event
					result=TRUE;
			}
		}
		else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
break;

case ACT_CHECK_XATTR:
	if (S_ISREG(Stat->st_mode))
	{
		//result == TRUE by default (TRUE==Signficant event, here meaning 'check failed')
		//we set it here so we get the right result even if the stored hash fails to load
		result=TRUE;
		FP=XAttrLoadHash(Ctx, Path);
		if (FP) 
		{
			HashItem(Ctx, FP->HashType, Path, Stat, &HashStr);
			if (FP->Flags & FP_HASSTAT) if (HashratCheckFile(Ctx, Path, Stat, HashStr, FP)) result=FALSE;
			else if (HashratCheckFile(Ctx, Path, Stat, HashStr, FP)) result=FALSE;
		}
		else fprintf(stderr,"ERROR: No stored hash for '%s'\n",Path);
	}
	else fprintf(stderr,"ERROR: Not regular file '%s'. Not checking in xattr mode.\n",Path);
break;


case ACT_CHECK_MEMCACHED:
	if (S_ISREG(Stat->st_mode))
	{
		//result == TRUE by default (TRUE==Signficant event, here meaning 'check failed')
		result=TRUE;

		if (Stat->st_size > 0)
		{
		HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr);
		FP=(TFingerprint *) calloc(1,sizeof(TFingerprint));
		if (Flags & FLAG_NET) FP->Path=MCopyStr(FP->Path, Path);
		else FP->Path=MCopyStr(FP->Path,"hashrat://",LocalHost,Path,NULL);
		FP->Hash=MemcachedGet(FP->Hash, FP->Path);

		if (FP && HashratCheckFile(Ctx, Path, NULL, HashStr, FP)) result=FALSE;
		else fprintf(stderr,"ERROR: No stored hash for '%s'\n",Path);
		}
		else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
	else fprintf(stderr,"ERROR: Not regular file '%s'. Not checking in memcached mode.\n",Path);
break;

case ACT_FINDMATCHES:
case ACT_FINDMATCHES_MEMCACHED:
	if (S_ISREG(Stat->st_mode))
	{
		if (Stat->st_size > 0)
		{
		HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr);
		FP=CheckForMatch(Ctx, Path, Stat, HashStr);
		if (FP)
		{
			if (StrValid(FP->Path) || StrValid(FP->Data)) printf("LOCATED: %s '%s %s' at %s\n",FP->Hash, FP->Path, FP->Data, Path);
			else printf("LOCATED: %s at %s\n",FP->Hash, Path);
			MatchCount++;
			//here we return true if a match found
			result=TRUE;
		}
		else DiffCount++;
		}
		else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
break;

case ACT_FINDDUPLICATES:
	if (S_ISREG(Stat->st_mode))
	{
	if (Stat->st_size > 0)
	{
		if (HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr))
		{
			FP=CheckForMatch(Ctx, Path, Stat, HashStr);
			if (FP)
			{
				printf("DUPLICATE: %s of %s %s\n",Path,FP->Path,FP->Data);
				MatchCount++;
			//here we return true if a match found
				result=TRUE;
			}
			else 
			{
				FP=TFingerprintCreate(HashStr, Ctx->HashType, "", Path);
				DiffCount++;
				MatchAdd(FP, Path, 0);
				//as we've added FP to an internal list we don't want it destroyed
				FP=NULL;
			}
		}
	}
	else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
break;
}

if (result==TRUE) 
{
	if (FP) RunHookScript(DiffHook, Path, FP->Path);
	else RunHookScript(DiffHook, Path, "");
}

if (FP) TFingerprintDestroy(FP);	
DestroyString(HashStr);

return(result);
}


//ProcessItem returns TRUE on a significant event, so any instance of TRUE
//from items checked makes return value here TRUE
int ProcessDir(HashratCtx *Ctx, const char *Dir, const char *HashType)
{
char *Tempstr=NULL, *HashStr=NULL;
ListNode *FileList, *Curr;
int result=FALSE;
int Type;

		Type=FileType(Dir, Flags, NULL);

		FileList=ListCreate();
		GlobFiles(Ctx, Dir, Type, FileList);

		Curr=ListGetNext(FileList);
		while (Curr)
		{
			if (ProcessItem(Ctx, Curr->Tag, (struct stat *) Curr->Item)) result=TRUE;
			Curr=ListGetNext(Curr);
		}

		ListDestroy(FileList,free);

DestroyString(Tempstr);
DestroyString(HashStr);

return(result);
}


//ProcessDir returns TRUE on a significant event, so any instance of TRUE
//from items checked makes return value here TRUE
int HashratRecurse(HashratCtx *Ctx, const char *Path, char **HashStr)
{
const char *ptr;
struct stat FStat;
int result=FALSE;

		if ((Ctx->Action == ACT_HASHDIR) && (! Ctx->Hash)) 
		{
				Ctx->Hash=HashInit(Ctx->HashType);
				ptr=GetVar(Ctx->Vars,"EncryptionKey");
				if (ptr) HMACSetKey(Ctx->Hash, ptr, StrLen(ptr));

				if (! ProcessDir(Ctx, Path, Ctx->HashType)) result=FALSE;
				HashratFinishHash(HashStr, Ctx, Ctx->Hash);
				stat(Path, &FStat);
				HashratOutputInfo(Ctx, Ctx->Out, Path, &FStat, *HashStr);
				result=TRUE;
		}
		else if (ProcessDir(Ctx, Path, Ctx->HashType)) result=TRUE;

	return(result);
}




int ProcessItem(HashratCtx *Ctx, const char *Path, struct stat *Stat)
{
char *HashStr=NULL;
int result=FALSE, Flags;

				switch (ConsiderItem(Ctx, Path, Stat))
				{
					case CTX_EXCLUDE:
					case CTX_ONE_FS:
						result=IGNORE;
					break;

					case CTX_RECURSE:
					result=HashratRecurse(Ctx, Path, &HashStr);
					break;

					default:
					result=HashratAction(Ctx, Path, Stat);
					break;
				}

DestroyString(HashStr);

return(result);
}



