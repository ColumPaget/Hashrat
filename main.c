#include "common.h"
#include "ssh.h"
#include "fingerprint.h"
#include "find.h"
#include "files.h"
#include "memcached.h"
#include "check-hash.h"
#include "command-line-args.h"
#include "filesigning.h"
#include "cgi.h"

int HashFromListFile(HashratCtx *Ctx)
{
char *Tempstr=NULL, *HashStr=NULL;
const char *ptr;
STREAM *S;
struct stat Stat;
int result=FALSE;

ptr=GetVar(Ctx->Vars, "ItemsListSource");
if (
			(! StrValid(ptr)) || 
			(strcmp(ptr,"-")==0) 
) S=STREAMFromFD(0);
else S=STREAMOpen(ptr,"r");

//if we managed to open the list, return TRUE
if (S) result=TRUE;

Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
	StripTrailingWhitespace(Tempstr);
	if (StatFile(Ctx,Tempstr,&Stat) != -1) 
	{
		HashStartTime=GetTime(TIME_MILLISECS);
		if (HashItem(Ctx, Ctx->HashType, Tempstr, &Stat, &HashStr))
		{
		HashratOutputInfo(Ctx, Ctx->Out, Tempstr, &Stat, HashStr);
		HashratStoreHash(Ctx, Tempstr, &Stat, HashStr);
		}
		else 
		{
			fprintf(stderr,"ERROR: Failed to open file %s\n",Tempstr);
			result=FALSE;
		}
	}
	else 
	{
		fprintf(stderr,"ERROR: Failed to open file %s\n",Tempstr);
		result=FALSE;
	}
	Tempstr=STREAMReadLine(Tempstr, S);
}

STREAMClose(S);

Destroy(Tempstr);
Destroy(HashStr);

return(result);
}


void HashStdInOutput(HashratCtx *Ctx, const char *Hash)
{
char *Tempstr=NULL, *Base64=NULL, *Reformatted=NULL;
const char *p_Hash;

	p_Hash=Hash;
	if (Ctx->Flags & CTX_REFORMAT) 
	{
		Reformatted=ReformatHash(Reformatted, Hash, Ctx);
		p_Hash=Reformatted;
	}

	Tempstr=MCopyStr(Tempstr,p_Hash,"\n",NULL);
	STREAMWriteString(Tempstr,Ctx->Out); 


	if (Flags & FLAG_XSELECT)
	{
	Base64=EncodeBytes(Base64, p_Hash, StrLen(p_Hash), ENCODE_BASE64);
	Tempstr=FormatStr(Tempstr,"\x1b]52;cps;%s\x07",Base64);
	STREAMWriteString(Tempstr,Ctx->Out);
	}

Destroy(Tempstr);
Destroy(Base64);
}



void HashStdIn(HashratCtx *Ctx)
{
STREAM *In=NULL;
char *Tempstr=NULL, *Hash=NULL;

if (Flags & FLAG_LINEMODE) 
{
	In=STREAMFromFD(0);
	STREAMSetTimeout(In,0);

	if (Flags & FLAG_HIDE_INPUT) Tempstr=TerminalReadText(Tempstr, TERM_HIDETEXT, In);
	else if (Flags & FLAG_STAR_INPUT) Tempstr=TerminalReadText(Tempstr, TERM_SHOWSTARS, In);
	else Tempstr=STREAMReadLine(Tempstr,In);
	while (Tempstr)
	{
		if (! (Flags & FLAG_RAW)) StripTrailingWhitespace(Tempstr);
		Hash=CopyStr(Hash,"");
		ProcessData(&Hash, Ctx, Tempstr, StrLen(Tempstr));
		HashStdInOutput(Ctx, Hash);

		if (Flags & FLAG_HIDE_INPUT) Tempstr=TerminalReadText(Tempstr, TERM_HIDETEXT, In);
		else if (Flags & FLAG_STAR_INPUT) Tempstr=TerminalReadText(Tempstr, TERM_SHOWSTARS, In);
		else Tempstr=STREAMReadLine(Tempstr,In);
	}
}
else
{
	HashratHashSingleFile(Ctx, Ctx->HashType, 0, "-", NULL, &Hash);
	HashStdInOutput(Ctx, Hash);
}
STREAMDestroy(In);

Destroy(Tempstr);
Destroy(Hash);
}



int ProcessTargetItems(HashratCtx *Ctx)
{
struct stat Stat;
int result=IGNORE, type;
char *Item=NULL;
const char *ptr;

ptr=GetToken(Ctx->Targets, ",", &Item, GETTOKEN_QUOTES);
while(ptr)
{
	type=StatFile(Ctx, Item, &Stat);
 	if (type > -1) result=ProcessItem(Ctx, Item, &Stat, TRUE);
	else
	{
		if (result==IGNORE) result=0;
		fprintf(stderr,"ERROR: File '%s' not found\n", Item);
	}
	ptr=GetToken(ptr, ",", &Item, GETTOKEN_QUOTES);
}

DestroyString(Item);

return(result);
}



int main(int argc, char *argv[])
{
char *Tempstr=NULL;
const char *ptr;
int i, result=FALSE;
struct stat Stat;
HashratCtx *Ctx;	

time(&Now);
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
		result=ProcessTargetItems(Ctx);
		//if we didn't find anything on the command-line then read from stdin
		if (result==IGNORE) HashStdIn(Ctx);
	break;

	case ACT_HASHDIR:
		result=ProcessTargetItems(Ctx);
	break;
	
	case ACT_CHECK_LIST:
		result=CheckHashesFromList(Ctx);
	break;

	case ACT_HASH_LISTFILE:
		HashFromListFile(Ctx);
	break;

	case ACT_CHECK:
	case ACT_FINDMATCHES:
		if (! MatchesLoad(0))
		{
			printf("No hashes supplied on stdin.\n");
			exit(1);
		}
		result=ProcessTargetItems(Ctx);
		if (Ctx->Action==ACT_CHECK) OutputUnmatched(Ctx);
	break;

	//Load matches to an mcached server
	case ACT_LOADMATCHES:
		ptr=GetVar(Ctx->Vars, "Memcached:Server");
		if (StrEnd(ptr)) printf("ERROR: No memcached server specified. Use the -memcached <server> command-line argument to specify one\n");
		else MatchesLoad(FLAG_MEMCACHED);
	break;

	case ACT_FINDMATCHES_MEMCACHED:
	case ACT_FINDDUPLICATES:
		result=ProcessTargetItems(Ctx);
	break;

	case ACT_CHECK_XATTR:
		result=ProcessTargetItems(Ctx);
	break;

	case ACT_CHECK_MEMCACHED:
		result=ProcessTargetItems(Ctx);
	break;

	case ACT_CGI:
		CGIDisplayPage();
	break;

	case ACT_PRINTUSAGE:
		CommandLinePrintUsage();
	break;

	case ACT_SIGN:
		Ctx->Encoding = ENCODE_BASE64;
		for (i=1; i < argc; i++)
		{
		if (StrValid(argv[i]))
		{
				if (StatFile(Ctx, argv[i],&Stat) != -1)
				{
					if (S_ISLNK(Stat.st_mode)) fprintf(stderr,"WARN: Not following symbolic link %s\n",argv[i]);
					else HashratSignFile(argv[i],Ctx);
				}
				else fprintf(stderr,"ERROR: File '%s' not found\n",argv[i]);
		}
		}
	break;

	case ACT_CHECKSIGN:
		Ctx->Encoding |= ENCODE_BASE64;
		for (i=1; i < argc; i++)
		{
		if (StrValid(argv[i]))
		{
				if (StatFile(Ctx, argv[i],&Stat) != -1)
				{
					if (S_ISLNK(Stat.st_mode)) fprintf(stderr,"WARN: Not following symbolic link %s\n",argv[i]);
					else HashratCheckSignedFile(argv[i], Ctx);
				}
				else fprintf(stderr,"ERROR: File '%s' not found\n",argv[i]);
		}
		}
	break;
}

fflush(NULL);
if (Ctx->Out) STREAMClose(Ctx->Out);
if (Ctx->Aux) STREAMClose(Ctx->Aux);

switch (Ctx->Action)
{
	case ACT_HASH:
	case ACT_HASHDIR:
	case ACT_HASH_LISTFILE:
	
	case ACT_FINDMATCHES:
	case ACT_FINDMATCHES_MEMCACHED:
	case ACT_FINDDUPLICATES:
			//result==TRUE for these means 'yes we found something' or 'yes that worked'
			if (result==TRUE) exit(0);
			else exit(1);
	break;

	case ACT_CHECK:
	case ACT_CHECK_LIST:
	case ACT_CHECK_MEMCACHED:
	case ACT_CHECK_XATTR:
			//result==TRUE for these means 'CHECK FAILED'
			if (result==TRUE) exit(1);
			else exit(0);
	break;
}
}

Destroy(Tempstr);

exit(1);
}
