#include "fingerprint.h"
#include <ctype.h>

/*

The 'fingerprint' concept here is the combination of a path, its hash, the hash type, etc.

*/


void ParseBSDFormat(const char *Data, char **Type, char **Hash, char **Path)
{
    int result=FALSE;
    const char *ptr;

    ptr=GetToken(Data,"\\S",Type,0);
    if (ptr)
    {
        while (isspace(*ptr)) ptr++;
        if (*ptr=='(') ptr++;
        ptr=GetToken(ptr,")",Path,0);
        if (ptr)
        {
            while (isspace(*ptr)) ptr++;
            if (*ptr=='=') ptr++;
            while (isspace(*ptr)) ptr++;
        }
        *Hash=CopyStr(*Hash,ptr);
    }

}


void ParseTradFormat(const char *Data, char **Hash, char **Path)
{
    const char *ptr;

    ptr=GetToken(Data,"\\S",Hash,0);
    if (ptr)
    {
        if (*ptr=='*') ptr++;
        while (isspace(*ptr)) ptr++;
        *Path=CopyStr(*Path,ptr);
    }
}





void TFingerprintDestroy(void *p_Fingerprint)
{
    TFingerprint *Fingerprint;

    if (! p_Fingerprint) return;
    Fingerprint=(TFingerprint *) p_Fingerprint;
    Destroy(Fingerprint->Hash);
    Destroy(Fingerprint->Data);
    Destroy(Fingerprint->Path);
    Destroy(Fingerprint->HashType);

    free(Fingerprint);
}



TFingerprint *TFingerprintCreate(const char *Hash, const char *HashType, const char *Data, const char *Path)
{
    TFingerprint *Item;

    Item=(TFingerprint *) calloc(1,sizeof(TFingerprint));
    Item->Hash=CopyStr(Item->Hash, Hash);
    strlwr(Item->Hash);
    Item->HashType=CopyStr(Item->HashType, HashType);
    Item->Data=CopyStr(Item->Data, Data);
    Item->Path=CopyStr(Item->Path, Path);

    return(Item);
}




TFingerprint *TFingerprintParse(const char *Data)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    TFingerprint *FP;

    FP=(TFingerprint *) calloc(1,sizeof(TFingerprint));
    FP->Path=CopyStr(FP->Path,"");
    FP->Hash=CopyStr(FP->Hash,"");
    FP->Flags=0;
    memset(&FP->FStat,0,sizeof(struct stat));

    if (strncmp(Data,"hash=",5) ==0)
    {
//Native format

        ptr=GetNameValuePair(Data," ","=",&Name,&Value);
        while (ptr)
        {
            if (StrLen(Name))
            {
                if (strcmp(Name,"path")==0) FP->Path=UnQuoteStr(FP->Path,Value);
                if (strcmp(Name,"size")==0)
                {
                    FP->Flags |= FP_HASSTAT;
                    FP->FStat.st_size=(size_t) strtol(Value,NULL,10);
                }
                if (strcmp(Name,"mode")==0)
                {
                    FP->Flags |= FP_HASSTAT;
                    FP->FStat.st_mode=strtol(Value,NULL,8);
                }
                if (strcmp(Name,"mtime")==0)
                {
                    FP->Flags |= FP_HASSTAT;
                    FP->FStat.st_mtime=strtol(Value,NULL,10);
                }
                if (strcmp(Name,"inode")==0)
                {
                    FP->Flags |= FP_HASSTAT;
                    FP->FStat.st_ino=(ino_t) strtol(Value,NULL,10);
                }
                if (strcmp(Name,"uid")==0)
                {
                    FP->Flags |= FP_HASSTAT;
                    FP->FStat.st_uid=strtol(Value,NULL,10);
                }
                if (strcmp(Name,"gid")==0)
                {
                    FP->Flags |= FP_HASSTAT;
                    FP->FStat.st_gid=strtol(Value,NULL,10);
                }
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
        ptr=GetToken(Data," ",&FP->Hash,0);
        if (StrLen(FP->Hash) < 10) ParseBSDFormat(Data, &FP->HashType, &FP->Hash, &FP->Path);
        else ParseTradFormat(Data, &FP->Hash, &FP->Path);
    }

    if (StrLen(FP->Hash)) strlwr(FP->Hash);
    Destroy(Name);
    Destroy(Value);

    return(FP);
}


TFingerprint *FingerprintRead(STREAM *S)
{
    char *Tempstr=NULL;
    TFingerprint *FP;

    Tempstr=STREAMReadLine(Tempstr,S);
    if (! Tempstr) return(NULL);

    StripTrailingWhitespace(Tempstr);
    FP=TFingerprintParse(Tempstr);

    Destroy(Tempstr);
    return(FP);
}




int FingerprintCompare(const void *v1, const void *v2)
{
    const TFingerprint *FP1, *FP2;

    FP1=(TFingerprint *) v1;
    FP2=(TFingerprint *) v2;

    if (! FP1->Hash) return(FALSE);
    if (! FP2->Hash) return(TRUE);
    if (strcmp(FP1->Hash,FP2->Hash) < 0) return(TRUE);

    return(FALSE);
}

