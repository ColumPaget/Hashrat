#include "common.h"
#include "xattr.h"
#include "memcached.h"

int Flags=0;
char *DiffHook=NULL;
char *Key=NULL;
char *LocalHost=NULL;
ListNode *IncludeExclude=NULL;
int MatchCount=0, DiffCount=0;
time_t Now;
uint64_t HashStartTime;

const char *HashratHashTypes[]= {"md5","sha1","sha256","sha384","sha512","whirl","whirlpool","jh-224","jh-256","jh-384","jh-512",NULL};




void HashratCtxDestroy(void *p_Ctx)
{
    HashratCtx *Ctx;

    Ctx=(HashratCtx *) p_Ctx;
    STREAMClose(Ctx->Out);
    Destroy(Ctx->HashType);
    Destroy(Ctx->HashStr);
    Destroy(Ctx->Targets);
    HashDestroy(Ctx->Hash);
    free(Ctx);
}


char *ReformatHash(char *RetStr, const char *Str, HashratCtx *Ctx)
{
    int ipos=0, opos=0;
    int OutputLen;

    OutputLen=Ctx->OutputLength;
    if (OutputLen==0) OutputLen=StrLen(Str);

    RetStr=CopyStr(RetStr, GetVar(Ctx->Vars, "OutputPrefix"));
    opos=StrLen(RetStr);

    for (ipos=0; ipos < OutputLen; ipos++)
    {
        //don't increment pos, the loop does this one
        RetStr=AddCharToBuffer(RetStr, opos++, Str[ipos]);
        if (Str[ipos]=='\0') break;

        if ((Ctx->SegmentLength > 0) && (ipos > 0) && (OutputLen - ipos > 1) && (((ipos+1) % Ctx->SegmentLength)==0))
        {
            RetStr=AddCharToBuffer(RetStr, opos++, Ctx->SegmentChar);
        }
    }



    return(RetStr);
}



void HashratStoreHash(HashratCtx *Ctx, const char *Path, struct stat *Stat, const char *Hash)
{
    char *Tempstr=NULL;

    //if CTX_CACHED is set, then unset. Otherwise update XATTR for this item
//	if (Ctx->Flags & CTX_CACHED) Ctx->Flags &= ~CTX_CACHED;
//	else


    if (Ctx->Flags & CTX_STORE_XATTR)
    {
        if (Ctx->Flags & CTX_XATTR_CACHE)
        {
            if ((Ctx->Flags & CTX_XATTR_ROOT) && (getuid()==0)) XAttrGetHash(Ctx, "trusted", Ctx->HashType, Path, NULL, &Tempstr);
            else XAttrGetHash(Ctx, "user", Ctx->HashType, Path, NULL, &Tempstr);
            if (strcmp(Tempstr, Hash) !=0) HashRatSetXAttr(Ctx, Path, Stat, Ctx->HashType, Hash);
        }
        else HashRatSetXAttr(Ctx, Path, Stat, Ctx->HashType, Hash);
    }

    if (Ctx->Flags & CTX_STORE_MEMCACHED)
    {
        if (Flags & FLAG_NET) Tempstr=MCopyStr(Tempstr, Path);
        else Tempstr=MCopyStr(Tempstr,"hashrat://",LocalHost,Path,NULL);
        MemcachedSet(Hash, 0, Tempstr);
        MemcachedSet(Tempstr, 0, Hash);
    }

    if (Ctx->Aux) HashratOutputFileInfo(Ctx, Ctx->Aux, Path, Stat, Hash);
    Destroy(Tempstr);
}


void RunHookScript(const char *Hook, const char *Path, const char *Other)
{
    char *Tempstr=NULL, *QuotedPath=NULL, *QuotedOther=NULL;
    STREAM *S;

    if (StrValid(Hook))
    {
        //must quote twice to get through system comamnd
        QuotedPath=QuoteCharsInStr(QuotedPath, Path,"\"'`!|;<> 	");
        QuotedOther=QuoteCharsInStr(QuotedOther, Other,"\"'`!|;<> 	");
        S=STREAMSpawnCommand("/bin/sh", "");
        if (S)
        {
            Tempstr=MCopyStr(Tempstr, DiffHook," ",QuotedPath, " ", QuotedOther, ";exit\n",NULL);
            STREAMWriteLine(Tempstr,S);
            STREAMFlush(S);

            Tempstr=STREAMReadLine(Tempstr,S);
            while (Tempstr)
            {
                printf("%s\n",Tempstr);
                Tempstr=STREAMReadLine(Tempstr,S);
            }
        }
    }

    Destroy(Tempstr);
    Destroy(QuotedPath);
    Destroy(QuotedOther);
}



