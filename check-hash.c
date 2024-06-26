#include "check-hash.h"
#include "fingerprint.h"
#include "files.h"


void HandleCheckFail(const char *Path, const char *ErrorMessage)
{
    char *Tempstr=NULL;

    if (Flags & FLAG_COLOR) printf("%s%s: FAILED. '%s'.%s\n",ANSICode(ANSI_RED, 0, 0),Path, ErrorMessage, ANSI_NORM);
    else printf("%s: FAILED. %s.\n",Path,ErrorMessage);

    RunHookScript(DiffHook, Path, "");
    Destroy(Tempstr);
}



int CheckStat(const char *Path, struct stat *ExpectedStat, struct stat *Stat)
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


//FP here is the expected fingerprint of the file, ActualHash and Path are
// its actual path and hash value

int CheckFileHash(HashratCtx *Ctx, const char *Path, struct stat *Stat, const char *ActualHash, TFingerprint *FP)
{
    int result=FALSE;

    if (strcasecmp(FP->Hash,ActualHash) != 0) HandleCheckFail(Path, "Hash mismatch");
    else
    {
        if (! (Flags & FLAG_OUTPUT_FAILS))
        {
            if (Flags & FLAG_COLOR) printf("%s%s: OKAY%s\n",ANSICode(ANSI_GREEN, 0, 0), Path, ANSI_NORM);
            else printf("%s: OKAY\n",Path);
        }
        result=TRUE;
    }

    if (Flags & FLAG_UPDATE) HashratStoreHash(Ctx, Path, Stat, ActualHash);

    return(result);
}


int HashratCheckFile(HashratCtx *Ctx, const char *Path, struct stat *ActualStat, const char *ActualHash, TFingerprint *FP)
{
    int result=FALSE;
    char *Tempstr=NULL;


    if (access(Path,F_OK)!=0)
    {
        //fprintf(stderr,"\rERROR: No such file '%s'\n",Path);
        Tempstr=MCopyStr(Tempstr,"No such file [",Path,"]",NULL);
        HandleCheckFail(Path, Tempstr);
    }
    else if (strcasecmp(FP->Path, Path) != 0)
    {
        Tempstr=MCopyStr(Tempstr,"Moved [",FP->Path,"] to [",Path,"]",NULL);
        HandleCheckFail(Path, Tempstr);
    }
    else
    {
        if (FP->Flags & FP_HASSTAT)
        {
            result=CheckStat(Path, &FP->FStat, ActualStat);
            if (result) result=CheckFileHash(Ctx, Path, ActualStat, ActualHash, FP);
        }
        else result=CheckFileHash(Ctx, Path, ActualStat, ActualHash, FP);
    }

    Destroy(Tempstr);
    return(result);
}


//returns true on a significant event, meaning on FAIL
int CheckHashesFromList(HashratCtx *Ctx)
{
    char *HashStr=NULL;
    const char *ptr;
    int Checked=0, Errors=0;
    STREAM *ListStream;
    TFingerprint *FP;
    struct stat Stat;


    ptr=GetVar(Ctx->Vars,"Path");
    if (strcmp(ptr,"-")==0)
    {
        ListStream=STREAMFromFD(0);
        STREAMSetTimeout(ListStream,0);
    }
    else ListStream=STREAMOpen(ptr, "r");

    FP=FingerprintRead(ListStream);
    while (FP)
    {
        if (StrValid(FP->HashType)) Ctx->HashType=CopyStr(Ctx->HashType, FP->HashType);
        if (StatFile(Ctx,FP->Path,&Stat) != -1) HashItem(Ctx, Ctx->HashType, FP->Path, &Stat, &HashStr);

        if (! HashratCheckFile(Ctx, FP->Path, &Stat, HashStr, FP)) Errors++;
        TFingerprintDestroy(FP);
        FP=FingerprintRead(ListStream);
        Checked++;
    }

    fprintf(stderr,"\nChecked %d files. %d Failures\n",Checked,Errors);

    Destroy(HashStr);

    if (Errors) return(TRUE);
    return(FALSE);
}

