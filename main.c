#include "libUseful-2.0/libUseful.h"
#include "glob.h"


#define ACT_NONE 0
#define ACT_HASH 1
#define ACT_CHECK 2
#define ACT_HASH_LISTFILE 3
#define ACT_PRINTUSAGE 4

#define FLAG_RECURSE 1
#define FLAG_VERBOSE 2
#define FLAG_DIRMODE 4
#define FLAG_DEVMODE 8
#define FLAG_CHECK   16
#define FLAG_STDIN	 32
#define FLAG_ONE_FS  64
#define FLAG_DIR_INFO 128
#define FLAG_XATTR   256
#define FLAG_FROM_LISTFILE 512
#define FLAG_OUTPUT_FAILS 1024 
#define FLAG_TRAD_OUTPUT 2048 
#define FLAG_INCLUDE 4096
#define FLAG_EXCLUDE 8192
#define FLAG_DEREFERENCE 16384
#define FLAG_HMAC 32768
#define FLAG_BASE64 65536
#define FLAG_ARG_NAMEVALUE 131072


#define BLOCKSIZE 4096

#define VERSION "0.1"

ListNode *Vars;


typedef struct
{
char *Path;
char *Hash;
char *HashType;
int Size;
} TFingerprint;

ListNode *IncludeExclude=NULL;
dev_t StartingFS=0;
int Flags=0;
int Encoding=ENCODE_HEX;
char *DiffHook=NULL;
char *Key=NULL;


#define USE_XATTR

#ifdef USE_XATTR
 #include <sys/types.h>
 #include <sys/xattr.h>
#endif


int StatFile(char *Path, struct stat *Stat)
{
if (Flags & FLAG_DEREFERENCE) return(stat(Path,Stat));
return(lstat(Path,Stat));
}


int FingerprintRead(STREAM *S, TFingerprint *FP)
{
char *Tempstr=NULL, *Name=NULL, *Value=NULL, *ptr;

FP->Path=CopyStr(FP->Path,"");
FP->Hash=CopyStr(FP->Hash,"");
FP->Size=0;
Tempstr=STREAMReadLine(Tempstr,S);
if (! Tempstr) return(FALSE);

StripTrailingWhitespace(Tempstr);
if (strncmp(Tempstr,"path=",5) ==0)
{
//Native format

	ptr=GetNameValuePair(Tempstr," ","=",&Name,&Value);
	while (ptr)
	{
		if (StrLen(Name))
		{
		if (strcmp(Name,"path")==0) FP->Path=CopyStr(FP->Path,Value);
		if (strcmp(Name,"size")==0) FP->Size=atoi(Value);
		if (strcmp(Name,"hash")==0)
		{
			 FP->Hash=CopyStr(FP->Hash,GetToken(Value,":",&FP->HashType,0));
		}
		}
		ptr=GetNameValuePair(ptr," ","=",&Name,&Value);
	}

}
else
{
	ptr=GetToken(Tempstr," ",&FP->Hash,0);
	while (isspace(*ptr)) ptr++;
	FP->Path=CopyStr(FP->Path,ptr);
}

DestroyString(Tempstr);
DestroyString(Name);
DestroyString(Value);

return(TRUE);
}




void FingerprintDestroy(void *p_FP)
{
TFingerprint *FP;

FP=(TFingerprint *) p_FP;
DestroyString(FP->Path);
DestroyString(FP->Hash);
DestroyString(FP->HashType);
free(FP);
}



int FPCompare(const void *v1, const void *v2)
{
const TFingerprint *FP1, *FP2;

FP1=(TFingerprint *) v1;
FP2=(TFingerprint *) v2;

if (! FP1->Hash) return(FALSE);
if (! FP2->Hash) return(TRUE);
if (strcmp(FP1->Hash,FP2->Hash) < 0) return(TRUE);

return(FALSE);
}


int FDigestOutputInfo(STREAM *Out, char *Path, struct stat *Stat, char *HashType, char *Hash)
{
char *Tempstr=NULL;
ListNode *Curr;

if (Flags & FLAG_XATTR) setxattr(Path, HashType, Hash, StrLen(Hash), 0);

if (Flags & FLAG_TRAD_OUTPUT) Tempstr=MCopyStr(Tempstr,Hash, "  ", Path,NULL);
else
{
	Tempstr=FormatStr(Tempstr,"path='%s' size='%ld' mode='%d' mtime='%d' ",Path,Stat->st_size,Stat->st_mode,Stat->st_mtime);
	Tempstr=MCatStr(Tempstr,"hash='",HashType,":",Hash,"' ",NULL);
}

Tempstr=CatStr(Tempstr,"\n");

STREAMWriteString(Tempstr,Out);

DestroyString(Tempstr);
}


int FDigestHashFile(THash *Hash, char *Path)
{
STREAM *S;
char *Tempstr=NULL;
int result;

if (strcmp(Path,"-")==0) S=STREAMFromFD(0);
else S=STREAMOpenFile(Path,O_RDONLY);
if (! S) return(FALSE);

Tempstr=SetStrLen(Tempstr,4096);

result=STREAMReadBytes(S,Tempstr,4096);
while (result > 0)
{
	Hash->Update(Hash ,Tempstr, result);
	result=STREAMReadBytes(S,Tempstr,4096);
}

STREAMClose(S);

DestroyString(Tempstr);


return(TRUE);
}


char *FDigestHashSingleFile(char *RetStr, char *HashType, char *Path)
{
THash *Hash;
char *ptr;

			RetStr=CopyStr(RetStr,"");
			Hash=HashInit(HashType);
			ptr=GetVar(Vars,"EncryptionKey");
			HMACSetKey(Hash, ptr, StrLen(ptr));

			FDigestHashFile(Hash,Path);
			Hash->Finish(Hash,Encoding,&RetStr);
			HashDestroy(Hash);

	return(RetStr);
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



//This is used to processs small pieces of data like device IDs
char *ProcessData(char *RetStr, THash *Hash, char *Path, char *Data, int DataLen, char *HashType, struct stat *Stat)
{
char *ptr;

		if (! Hash) 
		{
			Hash=HashInit(HashType);
			ptr=GetVar(Vars,"EncryptionKey");
			HMACSetKey(Hash, ptr, StrLen(ptr));

			Hash->Update(Hash ,Data, DataLen);
			Hash->Finish(Hash,Encoding,&RetStr);
			HashDestroy(Hash);
		} else Hash->Update(Hash ,Data, DataLen);

return(RetStr);
}



int HashItem(THash *Hash, char *Path, struct stat *FStat, char *HashType, char **HashStr)
{
char *Tempstr=NULL;
int result=TRUE, val;


	if (Flags & FLAG_ONE_FS)
	{
		if (StartingFS==0) StartingFS=FStat->st_dev;
		else if (FStat->st_dev != StartingFS) return(FLAG_ONE_FS);
	}

	if (! IsIncluded(Path, FStat)) return(FLAG_EXCLUDE);

	if (S_ISDIR(FStat->st_mode))
	{
		if (Flags & FLAG_RECURSE) return(FLAG_RECURSE);
	}
	else if ((! (Flags & FLAG_DEVMODE)) && S_ISCHR(FStat->st_mode))  *HashStr=ProcessData(*HashStr, Hash, Path, (char *) FStat->st_rdev, sizeof(dev_t), HashType, FStat);
	else if ((! (Flags & FLAG_DEVMODE)) && S_ISBLK(FStat->st_mode))  *HashStr=ProcessData(*HashStr, Hash, Path, (char *) FStat->st_rdev, sizeof(dev_t), HashType, FStat);
	else if ((! (Flags & FLAG_DEVMODE)) && S_ISFIFO(FStat->st_mode)) *HashStr=ProcessData(*HashStr, Hash, Path, (char *) FStat->st_rdev, sizeof(dev_t), HashType, FStat);
	else if ((! (Flags & FLAG_DEVMODE)) && S_ISSOCK(FStat->st_mode)) *HashStr=ProcessData(*HashStr, Hash, Path, (char *) FStat->st_rdev, sizeof(dev_t), HashType, FStat);
	else if ((! (Flags & FLAG_DEVMODE)) && S_ISLNK(FStat->st_mode)) 
	{
		Tempstr=SetStrLen(Tempstr,PATH_MAX);
		val=readlink(Path, Tempstr,PATH_MAX);
		if (val > 0)
		{
			Tempstr[val]='\0';
			*HashStr=ProcessData(*HashStr, Hash, Path, Tempstr, val, HashType, FStat);
		}
	}
	else
	{
		if (! Hash) 
		{
			*HashStr=FDigestHashSingleFile(*HashStr, HashType, Path);
		} else FDigestHashFile(Hash,Path);

	}

DestroyString(Tempstr);

return(0);
}




void ProcessItem(STREAM *Out, THash *Hash, char *Path, struct stat *Stat, char *HashType)
{
char *HashStr=NULL;

				switch (HashItem(Hash, Path, Stat, HashType, &HashStr))
				{
					case FLAG_RECURSE:
					FDigestRecurse(Out, Hash, HashType, Path, &HashStr, 0);
					break;

					case FLAG_EXCLUDE:
					break;

					default:
					FDigestOutputInfo(Out, Path, Stat, HashType, HashStr);
					break;
				}

DestroyString(HashStr);
}



int ProcessDir(THash *Hash, char *Dir, char *HashType, int Flags, STREAM *Output)
{
char *Tempstr=NULL, *HashStr=NULL;
STREAM *S=NULL;
glob_t Glob;
int i;
int result=TRUE;
struct stat Stat;

			if (strcmp(Dir,".")==0) return(TRUE);
			if (strcmp(Dir,"..")==0) return(TRUE);


			if (Flags & FLAG_DIR_INFO)
			{
				Tempstr=MCopyStr(Tempstr,Dir,"/.fdigest.info",NULL);
				S=STREAMOpenFile(Tempstr,O_WRONLY|O_CREAT|O_TRUNC);	
			}
			else S=Output;

			Tempstr=MCopyStr(Tempstr,Dir,"/*",NULL);

			glob(Tempstr,0,0,&Glob);
			for (i=0; i < Glob.gl_pathc; i++)
			{
				if (StatFile(Glob.gl_pathv[i],&Stat)==0) ProcessItem(S, Hash, Glob.gl_pathv[i], &Stat, HashType);
				else fprintf(stderr,"\rERROR: Failed to open '%s'\n",Glob.gl_pathv[i]);
			}
			globfree(&Glob);

			if (Flags & FLAG_DIR_INFO) STREAMClose(S);


DestroyString(Tempstr);
DestroyString(HashStr);

return(result);
}



int FDigestRecurse(STREAM *Out, THash *Hash, char *HashType, char *Path, char **HashStr, int result)
{
char *ptr;

		if ((Flags & FLAG_DIRMODE) && (! Hash)) 
		{
				Hash=HashInit(HashType);
				ptr=GetVar(Vars,"EncryptionKey");
				HMACSetKey(Hash, ptr, StrLen(ptr));

				if (! ProcessDir(Hash, Path, HashType, Flags, Out)) result=FALSE;
				Hash->Finish(Hash,Encoding,HashStr);
				HashDestroy(Hash);

				FDigestOutputInfo(Out, Path, NULL, HashType, HashStr);
		}
		else if (! ProcessDir(Hash, Path, HashType, Flags,Out)) result=FALSE;

	return(result);
}


/*
*/



int CheckHashes(ListNode *Vars, char *DefaultHashType)
{
char *HashStr=NULL, *Tempstr=NULL,  *ptr;
int result=TRUE, i;
int Checked=0, Errors=0;
struct stat Stat;
STREAM *ListStream;
TFingerprint *FP;


ptr=GetVar(Vars,"Path");
if (strcmp(ptr,"-")==0) 
{
	ListStream=STREAMFromFD(0);
	STREAMSetTimeout(ListStream,0);
}
else ListStream=STREAMOpenFile(ptr, O_RDONLY);

FP=(TFingerprint *) calloc(1,sizeof(TFingerprint));
while (FingerprintRead(ListStream, FP))
{
		Checked++;
		HashStr=CopyStr(HashStr,"");
		if (! StrLen(FP->HashType)) FP->HashType=CopyStr(FP->HashType, DefaultHashType);
		if (StatFile(FP->Path,&Stat)==0)
		{
			if ((FP->Size > 0) && (Stat.st_size != FP->Size))
			{
				printf("%s FAILED. Filesize mismatch.\n",FP->Path);
				Errors++;
				result=FALSE;
				if (StrLen(DiffHook))
				{
					Tempstr=MCopyStr(Tempstr,DiffHook," ",FP->Path,NULL);
					system(Tempstr);
				}
			}
			else
			{
			HashFile(&HashStr,FP->HashType,FP->Path,Encoding);
			if (strcasecmp(FP->Hash,HashStr)!=0)
			{
				Errors++;
				printf("%s FAILED\n",FP->Path);
				result=FALSE;
				if (StrLen(DiffHook))
				{
					Tempstr=MCopyStr(Tempstr,DiffHook," ",FP->Path,NULL);
					system(Tempstr);
				}
			}
			else if (! (Flags & FLAG_OUTPUT_FAILS)) printf("%s OKAY\n",FP->Path);
			}
		}
		else fprintf(stderr,"\rERROR: Failed to open '%s'\n",FP->Path);
}

fprintf(stderr,"\nChecked %d files. %d Failures\n",Checked,Errors);

DestroyString(Tempstr);
DestroyString(HashStr);
FingerprintDestroy(FP);

return(result); 
}



void HashFromListFile(char *HashType, char *ListFilePath)
{
char *Tempstr=NULL, *HashStr=NULL;
STREAM *S, *Out;
struct stat Stat;

if (strcmp(ListFilePath,"-")==0) S=STREAMFromFD(0);
else S=STREAMOpenFile(ListFilePath,O_RDONLY);

Out=STREAMFromFD(1);
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	if (StatFile(Tempstr,&Stat)==0) HashItem(NULL, Tempstr, &Stat, HashType, &HashStr);
	FDigestOutputInfo(Out, Tempstr, &Stat, HashType, HashStr);
	Tempstr=STREAMReadLine(Tempstr, S);
}

STREAMClose(S);
STREAMClose(Out);

DestroyString(Tempstr);
DestroyString(HashStr);
}



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



int HandleArg(int argc, char *argv[], int pos, int SetFlags, char *VarName, char *VarValue)
{
	Flags |= SetFlags;
	
	if (SetFlags & FLAG_ARG_NAMEVALUE)
	{
	if (argv[pos + 1] ==NULL)
	{
		printf("ERROR: The %s option requires an argument.\n",argv[pos]);
		exit(1);
	}
	else
	{
		strcpy(argv[pos],"");
		pos++;
		if (SetFlags & FLAG_INCLUDE) AddIncludeExclude(FLAG_INCLUDE, argv[pos]);
		else if (SetFlags & FLAG_EXCLUDE) AddIncludeExclude(FLAG_EXCLUDE, argv[pos]);
		else SetVar(Vars,VarName,argv[pos]);
	}
	}
	else if (StrLen(VarName)) SetVar(Vars,VarName,VarValue);

	if (argc > 0) strcpy(argv[pos],"");
return(pos);
}



int ParseArgs(int argc,char *argv[], ListNode *Vars)
{
int i;
char *ptr;
int Action=ACT_HASH;

//You never know
if (argc < 1) return(ACT_PRINTUSAGE);

//argv[0] might be full path to the program, or just its name
ptr=strrchr(argv[0],'/');
if (! ptr) ptr=argv[0];
else ptr++;


if (strcmp(ptr,"md5sum")==0) 
HandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "md5");
if (
		(strcmp(ptr,"sha1sum")==0) ||
		(strcmp(ptr,"shasum")==0) 
	) 
HandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "sha1");
if (strcmp(ptr,"sha256sum")==0) 
HandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "sha256");
if (strcmp(ptr,"sha512sum")==0) 
HandleArg(0, argv, i, FLAG_TRAD_OUTPUT, "HashType", "sha512");


for (i=1; i < argc; i++)
{
if (
		(strcmp(argv[i],"--version")==0) ||
		(strcmp(argv[i],"-version")==0)
	)
{
	printf("version: %s\n",VERSION);
	return(ACT_NONE);
}
else if (
		(strcmp(argv[i],"--help")==0) ||
		(strcmp(argv[i],"-help")==0) ||
		(strcmp(argv[i],"-?")==0)
	)
{
	return(ACT_PRINTUSAGE);
}
else if (strcmp(argv[i],"-c")==0)
{
	Action = ACT_CHECK;
	strcpy(argv[i],"");
}
else if (strcmp(argv[i],"-cf")==0)
{
	Action = ACT_CHECK;
	HandleArg(argc, argv, i, FLAG_OUTPUT_FAILS, "", "");
}
else if (strcmp(argv[i],"-diff-hook")==0) 
{
	strcpy(argv[i],"");
	i++;
	DiffHook=CopyStr(DiffHook,argv[i]);
	strcpy(argv[i],"");
}

else if (strcmp(argv[i],"-md5")==0) HandleArg(argc, argv, i, 0, "HashType", "md5");
else if (
					(strcmp(argv[i],"-sha")==0) ||
					(strcmp(argv[i],"-sha1")==0)
				) HandleArg(argc, argv, i, 0, "HashType", "sha1");
else if (
					(strcmp(argv[i],"-t")==0) ||
					(strcmp(argv[i],"-trad")==0)
				) HandleArg(argc, argv, i, FLAG_TRAD_OUTPUT, "", "");
else if (strcmp(argv[i],"-sha256")==0) HandleArg(argc, argv, i, 0, "HashType", "sha256");
else if (strcmp(argv[i],"-sha512")==0) HandleArg(argc, argv, i, 0, "HashType", "sha512");
else if (strcmp(argv[i],"-crc32")==0) HandleArg(argc, argv, i, 0, "HashType", "crc32");
else if (strcmp(argv[i],"-64")==0) HandleArg(argc, argv, i, FLAG_BASE64, "", "");
else if (strcmp(argv[i],"-base64")==0) HandleArg(argc, argv, i, FLAG_BASE64, "", "");
else if (strcmp(argv[i],"-hmac")==0) HandleArg(argc, argv, i, FLAG_HMAC | FLAG_ARG_NAMEVALUE, "EncryptionKey", "");
else if (strcmp(argv[i],"-r")==0) HandleArg(argc, argv, i, FLAG_RECURSE, "", "");
else if (strcmp(argv[i],"-f")==0) HandleArg(argc, argv, i, FLAG_FROM_LISTFILE, "", "");
else if (strcmp(argv[i],"-i")==0) HandleArg(argc, argv, i, FLAG_INCLUDE | FLAG_ARG_NAMEVALUE, "", "");
else if (strcmp(argv[i],"-x")==0) HandleArg(argc, argv, i, FLAG_EXCLUDE | FLAG_ARG_NAMEVALUE, "", "");
else if (strcmp(argv[i],"-d")==0) HandleArg(argc, argv, i, FLAG_DEREFERENCE, "", "");
else if (strcmp(argv[i],"-dirmode")==0) HandleArg(argc, argv, i, FLAG_DIRMODE | FLAG_RECURSE, "", "");
else if (strcmp(argv[i],"-devmode")==0) HandleArg(argc, argv, i, FLAG_DIRMODE | FLAG_DEVMODE, "", "");
else if (strcmp(argv[i],"-fs")==0) HandleArg(argc, argv, i, FLAG_ONE_FS, "", "");
else if (strcmp(argv[i],"-dir-info")==0) HandleArg(argc, argv, i, FLAG_DIR_INFO, "", "");
else if (strcmp(argv[i],"-xattr")==0) HandleArg(argc, argv, i, FLAG_XATTR, "", "");
}

if (Flags & FLAG_BASE64) Encoding = ENCODE_BASE64;

if ((Flags & FLAG_FROM_LISTFILE))
{
	if (Action==ACT_HASH) Action=ACT_HASH_LISTFILE;

	for (i=1; i < argc; i++)
	{
		if (StrLen(argv[i]))
		{
			SetVar(Vars,"Path",argv[i]);
			strcpy(argv[i],"");
			break;
		}
	}
}

return(Action);
}



void PrintUsage()
{
printf("Hasher: version %s\n",VERSION);
printf("Author: Colum Paget\n");
printf("Email: colums.projects@gmail.com\n");
printf("Blog:  http://idratherhack.blogspot.com\n\n");
printf("Usage:\n    hasher [options] [path to hash]...\n");
printf("\n    hasher -c [options] [input file of hashes]...\n\n");

printf("Options:\n");
printf("  %-15s %s\n","--help", "Print this help");
printf("  %-15s %s\n","-help", "Print this help");
printf("  %-15s %s\n","-?", "Print this help");
printf("  %-15s %s\n","--version", "Print program version");
printf("  %-15s %s\n","-version", "Print program version");
printf("  %-15s %s\n","-md5", "Use md5 hash algorithmn");
printf("  %-15s %s\n","-sha1", "Use sha1 hash algorithmn");
printf("  %-15s %s\n","-sha256", "Use sha256 hash algorithmn");
printf("  %-15s %s\n","-sha512", "Use sha512 hash algorithmn");
printf("  %-15s %s\n","-64", "Encode with base65 instead of hex");
printf("  %-15s %s\n","-base64", "Encode with base65 instead of hex");
printf("  %-15s %s\n","-hmac", "HMAC using specified hash algorithm");
printf("  %-15s %s\n","-r", "Recurse into directories when hashing files");
printf("  %-15s %s\n","-t", "Output hashes in traditional md5sum, shaXsum format");
printf("  %-15s %s\n","-f <listfile>", "Hash files listed in <listfile>");
printf("  %-15s %s\n","-i <pattern>", "Only hash items matching <pattern>");
printf("  %-15s %s\n","-x <pattern>", "Exclude items matching <pattern>");
printf("  %-15s %s\n","-c", "Check hashes against list from file (or stdin)");
printf("  %-15s %s\n","-cf", "Check hashes but only show failures");
printf("  %-15s %s\n","-d","dereference (follow) symlinks"); 
printf("  %-15s %s\n","-dev", "DevMode: read from a file EVEN OF IT'S A DEVNODE");
printf("  %-15s %s\n","-fs", "Stay one one file system");
}



main(int argc, char *argv[])
{
char *Tempstr=NULL, *HashStr=NULL, *HashType=NULL;
int i, Action, result=FALSE, count=0;
struct stat Stat;
STREAM *Out=NULL;

Vars=ListCreate();
SetVar(Vars,"HashType","md5");
SetVar(Vars,"Path","-");

Action=ParseArgs(argc,argv,Vars);
Out=STREAMFromFD(1);

if (Flags & FLAG_HMAC) HashType=MCopyStr(HashType,"hmac-",GetVar(Vars,"HashType"),NULL);
else HashType=CopyStr(HashType,GetVar(Vars,"HashType"));

switch (Action)
{
	case ACT_CHECK:
		Flags |= FLAG_CHECK;	
		result=CheckHashes(Vars,HashType);
	break;

	case ACT_PRINTUSAGE:
		PrintUsage();
	break;

	case ACT_HASH:
	for (i=1; i < argc; i++)
	{
	Tempstr=CopyStr(Tempstr,"");
	if (StrLen(argv[i]))
	{
			if (StatFile(argv[i],&Stat)==0)
			{
			if (S_ISLNK(Stat.st_mode)) fprintf(stderr,"WARN: Not following symbolic link %s\n",argv[i]);
			ProcessItem(Out, NULL, argv[i], &Stat, HashType);
			}
			count++;
	}
	}

	//if we didn't find anything on the command-line then read from stdin
	if (count==0)
	{
			Tempstr=FDigestHashSingleFile(Tempstr, HashType, GetVar(Vars,"Path"));
			STREAMWriteString(Tempstr,Out); STREAMWriteString("\n",Out);
	}
	break;


	case ACT_HASH_LISTFILE:
		HashFromListFile(HashType, GetVar(Vars,"Path"));
	break;

}

fflush(NULL);
STREAMClose(Out);

DestroyString(Tempstr);

if (result) exit(0);
exit(1);
}
