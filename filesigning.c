#include "filesigning.h"
#include "files.h"

void HashratSignFile(char *Path, HashratCtx *Ctx)
{
STREAM *S;
char *Tempstr=NULL, *HashStr=NULL;
double pos;
THash *Hash;

S=STREAMOpenFile(Path, O_RDWR);
if (! S) return;


Hash=HashInit(Ctx->HashType);
HashratFinishHash(&HashStr, Ctx, Hash);
pos=STREAMSeek(S,0,SEEK_END);

Tempstr=MCopyStr(Tempstr,"hashrat-placeholder---: ",GetDateStr("%Y/%m/%d %H:%M:%S",NULL)," ",Ctx->HashType,":", HashStr,"\n",NULL);
STREAMWriteLine(Tempstr,S);
STREAMFlush(S);

STREAMSeek(S,0,SEEK_SET);
Hash=HashInit(Ctx->HashType);
HashratHashFile(Ctx, Hash, FT_FILE, Path, (off_t) pos);
HashratFinishHash(&HashStr, Ctx, Hash);


Tempstr=MCopyStr(Tempstr,"hashrat-integrity-mark: ",GetDateStr("%Y/%m/%d %H:%M:%S",NULL)," ",Ctx->HashType,":", HashStr,"\n",NULL);
STREAMSeek(S,pos,SEEK_SET);
STREAMWriteLine(Tempstr,S);
STREAMFlush(S);


DestroyString(Tempstr);
DestroyString(HashStr);
}



int HashratOutputSigningCheck(HashratCtx *Ctx, const char *ExpectedHash, const char *SigningLine, int LineCount)
{
char *Token=NULL, *ptr;
char *DateStr=NULL, *SignHash=NULL, *HashType=NULL;

		ptr=GetToken(SigningLine+24," ",&Token,0);
		DateStr=MCopyStr(DateStr,Token, " ", NULL);
		ptr=GetToken(ptr," ",&Token,0);
		DateStr=CatStr(DateStr,Token);


		ptr=GetToken(ptr,":",&HashType,0);
		SignHash=CopyStr(SignHash,ptr);
		StripTrailingWhitespace(SignHash);
		if (strcmp(HashType, Ctx->HashType) !=0) 
		{
			if (Flags & FLAG_COLOR) printf("%sIntegrity Mark has wrong HashType: '%s'%s\n",ANSICode(ANSI_YELLOW, 0, 0),Token,ANSI_NORM);
			else printf("Integrity Mark has wrong HashType: '%s'\n",Token);
		}
		else 
		{
			if (strcmp(ExpectedHash, SignHash) ==0)
			{
				if (Flags & FLAG_COLOR) printf("%sIntegrity Mark OKAY at Line: %d Date: %s%s\n",ANSICode(ANSI_GREEN,0,0),LineCount,DateStr,ANSI_NORM);
				else printf("Integrity Mark OKAY at Line: %d Date: %s\n",LineCount,DateStr);
			}
			else 
			{
				if (Flags & FLAG_COLOR) printf("%sIntegrity Mark FAILED at Line: %d Date: %s%s\n",ANSICode(ANSI_RED, 0, 0), LineCount, DateStr, ANSI_NORM);
				else printf("Integrity Mark FAILED at Line: %d Date: %s\n",LineCount,DateStr);
			}
		}

	DestroyString(Token);
	DestroyString(DateStr);
	DestroyString(SignHash);
	DestroyString(HashType);
}



int HashratCheckSignedFile(char *Path, HashratCtx *Ctx)
{
STREAM *S;
char *Tempstr=NULL, *HashStr=NULL;
THash *Hash, *tmpHash;
int LineCount=0;

S=STREAMOpenFile(Path, O_RDWR);
if (! S) return(FALSE);

Hash=HashInit(Ctx->HashType);
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
	LineCount++;

	//hashrat-integrity-mark: 2014/10/29 21:05:19 md5:nTnlHmvVowFowmxXtm0uNw==
	if (strncmp(Tempstr, "hashrat-integrity-mark: ",24)==0)
	{
		tmpHash=Hash->Clone(Hash);
		tmpHash->Finish(tmpHash,ENCODE_BASE64,&HashStr);

		HashratOutputSigningCheck(Ctx, HashStr, Tempstr, LineCount);
		HashDestroy(tmpHash);
	}
  Hash->Update(Hash ,Tempstr, StrLen(Tempstr));

	
	Tempstr=STREAMReadLine(Tempstr, S);
}

DestroyString(Tempstr);
DestroyString(HashStr);

return(TRUE);
}



