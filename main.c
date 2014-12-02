#include "common.h"
#include "ssh.h"
#include "fingerprint.h"
#include "find.h"
#include "memcached.h"
#include "check.h"



void HashFromListFile(HashratCtx *Ctx)
{
char *Tempstr=NULL, *HashStr=NULL;
STREAM *S;
struct stat Stat;

if (strcmp(Ctx->ListPath,"-")==0) S=STREAMFromFD(0);
else S=STREAMOpenFile(Ctx->ListPath,O_RDONLY);
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	if (StatFile(Ctx,Tempstr,&Stat)==0) HashItem(NULL, Ctx->HashType, Tempstr, &Stat, NULL, &HashStr);

	HashratOutputInfo(Ctx, Ctx->Out, Tempstr, &Stat, HashStr);
	HashratStoreHash(Ctx, Tempstr, &Stat, HashStr);
	Tempstr=STREAMReadLine(Tempstr, S);
}

STREAMClose(S);

DestroyString(Tempstr);
DestroyString(HashStr);
}


void HMACSetup(HashratCtx *Ctx)
{
char *Tempstr=NULL, *ptr;
STREAM *S;

	ptr=GetVar(Ctx->Vars,"EncryptionKey");
	if (StrLen(ptr)==0) 
	{
		if (isatty(0)) 
		{
			write(1, "Enter HMAC Key: ",16);

			S=STREAMFromFD(0);
			Tempstr=STREAMReadLine(Tempstr,S);
			StripTrailingWhitespace(Tempstr);
			SetVar(Ctx->Vars,"EncryptionKey",Tempstr);
			ptr=Tempstr;
			STREAMDisassociateFromFD(S);			
		}

		//By now we must have an encryption key!
		if (! StrLen(ptr))
		{
			write(1,"ERROR: No HMAC Key given!\n",27);
			exit(2);
		}
	}
DestroyString(Tempstr);
}



void HashStdIn(HashratCtx *Ctx)
{
STREAM *In=NULL;
char *Tempstr=NULL, *Hash=NULL;

if (Flags & FLAG_LINEMODE) 
{
	In=STREAMFromFD(0);
	STREAMSetTimeout(In,0);
	Tempstr=STREAMReadLine(Tempstr,In);
	while (Tempstr)
	{
		if (! (Flags & FLAG_RAW)) StripTrailingWhitespace(Tempstr);
		Hash=CopyStr(Hash,"");
		ProcessData(&Hash, Ctx, Tempstr, StrLen(Tempstr));
		STREAMWriteString(Hash,Ctx->Out); STREAMWriteString("\n",Ctx->Out);
		Tempstr=STREAMReadLine(Tempstr,In);
	}
}
else
{
	HashratHashSingleFile(Ctx, Ctx->HashType, 0, "-", NULL, &Hash);
	STREAMWriteString(Hash,Ctx->Out); STREAMWriteString("\n",Ctx->Out);
}
STREAMDisassociateFromFD(In);

DestroyString(Tempstr);
DestroyString(Hash);
}



int ProcessCommandLine(HashratCtx *Ctx, int argc, char *argv[])
{
struct stat Stat;
int i, count=0;

	for (i=1; i < argc; i++)
	{
	if (StrLen(argv[i]))
	{
			if (StatFile(Ctx, argv[i],&Stat)==0)
			{
				if (S_ISLNK(Stat.st_mode)) fprintf(stderr,"WARN: Not following symbolic link %s\n",argv[i]);
				ProcessItem(Ctx, argv[i], &Stat);
			}
			else fprintf(stderr,"ERROR: File '%s' not found\n",argv[i]);
			count++;
	}
	}

return(count);
}



main(int argc, char *argv[])
{
char *Tempstr=NULL, *HashStr=NULL, *ptr;
int i, result=FALSE, count=0;
struct stat Stat;
HashratCtx *Ctx;	

Tempstr=SetStrLen(Tempstr, HOST_NAME_MAX);
gethostname(Tempstr, HOST_NAME_MAX);
LocalHost=CopyStr(LocalHost, Tempstr);
Tempstr=SetStrLen(Tempstr, HOST_NAME_MAX);
getdomainname(Tempstr, HOST_NAME_MAX);
LocalHost=MCatStr(LocalHost, ".", Tempstr, NULL);

memset(&Stat,0,sizeof(struct stat)); //to keep valgrind happy

Ctx=CommandLineParseArgs(argc,argv);
if (Ctx)
{
MemcachedConnect(GetVar(Ctx->Vars, "Memcached:Server"));

switch (Ctx->Action)
{
	case ACT_HASH:
	count=ProcessCommandLine(Ctx, argc, argv);
	//if we didn't find anything on the command-line then read from stdin
	if (count==0) HashStdIn(Ctx);
	break;

	case ACT_CHECK:
		result=CheckHashesFromList(Ctx);
	break;

	case ACT_CHECK_XATTR:
		ProcessCommandLine(Ctx, argc, argv);
	break;

	case ACT_CHECK_MEMCACHED:
		ProcessCommandLine(Ctx, argc, argv);
	break;

	case ACT_CGI:
		CGIDisplayPage();
	break;

	case ACT_PRINTUSAGE:
		CommandLinePrintUsage();
	break;


	case ACT_HASH_LISTFILE:
		HashFromListFile(Ctx);
	break;


	case ACT_SIGN:
	Ctx->Flags |= CTX_BASE64;
	for (i=1; i < argc; i++)
	{
	if (StrLen(argv[i]))
	{
			if (StatFile(Ctx, argv[i],&Stat)==0)
			{
				if (S_ISLNK(Stat.st_mode)) fprintf(stderr,"WARN: Not following symbolic link %s\n",argv[i]);
				else HashratSignFile(argv[i],Ctx);
			}
			else fprintf(stderr,"ERROR: File '%s' not found\n",argv[i]);
			count++;
	}
	}
	break;

	case ACT_CHECKSIGN:
	Ctx->Flags |= CTX_BASE64;
	for (i=1; i < argc; i++)
	{
	if (StrLen(argv[i]))
	{
			if (StatFile(Ctx, argv[i],&Stat)==0)
			{
				if (S_ISLNK(Stat.st_mode)) fprintf(stderr,"WARN: Not following symbolic link %s\n",argv[i]);
				else HashratCheckSignedFile(argv[i], Ctx);
			}
			else fprintf(stderr,"ERROR: File '%s' not found\n",argv[i]);
			count++;
	}
	}
	break;

	case ACT_FINDMATCHES:
	if (! MatchesLoad())
	{
		printf("No hashes supplied on stdin.\n");
		exit(1);
	}
	ProcessCommandLine(Ctx, argc, argv);
	break;

	case ACT_FINDMATCHES_MEMCACHED:
	ProcessCommandLine(Ctx, argc, argv);
	break;

	

	case ACT_LOADMATCHES:
	ptr=GetVar(Ctx->Vars, "Memcached:Server");
	if (StrLen(ptr) ==0) printf("ERROR: No memcached server specified. Use the -memcached <server> command-line argument to specify one\n");
	else LoadMatchesToMemcache();
	break;
}

fflush(NULL);
if (Ctx->Out) STREAMClose(Ctx->Out);
if (Ctx->Aux) STREAMClose(Ctx->Aux);
}

DestroyString(Tempstr);

if (result) exit(0);
exit(1);
}
