#include "check.h"


void HandleCheckFail(char *Path, char *ErrorMessage)
{
char *Tempstr=NULL;

  if (Flags & FLAG_COLOR) printf("%s%s: FAILED. '%s'.%s\n",ANSICode(ANSI_RED, 0, 0),Path, ErrorMessage, ANSI_NORM);
	else printf("%s: FAILED. %s.\n",Path,ErrorMessage);

  if (StrLen(DiffHook))
  {
    Tempstr=MCopyStr(Tempstr,DiffHook," '",Path,"'",NULL);
    system(Tempstr);
  }

  DestroyString(Tempstr);
}



int CheckStat(char *Path, struct stat *ExpectedStat, struct stat *Stat)
{
	if ((ExpectedStat->st_size > 0) && ((size_t) Stat->st_size != (size_t) ExpectedStat->st_size)) 
	{
		HandleCheckFail(Path, "Filesize changed");
		return(FALSE);
	}

	if ((Flags & FLAG_FULLCHECK) && (Stat->st_ino != ExpectedStat->st_ino))
	{
		HandleCheckFail(Path, "Inode changed");
		return(FALSE);
	}

	if ((Flags & FLAG_FULLCHECK) && (Stat->st_uid != ExpectedStat->st_uid))
	{
		HandleCheckFail(Path, "Owner changed");
		return(FALSE);
	}

	if ((Flags & FLAG_FULLCHECK) && (Stat->st_gid != ExpectedStat->st_gid))
	{
		HandleCheckFail(Path, "Group changed");
		return(FALSE);
	}

	if ((Flags & FLAG_FULLCHECK) && (Stat->st_mode != ExpectedStat->st_mode))
	{
		HandleCheckFail(Path, "Mode changed");
		return(FALSE);
	}

	if ((Flags & FLAG_FULLCHECK) && (Stat->st_mtime != ExpectedStat->st_mtime))
	{
		HandleCheckFail(Path, "MTime changed");
		return(FALSE);
	}

		return(TRUE);
}


int CheckFileHash(HashratCtx *Ctx, char *Path, struct stat *Stat, char *ExpectedHash) 
{
char *HashStr=NULL;
int result=FALSE;

	HashStr=CopyStr(HashStr,"");
	HashItem(Ctx, Path, Stat, &HashStr);
	if (strcasecmp(ExpectedHash,HashStr)!=0) HandleCheckFail(Path, "Hash mismatch");
	else
	{
		if (! (Flags & FLAG_OUTPUT_FAILS))
		{
		if (Flags & FLAG_COLOR) printf("%s%s: OKAY%s\n",ANSICode(ANSI_GREEN, 0, 0), Path, ANSI_NORM);
		else printf("%s: OKAY\n",Path);
		}
		result=TRUE;
	}

	if (Flags & FLAG_UPDATE) HashratStoreHash(Ctx, Path, &Stat, HashStr);

DestroyString(HashStr);
return(result);
}


int HashratCheckFile(HashratCtx *Ctx, char *Path, char *ExpectedHash, struct stat *ExpectedStat)
{
int result=FALSE;
struct stat Stat;
int enc;


    //Set encoding from args
    if (Ctx->Flags & CTX_BASE8) enc = ENCODE_OCTAL;
    else if (Ctx->Flags & CTX_BASE10) enc = ENCODE_DECIMAL;
    else if (Ctx->Flags & CTX_HEXUPPER) enc = ENCODE_HEXUPPER;
    else if (Ctx->Flags & CTX_BASE64) enc = ENCODE_BASE64;
    else enc= ENCODE_HEX;

		if (access(Path,F_OK)!=0) fprintf(stderr,"\rERROR: No such file '%s'\n",Path);
		else
		{
		StatFile(Ctx,Path,&Stat);
		if (Flags & FP_HASSTAT)
		{
				if (CheckStat(Path, ExpectedStat, &Stat)) result=CheckFileHash(Ctx, Path, &Stat, ExpectedHash); 
		}
		else result=CheckFileHash(Ctx, Path, &Stat, ExpectedHash); 
		}

return(result);
}


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

FP=FingerprintRead(ListStream);
while (FP)
{
  if (StrLen(FP->HashType)) Ctx->HashType=CopyStr(Ctx->HashType, FP->HashType);
  if (! HashratCheckFile(Ctx, FP->Path, FP->Hash, &FP->FStat)) Errors++;
  FingerprintDestroy(FP);
  FP=FingerprintRead(ListStream);
  Checked++;
}

fprintf(stderr,"\nChecked %d files. %d Failures\n",Checked,Errors);


return(result);
}

