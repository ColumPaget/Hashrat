#include "files.h"
#include "ssh.h"
#include "memcached.h"
#include "fingerprint.h"
#include "xattr.h"
#include <string.h>


dev_t StartingFS=0;
ListNode *IncludeExclude=NULL;




//if first item added to Include/Exclude is an include
//then the program will exclude by default
void AddIncludeExclude(int Type, const char *Item)
{
ListNode *Node;

if (! IncludeExclude)
{
  IncludeExclude=ListCreate();
  if (Type==FLAG_INCLUDE) Flags |= FLAG_EXCLUDE;
}

Node=ListAddItem(IncludeExclude, CopyStr(NULL, Item));
Node->ItemType=Type;
}



int FileType(char *Path, int FTFlags, struct stat *Stat)
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


int IsIncluded(char *Path, struct stat *FStat)
{
ListNode *Curr;
char *mptr, *dptr;
int result=TRUE;

if (Flags & FLAG_EXCLUDE) result=FALSE;
if (S_ISDIR(FStat->st_mode)) result=TRUE;

Curr=ListGetNext(IncludeExclude);
while (Curr)
{
mptr=(char *) Curr->Item;
dptr=Path;
if (*mptr!='/') 
{
mptr=GetBasename(mptr);
dptr=GetBasename(Path);
}

if (fnmatch(mptr,dptr,0)==0) 
{
	if (Curr->ItemType==FLAG_INCLUDE) result=TRUE;
	else result=FALSE;
}

Curr=ListGetNext(Curr);
}


return(result);
}




int StatFile(HashratCtx *Ctx, char *Path, struct stat *Stat)
{
int val;

  //Pass NULL for stat, because we are only checking for 'net' type paths
  val=FileType(Path,Flags, NULL);
  if ((val !=FT_FILE) && (val !=FT_DIR))
  {
    return(0);
  }

  if (Ctx->Flags & CTX_DEREFERENCE) val=stat(Path,Stat);
  else val=lstat(Path,Stat);

  return(val);
}




int GlobFiles(HashratCtx *Ctx, char *Path, int FType, ListNode *Dirs)
{
char *Tempstr=NULL;
glob_t Glob;
int i, count=0, val;
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






int HashratHashFile(HashratCtx *Ctx, THash *Hash, int Type, char *Path, off_t FileSize)
{
STREAM *S;
char *Tempstr=NULL, *User=NULL, *Pass=NULL;
int result, val, RetVal=FALSE;
off_t bytes_read=0;

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
		if ((! StrLen(Path)) || (strcmp(Path,"-")==0)) S=STREAMFromFD(0);
		else S=STREAMOpenFile(Path,O_RDONLY);
	break;
}


if (S) 
{
Tempstr=SetStrLen(Tempstr,4096);

val=FileSize;
if (val > 4096) val=4096;
result=STREAMReadBytes(S,Tempstr,val);
while (result > 0)
{
	Tempstr[result]='\0';

	if (result > 0) Hash->Update(Hash ,Tempstr, result);
	bytes_read+=result;
	if ((Type != FT_HTTP) && (bytes_read >= FileSize)) break;

	val=FileSize - bytes_read;
	if (val > 4096) val=4096;
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
char *ptr;

			//Set encoding from args
		if (Ctx->Flags & CTX_BASE8) val = ENCODE_OCTAL;
		else if (Ctx->Flags & CTX_BASE10) val = ENCODE_DECIMAL;
		else if (Ctx->Flags & CTX_HEXUPPER) val = ENCODE_HEXUPPER;
		else if (Ctx->Flags & CTX_BASE64) val = ENCODE_BASE64;
		else val= ENCODE_HEX;

		Hash->Finish(Hash,val,RetStr);

		ptr=GetVar(Ctx->Vars,"Output:Length");
		if (StrLen(ptr))
		{
			val=atoi(ptr);
			if ((val > 0) && (StrLen(*RetStr) > val)) (*RetStr)[val]='\0';
		}
		HashDestroy(Hash);
}



int HashratHashSingleFile(HashratCtx *Ctx, char *HashType, int FileType, char *Path, struct stat *FStat, char **RetStr)
{
THash *Hash;
char *ptr;

		*RetStr=CopyStr(*RetStr,"");
		Hash=HashInit(HashType);

		//If we're not doing HMAC then this doesn't do anything
		ptr=GetVar(Ctx->Vars,"EncryptionKey");
		if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

		if (! HashratHashFile(Ctx,Hash,FileType,Path, FStat->st_size)) return(FALSE);

		HashratFinishHash(RetStr, Ctx, Hash);

		return(TRUE);
}



//This is used to processs small pieces of data like device IDs
void ProcessData(char **RetStr, HashratCtx *Ctx, char *Data, int DataLen)
{
char *ptr;
THash *Hash;

		if (! Ctx->Hash) 
		{
			Hash=HashInit(Ctx->HashType);
			ptr=GetVar(Ctx->Vars,"EncryptionKey");
			if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

			Hash->Update(Hash ,Data, DataLen);
			HashratFinishHash(RetStr, Ctx, Hash);
		} 
		else Hash->Update(Ctx->Hash ,Data, DataLen);
}


int ConsiderItem(HashratCtx *Ctx, char *Path, struct stat *FStat)
{
	int Type;

	Type=FileType(Path, Flags, FStat);
	switch (Type)
	{
		case FT_SSH:
			if (SSHGlob(Ctx, Path, NULL) > 1) return(FLAG_RECURSE);
		break;

		case FT_DIR:
		if (Flags & FLAG_RECURSE) return(FLAG_RECURSE);
		break;
	}

	if (Flags & FLAG_ONE_FS)
	{
		if (StartingFS==0) StartingFS=FStat->st_dev;
		else if (FStat->st_dev != StartingFS) return(FLAG_ONE_FS);
	}
	if ((Ctx->Flags & CTX_EXES) && (! (FStat->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))) return(FLAG_EXCLUDE);
	if (! IsIncluded(Path, FStat)) return(FLAG_EXCLUDE);


	return(0);
}


int HashItem(HashratCtx *Ctx, char *HashType, char *Path, struct stat *FStat, char **HashStr)
{
int result=TRUE, Type=FT_FILE, val;
char *Tempstr=NULL;

	Type=FileType(Path, Flags, FStat);

	switch (Type)
	{
		case FT_HTTP:
			HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr);
			return(0);
		break;

		case FT_SSH:
			HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr);
			return(0);
		break;

		case FT_DIR:
		if (Flags & FLAG_RECURSE) return(FLAG_RECURSE);
		else ProcessData(HashStr, Ctx, (char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_CHR:
			ProcessData(HashStr, Ctx, (char *) &FStat->st_rdev, sizeof(dev_t));
		break;

		case FT_BLK:
			ProcessData(HashStr, Ctx, (char *) &FStat->st_rdev, sizeof(dev_t));
		break;

		case FT_FIFO:
			ProcessData(HashStr, Ctx, (char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_SOCK:
		 ProcessData(HashStr, Ctx, (char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_LNK:
		Tempstr=SetStrLen(Tempstr,PATH_MAX);
		val=readlink(Path, Tempstr,PATH_MAX);
		if (val > 0)
		{
			Tempstr[val]='\0';
			ProcessData(HashStr, Ctx, Tempstr, val);
		}
		break;

		default:
		if (! Ctx->Hash) 
		{
			if (! HashratHashSingleFile(Ctx, HashType, Type, Path, FStat, HashStr)) return(FLAG_ERROR);
		} else if (! HashratHashFile(Ctx, Ctx->Hash,Type,Path, FStat->st_size)) return(FLAG_ERROR);
		break;
	}

DestroyString(Tempstr);

return(0);
}




void HashratAction(HashratCtx *Ctx, char *Path, struct stat *Stat)
{
char *HashStr=NULL;
TFingerprint *FP;
int FType;

switch (Ctx->Action)
{
case ACT_HASH:
	HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr);
	HashratOutputInfo(Ctx, Ctx->Out, Path, Stat, HashStr);
	HashratStoreHash(Ctx, Path, Stat, HashStr);
break;

case ACT_CHECK_XATTR:
	if (S_ISREG(Stat->st_mode))
	{
		FP=XAttrLoadHash(Ctx, Path);
		if (FP) 
		{
			HashItem(Ctx, FP->HashType, Path, Stat, &HashStr);
			if (FP->Flags & FP_HASSTAT) HashratCheckFile(Ctx, Path, &FP->FStat, Stat, FP->Hash, HashStr);
			else HashratCheckFile(Ctx, Path, NULL, Stat, FP->Hash, HashStr);
		}
		else fprintf(stderr,"ERROR: No stored hash for '%s'\n",Path);
	}
break;

case ACT_CHECK_MEMCACHED:
	if (S_ISREG(Stat->st_mode))
	{
		if (Stat->st_size > 0)
		{
		HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr);
		FP=(TFingerprint *) calloc(1,sizeof(TFingerprint));
    if (Flags & FLAG_NET) FP->Path=MCopyStr(FP->Path, Path);
    else FP->Path=MCopyStr(FP->Path,"hashrat://",LocalHost,Path,NULL);
    FP->Hash=MemcachedGet(FP->Hash, FP->Path);

		if (FP) HashratCheckFile(Ctx, Path, NULL, NULL, FP->Hash, HashStr);
		else fprintf(stderr,"ERROR: No stored hash for '%s'\n",Path);
		FingerprintDestroy(FP);
		}
		else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
break;

case ACT_FINDMATCHES:
case ACT_FINDMATCHES_MEMCACHED:
	if (S_ISREG(Stat->st_mode))
	{
		if (Stat->st_size > 0)
		{
		HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr);
		FP=CheckForMatch(Ctx, Path, Stat, HashStr);
		if (FP) printf("LOCATED: %s %s %s\n",FP->Hash,FP->Path,FP->Data);
		TFingerprintDestroy(FP);	
		}
		else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
break;

case ACT_FINDDUPLICATES:
	if (S_ISREG(Stat->st_mode))
	{
	if (Stat->st_size > 0)
	{
		if (HashItem(Ctx, Ctx->HashType, Path, Stat, &HashStr) != FLAG_ERROR)
		{
			FP=CheckForMatch(Ctx, Path, Stat, HashStr);
			if (FP) printf("DUPLICATE: %s of %s %s\n",Path,FP->Path,FP->Data);
			else MatchAdd(HashStr,Ctx->HashType,"", Path);
			TFingerprintDestroy(FP);	
		}
	}
	else if (Flags & FLAG_VERBOSE) fprintf(stderr,"ZERO LENGTH FILE: %s\n",Path);
	}
break;
}

DestroyString(HashStr);
}



int ProcessDir(HashratCtx *Ctx, char *Dir, char *HashType)
{
char *Tempstr=NULL, *HashStr=NULL;
STREAM *S=NULL;
ListNode *FileList, *Curr;
int i, Type;
int result=TRUE;

		Type=FileType(Dir, Flags, NULL);

		S=Ctx->Out;

		FileList=ListCreate();
		i=GlobFiles(Ctx, Dir, Type, FileList);

		Curr=ListGetNext(FileList);
		while (Curr)
		{
			ProcessItem(Ctx, Curr->Tag, (struct stat *) Curr->Item);
			Curr=ListGetNext(Curr);
		}

		ListDestroy(FileList,free);

DestroyString(Tempstr);
DestroyString(HashStr);

return(result);
}



int HashratRecurse(HashratCtx *Ctx, char *Path, char **HashStr, int result)
{
char *ptr;
THash *Hash;


		if ((Flags & FLAG_DIRMODE) && (! Ctx->Hash)) 
		{
				Hash=HashInit(Ctx->HashType);
				ptr=GetVar(Ctx->Vars,"EncryptionKey");
				if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

				if (! ProcessDir(Ctx, Path, Ctx->HashType)) result=FALSE;
				HashratFinishHash(HashStr, Ctx, Hash);
		}
		else if (! ProcessDir(Ctx, Path, Ctx->HashType)) result=FALSE;

	return(result);
}




void ProcessItem(HashratCtx *Ctx, char *Path, struct stat *Stat)
{
char *HashStr=NULL;

				switch (ConsiderItem(Ctx, Path, Stat))
				{
					case FLAG_EXCLUDE:
					case FLAG_ONE_FS:
					break;

					case FLAG_RECURSE:
					HashratRecurse(Ctx, Path, &HashStr, 0);
					break;

					default:
					HashratAction(Ctx, Path, Stat);
					break;
				}

DestroyString(HashStr);
}



