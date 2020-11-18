#include "files.h"
#include "ssh.h"
#include "http.h"
#include "memcached.h"
#include "fingerprint.h"
#include "xattr.h"
#include "check-hash.h"
#include "include-exclude.h"
#include "find.h"
#include <string.h>
#include <fnmatch.h>

//this is a map of all directories/collections visited, (webpages in the case of http). It's used to
//prevent getting trapped in loops
ListNode *Visited=NULL;



static int FileType(const char *Path, int FTFlags, struct stat *Stat)
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




int StatFile(HashratCtx *Ctx, const char *Path, struct stat *Stat)
{
int val;

	memset(Stat, 0, sizeof(struct stat));
  //Pass NULL for stat, because we are only checking for 'net' type paths
  val=FileType(Path, Flags, NULL);

  if ((val !=FT_FILE) && (val !=FT_DIR) && (val != FT_LNK))
  {
    return(val);
  }

  if (Ctx->Flags & CTX_DEREFERENCE) val=stat(Path,Stat);
  else val=lstat(Path,Stat);

	if (val==0) return(FileType(Path,Flags, Stat));
  return(val);
}



void GlobFiles(HashratCtx *Ctx, const char *Path, int FType, ListNode *Dirs)
{
char *Tempstr=NULL;
glob_t Glob;
int i, flags=0;
struct stat *Stat;

ListClear(Dirs, Destroy);
switch (FType)
{
	case FT_SSH:
		 SSHGlob(Ctx, Path, Dirs);
		 return; 
	break;

	case FT_HTTP:
		HTTPGlob(Ctx, Path, Dirs);
		return;
	break;
}

Tempstr=QuoteCharsInStr(Tempstr, Path, "[]*?");
Tempstr=CatStr(Tempstr,"/*");
#ifdef GLOB_PERIOD
if (Ctx->Flags & CTX_HIDDEN) flags |= GLOB_PERIOD;
#endif
glob(Tempstr,flags,0,&Glob);
for (i=0; i < Glob.gl_pathc; i++)
{
	if (
				(strcmp(GetBasename(Glob.gl_pathv[i]),".") !=0) &&
				(strcmp(GetBasename(Glob.gl_pathv[i]),"..") !=0)
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
	}
}

Destroy(Tempstr);
globfree(&Glob);
}






int HashratHashFile(HashratCtx *Ctx, HASH *Hash, int Type, const char *Path, struct stat *FStat)
{
STREAM *S;
char *Tempstr=NULL, *User=NULL, *Pass=NULL;
const char *ptr;
int result, val, RetVal=FALSE;
off_t bytes_read=0, size=0, oval;

if (FStat) size=FStat->st_size;
switch (Type)
{
	case FT_HTTP:
		User=CopyStr(User,GetVar(Ctx->Vars,"HTTPUser"));
		Pass=CopyStr(Pass,GetVar(Ctx->Vars,"HTTPPass"));
		S=HTTPGet(Path); 
		ptr=STREAMGetValue(S, "HTTP:ResponseCode");
		if ((!ptr) || (*ptr !='2'))
		{
			STREAMClose(S);
			S=NULL;
		}
	break;

	case FT_SSH:
		S=SSHGet(Ctx, Path);
	break;

	default:
		if ((! StrValid(Path)) || (strcmp(Path,"-")==0)) S=STREAMFromFD(0);
		else
		{
			if (Ctx->Flags & CTX_DEREFERENCE) S=STREAMOpen(Path,"rf");
			else S=STREAMOpen(Path,"rf");
		}
	break;
}

if (S) 
{
Tempstr=SetStrLen(Tempstr,BUFSIZ);

oval=size;
if ((oval==0) || ( oval > BUFSIZ)) val=BUFSIZ;
else val=(int)oval;
result=STREAMReadBytes(S,Tempstr,val);
while (result > STREAM_CLOSED)
{
	Tempstr[result]='\0';

	if (result > 0) Hash->Update(Hash ,Tempstr, result);
	bytes_read+=result;

	if (size > 0)
	{
	if ((Type != FT_HTTP) && (bytes_read >= size)) break;
	oval=size - bytes_read;
	if (oval > BUFSIZ) val=BUFSIZ;
	else val=(int)oval;
	}
	result=STREAMReadBytes(S,Tempstr,val);
}

if (FStat && (FStat->st_size==0)) FStat->st_size=bytes_read;
if (Type != FT_SSH) STREAMClose(S);
RetVal=TRUE;
}

Destroy(Tempstr);
Destroy(User);
Destroy(Pass);

//for now we close any connection after a 'get'
//Ctx->NetCon=NULL;

return(RetVal);
}



void HashratFinishHash(char **RetStr, HashratCtx *Ctx, HASH *Hash)
{
int val;
const char *ptr;

		HashFinish(Hash,Ctx->Encoding,RetStr);

		/*
		ptr=GetVar(Ctx->Vars,"Output:Length");
		if (StrValid(ptr))
		{
			val=atoi(ptr);
			if ((val > 0) && (StrLen(*RetStr) > val)) (*RetStr)[val]='\0';
		}
		*/
}



int HashratHashSingleFile(HashratCtx *Ctx, const char *HashType, int FileType, const char *Path, struct stat *FStat, char **RetStr)
{
HASH *Hash;
struct stat XattrStat;
const char *ptr;

		*RetStr=CopyStr(*RetStr,"");

		#ifdef USE_XATTR
		if (Ctx->Flags & CTX_XATTR_CACHE)
		{
			XAttrGetHash(Ctx, "user", Ctx->HashType, Path, &XattrStat, RetStr);
			//only use the hash cached in the xattr address if it's younger than the mtime
printf("cache %llu %llu %llu\n",XattrStat.st_mtime, FStat->st_mtime, XattrStat.st_mtime - FStat->st_mtime);
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

			if (! HashratHashFile(Ctx,Hash,FileType,Path, FStat)) return(FALSE);

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
HASH *Hash;

		if (! Ctx->Hash) 
		{
			Hash=HashInit(Ctx->HashType);
			if (Hash)
			{
			ptr=GetVar(Ctx->Vars,"EncryptionKey");
			if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

			ptr=GetVar(Ctx->Vars, "InputPrefix");
			if (StrValid(ptr)) Hash->Update(Hash ,ptr, StrLen(ptr));
			Hash->Update(Hash ,Data, DataLen);
			HashratFinishHash(RetStr, Ctx, Hash);
			}
		} 
		else Ctx->Hash->Update(Ctx->Hash ,Data, DataLen);
}


static int ConsiderItem(HashratCtx *Ctx, const char *Path, struct stat *FStat)
{
	int Type, result;
	ListNode *Items, *Node;

	Type=FileType(Path, Flags, FStat);
	result=IncludeExcludeCheck(Ctx, Path, FStat);
	if (result != CTX_INCLUDE) return(result);

	switch (Type)
	{
		case FT_HTTP:
			Type=HTTPStat(Ctx, Path, FStat);
			if ((Type==FT_DIR) && (Ctx->Flags & CTX_RECURSE)) return(CTX_HASH_AND_RECURSE);
		break;

		case FT_SSH:
			Items=ListCreate();
			Type=SSHGlob(Ctx, Path, Items);
			//if remote path is a directory, then return without bothering to update
			//any information about it from 'Items'
			if (Type==FT_DIR)
			{
			 ListDestroy(Items, Destroy);
			 return(CTX_RECURSE);
			}

			//if it's a file then update FStat with its information
			Node=ListGetNext(Items);
			if (Node) memcpy(FStat, Node->Item, sizeof(struct stat));
			ListDestroy(Items, Destroy);
		break;

		case FT_DIR:
		if (Ctx->Flags & CTX_RECURSE) return(CTX_RECURSE);
		break;
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
			if (! Ctx->Hash) 
			{
				if (! HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr)) return(FALSE);
			} else if (! HashratHashFile(Ctx, Ctx->Hash,Type,Path, FStat)) return(FALSE);
		}
		else
		{
		 	fprintf(stderr,"WARN: Not following symbolic link %s\n",Path);
			Tempstr=SetStrLen(Tempstr,PATH_MAX);
			val=readlink(Path, Tempstr,PATH_MAX);
			if (val > 0)
			{
				Tempstr[val]='\0';
				ProcessData(HashStr, Ctx, Tempstr, val);
			}
		}
		break;

		default:
		if (! Ctx->Hash) 
		{
			if (! HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr)) return(FALSE);
		} else if (! HashratHashFile(Ctx, Ctx->Hash,Type,Path, FStat)) return(FALSE);
		break;
	}

Destroy(Tempstr);

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
	if (HashratHashFile(Ctx, Ctx->Hash, Type, Path, Stat)) result=TRUE;
break;

case ACT_HASH:
	HashStartTime=GetTime(TIME_MILLISECS);
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
Destroy(HashStr);

return(result);
}


//ProcessItem returns TRUE on a significant event, so any instance of TRUE
//from items checked makes return value here TRUE
static int ProcessDir(HashratCtx *Ctx, const char *Dir)
{
char *Tempstr=NULL;
ListNode *FileList, *Curr;
int result=FALSE;
int Type;

		Type=FileType(Dir, Flags, NULL);

		FileList=ListCreate();
		GlobFiles(Ctx, Dir, Type, FileList);

		Curr=ListGetNext(FileList);
		while (Curr)
		{
			if (ProcessItem(Ctx, Curr->Tag, (struct stat *) Curr->Item, FALSE)) result=TRUE;
			Curr=ListGetNext(Curr);
		}

		ListDestroy(FileList,free);

Destroy(Tempstr);

return(result);
}


//ProcessDir returns TRUE on a significant event, so any instance of TRUE
//from items checked makes return value here TRUE
static int HashratRecurse(HashratCtx *Ctx, const char *Path, char **HashStr)
{
const char *ptr;
struct stat FStat;
int result=FALSE;

		if ((Ctx->Action == ACT_HASHDIR) && (! Ctx->Hash)) 
		{ 
				HashStartTime=GetTime(TIME_MILLISECS);
				Ctx->Hash=HashInit(Ctx->HashType);
				ptr=GetVar(Ctx->Vars,"EncryptionKey");
				if (ptr) HMACSetKey(Ctx->Hash, ptr, StrLen(ptr));

				if (! ProcessDir(Ctx, Path)) result=FALSE;
				HashratFinishHash(HashStr, Ctx, Ctx->Hash);
				stat(Path, &FStat);
				HashratOutputInfo(Ctx, Ctx->Out, Path, &FStat, *HashStr);
				result=TRUE;
		}
		else if (ProcessDir(Ctx, Path)) result=TRUE;

	return(result);
}




int ProcessItem(HashratCtx *Ctx, const char *Path, struct stat *Stat, int IsTopLevel)
{
char *HashStr=NULL;
int result=FALSE, Flags;

if (! Visited) Visited=MapCreate(1025, LIST_FLAG_CACHE);
if (! ListFindNamedItem(Visited, Path))
{
	ListAddNamedItem(Visited, Path, NULL);
	switch (ConsiderItem(Ctx, Path, Stat))
	{
		case CTX_EXCLUDE:
		case CTX_ONE_FS:
			result=IGNORE;
		break;

		case CTX_RECURSE:
			result=HashratRecurse(Ctx, Path, &HashStr);
		break;

		case CTX_HASH_AND_RECURSE:
			result=HashratAction(Ctx, Path, Stat);
			result=HashratRecurse(Ctx, Path, &HashStr);
		break;


		default:
			result=HashratAction(Ctx, Path, Stat);
		break;
	}
}

if (IsTopLevel) 
{
MapClear(Visited, NULL);	
}
Destroy(HashStr);

return(result);
}



