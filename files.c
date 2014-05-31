#include "files.h"
#include "ssh.h"
#include <string.h>


dev_t StartingFS=0;
ListNode *IncludeExclude=NULL;

#define FT_FILE 0
#define FT_DIR 1
#define FT_LNK 2
#define FT_BLK 4
#define FT_CHR 8
#define FT_SOCK 16
#define FT_FIFO 32
#define FT_STDIN 64
#define FT_HTTP 256
#define FT_SSH  512





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
//		if (! Stat) printf("\nNOSTAT %s\n", Path);
//		else printf("NET: %d %s\n",Stat->st_mode,Path);
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
mptr=basename(mptr);
dptr=basename(Path);
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




int StatFile(char *Path, struct stat *Stat)
{
int val;

  //Pass NULL for stat, because we are only checking for 'net' type paths
  val=FileType(Path,Flags, NULL);
  if ((val !=FT_FILE) && (val !=FT_DIR))
  {
    return(0);
  }

  if (Flags & FLAG_DEREFERENCE) val=stat(Path,Stat);
  else val=lstat(Path,Stat);

  return(val);
}




int GlobFiles(HashratCtx *Ctx, char *Path, int FType, ListNode *Dirs)
{
char *Tempstr=NULL;
glob_t Glob;
int i, count=0, val;
struct stat *Stat;

//printf("GLIB: %d %s\n",FType,Path);
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
			if (StatFile(Path, Stat) != -1) ListAddNamedItem(Dirs, Glob.gl_pathv[i], Stat);
			else free(Stat);
		}
		count++;
	}
}

DestroyString(Tempstr);
globfree(&Glob);

return(count);
}






int HashratHashFile(HashratCtx *Ctx, THash *Hash, int Type, char *Path, struct stat *FStat)
{
STREAM *S;
char *Tempstr=NULL, *User=NULL, *Pass=NULL;
int result, bytes_read=0;

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

result=STREAMReadBytes(S,Tempstr,4096);
while (result > 0)
{
	Tempstr[result]='\0';
	if (result > 0) Hash->Update(Hash ,Tempstr, result);
	bytes_read+=result;
	if (bytes_read >= FStat->st_size) break;
	result=STREAMReadBytes(S,Tempstr,4096);
}

if (Type != FT_SSH) STREAMClose(S);
}

DestroyString(Tempstr);
DestroyString(User);
DestroyString(Pass);

//for now we close any connection after a 'get'
//Ctx->NetCon=NULL;

return(TRUE);
}



char *HashratHashSingleFile(char *RetStr, HashratCtx *Ctx,int Type,char *Path, struct stat *FStat)
{
THash *Hash;
char *ptr;

			RetStr=CopyStr(RetStr,"");
			Hash=HashInit(Ctx->HashType);

			//If we're not doing HMAC then this doesn't do anything
			ptr=GetVar(Ctx->Vars,"EncryptionKey");
			if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

			HashratHashFile(Ctx,Hash,Type,Path, FStat);
			Hash->Finish(Hash,Ctx->Encoding,&RetStr);
			HashDestroy(Hash);

	return(RetStr);
}



//This is used to processs small pieces of data like device IDs
char *ProcessData(char *RetStr, HashratCtx *Ctx, char *Data, int DataLen)
{
char *ptr;
THash *Hash;

		if (! Ctx->Hash) 
		{
			Hash=HashInit(Ctx->HashType);
			ptr=GetVar(Ctx->Vars,"EncryptionKey");
			if (ptr) HMACSetKey(Hash, ptr, StrLen(ptr));

			Hash->Update(Hash ,Data, DataLen);
			Hash->Finish(Hash,Ctx->Encoding,&RetStr);
			HashDestroy(Hash);
		} else Hash->Update(Ctx->Hash ,Data, DataLen);

return(RetStr);
}


int HashItem(HashratCtx *Ctx, char *Path, struct stat *FStat, char **HashStr)
{
int result=TRUE, Type=FT_FILE, val;
char *Tempstr=NULL;

	Type=FileType(Path, Flags, FStat);

	switch (Type)
	{
			case FT_HTTP:
			*HashStr=HashratHashSingleFile(*HashStr, Ctx, Type, Path, FStat);
			return(0);
			break;

			case FT_SSH:
				val=SSHGlob(Ctx, Path, NULL);
				if (val > 1) return(FLAG_RECURSE);
			*HashStr=HashratHashSingleFile(*HashStr, Ctx, Type, Path, FStat);
			return(0);
			break;
	}

	if (Flags & FLAG_ONE_FS)
	{
		if (StartingFS==0) StartingFS=FStat->st_dev;
		else if (FStat->st_dev != StartingFS) return(FLAG_ONE_FS);
	}

	if (! IsIncluded(Path, FStat)) return(FLAG_EXCLUDE);

	switch (Type)
	{
		case FT_DIR:
		if (Flags & FLAG_RECURSE) return(FLAG_RECURSE);
		else *HashStr=ProcessData(*HashStr, Ctx, (char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_CHR:
			*HashStr=ProcessData(*HashStr, Ctx, (char *) &FStat->st_rdev, sizeof(dev_t));
		break;

		case FT_BLK:
			*HashStr=ProcessData(*HashStr, Ctx, (char *) &FStat->st_rdev, sizeof(dev_t));
		break;

		case FT_FIFO:
			*HashStr=ProcessData(*HashStr, Ctx, (char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_SOCK:
		 *HashStr=ProcessData(*HashStr, Ctx, (char *) &FStat->st_ino, sizeof(ino_t));
		break;

		case FT_LNK:
		Tempstr=SetStrLen(Tempstr,PATH_MAX);
		val=readlink(Path, Tempstr,PATH_MAX);
		if (val > 0)
		{
			Tempstr[val]='\0';
			*HashStr=ProcessData(*HashStr, Ctx, Tempstr, val);
		}
		break;

		default:
		if (! Ctx->Hash) 
		{
			*HashStr=HashratHashSingleFile(*HashStr, Ctx, Type, Path, FStat);
		} else HashratHashFile(Ctx, Ctx->Hash,Type,Path, FStat);
		break;
	}

DestroyString(Tempstr);

return(0);
}





int ProcessDir(HashratCtx *Ctx, char *Dir, char *HashType)
{
char *Tempstr=NULL, *HashStr=NULL;
STREAM *S=NULL;
ListNode *FileList, *Curr;
int i, Type;
int result=TRUE;

		Type=FileType(Dir, Flags, NULL);

		if (Flags & FLAG_DIR_INFO)
		{
			Tempstr=MCopyStr(Tempstr,Dir,"/.fdigest.info",NULL);
			S=STREAMOpenFile(Tempstr,O_WRONLY|O_CREAT|O_TRUNC);	
		}
		else S=Ctx->Out;


		FileList=ListCreate();
		i=GlobFiles(Ctx, Dir, Type, FileList);

		Curr=ListGetNext(FileList);
		while (Curr)
		{
			ProcessItem(Ctx, Curr->Tag, (struct stat *) Curr->Item);
			//else fprintf(stderr,"\rERROR: Failed to open '%s'.\n",(char *) Curr->Item);
			Curr=ListGetNext(Curr);
		}

		if (Flags & FLAG_DIR_INFO) STREAMClose(S);

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
				Hash->Finish(Hash,Ctx->Encoding,HashStr);
				HashDestroy(Hash);
		}
		else if (! ProcessDir(Ctx, Path, Ctx->HashType)) result=FALSE;

	return(result);
}




void ProcessItem(HashratCtx *Ctx, char *Path, struct stat *Stat)
{
char *HashStr=NULL;

				switch (HashItem(Ctx, Path, Stat, &HashStr))
				{
					case FLAG_EXCLUDE:
					break;


					case FLAG_RECURSE:
					HashratRecurse(Ctx, Path, &HashStr, 0);
					break;

					default:
					switch (Ctx->Action)
					{
					default:
					HashratAction(Ctx, Path, Stat, HashStr);
					break;
					}
					break;
				}

DestroyString(HashStr);
}



