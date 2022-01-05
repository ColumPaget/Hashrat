#include "filesigning.h"
#include "files.h"

void HashratSignFile(char *Path, HashratCtx *Ctx)
{
    STREAM *S;
    char *Tempstr=NULL, *HashStr=NULL;
    double pos;
    HASH *Hash;

    S=STREAMOpen(Path, "rw");
    if (! S) return;


    Hash=HashInit(Ctx->HashType);
    HashratFinishHash(&HashStr, Ctx, Hash);
    pos=STREAMSeek(S,0,SEEK_END);

    Tempstr=MCopyStr(Tempstr,"hashrat-placeholder---: ",GetDateStr("%Y/%m/%d %H:%M:%S",NULL)," ",Ctx->HashType,":", HashStr,"\n",NULL);
    STREAMWriteLine(Tempstr,S);
    STREAMFlush(S);

    STREAMSeek(S,0,SEEK_SET);
    Hash=HashInit(Ctx->HashType);
//HashratHashFile(Ctx, Hash, FT_FILE, Path, (off_t) pos);
    HashratFinishHash(&HashStr, Ctx, Hash);


    Tempstr=MCopyStr(Tempstr,"hashrat-integrity-mark: ",GetDateStr("%Y/%m/%d %H:%M:%S",NULL)," ",Ctx->HashType,":", HashStr,"\n",NULL);
    STREAMSeek(S,pos,SEEK_SET);
    STREAMWriteLine(Tempstr,S);
    STREAMFlush(S);


    Destroy(Tempstr);
    Destroy(HashStr);
}



int HashratOutputSigningCheck(HashratCtx *Ctx, const char *ExpectedHash, const char *SigningLine, int LineCount)
{
    char *Token=NULL;
    const char *ptr;
    char *DateStr=NULL, *SignHash=NULL, *HashType=NULL;
    int result=FALSE;

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
            result=TRUE;
        }
        else
        {
            if (Flags & FLAG_COLOR) printf("%sIntegrity Mark FAILED at Line: %d Date: %s%s\n",ANSICode(ANSI_RED, 0, 0), LineCount, DateStr, ANSI_NORM);
            else printf("Integrity Mark FAILED at Line: %d Date: %s\n",LineCount,DateStr);
        }
    }

    Destroy(Token);
    Destroy(DateStr);
    Destroy(SignHash);
    Destroy(HashType);

    return(result);
}



int HashratCheckSignedFile(char *Path, HashratCtx *Ctx)
{
    STREAM *S;
    char *Tempstr=NULL, *HashStr=NULL;
    HASH *Hash, *tmpHash;
    int LineCount=0;

    S=STREAMOpen(Path, "rw");
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
            HashFinish(tmpHash,ENCODE_BASE64,&HashStr);

            HashratOutputSigningCheck(Ctx, HashStr, Tempstr, LineCount);
        }
        Hash->Update(Hash,Tempstr, StrLen(Tempstr));


        Tempstr=STREAMReadLine(Tempstr, S);
    }

    Destroy(Tempstr);
    Destroy(HashStr);

    return(TRUE);
}



