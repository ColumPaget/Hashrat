#include "common.h"
#include "ssh.h"
#include "fingerprint.h"


int CheckHashesFromList(HashratCtx *Ctx)
{
int result=0;
int Checked=0, Errors=0;
STREAM *ListStream;
TFingerprint *FP;
char *ptr;


ptr=GetVar(Ctx->Vars,"Path");
if (strcmp(ptr,"-")==0) 
{
	ListStream=STREAMFromFD(0);
	STREAMSetTimeout(ListStream,0);
}
else ListStream=STREAMOpenFile(ptr, O_RDONLY);

FP=(TFingerprint *) calloc(1,sizeof(TFingerprint));
while (FingerprintRead(ListStream, FP))
{
	if (! StrLen(FP->HashType)) FP->HashType=CopyStr(FP->HashType, Ctx->HashType);
	result=HashratCheckFile(Ctx, FP);
	Checked++;
}

fprintf(stderr,"\nChecked %d files. %d Failures\n",Checked,Errors);

FingerprintDestroy(FP);

return(result); 
}



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
	if (StatFile(Tempstr,&Stat)==0) HashItem(NULL, Tempstr, &Stat, Ctx->HashType, &HashStr);

	HashratAction(Ctx, Tempstr, &Stat, HashStr);
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
		Hash=ProcessData(Hash, Ctx, Tempstr, StrLen(Tempstr));
		STREAMWriteString(Hash,Ctx->Out); STREAMWriteString("\n",Ctx->Out);
		Tempstr=STREAMReadLine(Tempstr,In);
	}
}
else
{
	Hash=HashratHashSingleFile(Hash, Ctx, Ctx->HashType, 0, "-");
	STREAMWriteString(Hash,Ctx->Out); STREAMWriteString("\n",Ctx->Out);
}
STREAMDisassociateFromFD(In);

DestroyString(Tempstr);
DestroyString(Hash);
}




main(int argc, char *argv[])
{
char *Tempstr=NULL, *HashStr=NULL, *ptr;
int i, result=FALSE, count=0;
struct stat Stat;
HashratCtx *Ctx;	

memset(&Stat,0,sizeof(struct stat)); //to keep valgrind happy

Ctx=CommandLineParseArgs(argc,argv);
if (Ctx)
{
switch (Ctx->Action)
{
	case ACT_CHECK:
		Flags |= FLAG_CHECK;	
		if (Flags & FLAG_XATTR) result=CheckHashesFromXAttr(Ctx);
		else result=CheckHashesFromList(Ctx);
	break;

	case ACT_CGI:
		CGIDisplayPage();
	break;

	case ACT_PRINTUSAGE:
		CommandLinePrintUsage();
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
				ProcessItem(Ctx, argv[i], &Stat);
			}
			else fprintf(stderr,"ERROR: File '%s' not found\n",argv[i]);
			count++;
	}
	}

	//if we didn't find anything on the command-line then read from stdin
	if (count==0) HashStdIn(Ctx);

	break;


	case ACT_HASH_LISTFILE:
		HashFromListFile(Ctx);
	break;

}

fflush(NULL);
STREAMClose(Ctx->Out);
}

DestroyString(Tempstr);

if (result) exit(0);
exit(1);
}
