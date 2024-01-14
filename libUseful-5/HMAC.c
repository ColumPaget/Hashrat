#include "HMAC.h"

int HMACBlockSize(const char *Type)
{
    const char *ptr;

    if (strncasecmp(Type, "hmac-",5)==0) ptr=Type+5;
    else ptr=Type;

    if (strcmp(ptr, "md4")==0) return(64);
    if (strcmp(ptr, "md5")==0) return(64);
    if (strcmp(ptr, "sha")==0) return(64);
    if (strcmp(ptr, "sha1")==0) return(64);
    if (strcmp(ptr, "sha224")==0) return(64);
    if (strcmp(ptr, "sha256")==0) return(64);
    return(128);
}


void HMACUpdate(HASH *HMAC, const char *Data, int Len)
{
    HASH *Hash;

    Hash=(HASH *) HMAC->Ctx;
    Hash->Update(Hash,Data,Len);
}



int HMACFinish(HASH *HMAC, char **HashStr)
{
    HASH *Hash;
    int len, result, BlockSize;


    BlockSize=HMACBlockSize(HMAC->Type);
    Hash=(HASH *) HMAC->Ctx;

//We've done with this now, blank it and reuse for the inner result
    HMAC->Key1=CopyStr(HMAC->Key1,"");
    len=Hash->Finish(Hash,&HMAC->Key1);

    HMAC->Key2=SetStrLen(HMAC->Key2,BlockSize+len);
    memcpy(HMAC->Key2+BlockSize,HMAC->Key1,len);

//Hash->Type
    result=HashBytes(HashStr, Hash->Type, HMAC->Key2, BlockSize+len, ENCODE_NONE);

    return(result);
}


void HMACPrepare(HASH *HMAC, const char *Data, int Len)
{
    int i, BlockSize;
    char *Key=NULL, *Tempstr=NULL;

//Whatever we've been given as a key, we have to turn it into a
//key of 'HMAC_BLOCKSIZE', either by hashing it to make it shorter
//or by padding with NULLS
    BlockSize=HMACBlockSize(HMAC->Type);
    Key=SetStrLen(Key, BlockSize);
    memset(Key,0, BlockSize);

    if (Len > BlockSize)
    {
        HMAC->Key1Len=HashBytes(&Tempstr,HMAC->Type,HMAC->Key1,HMAC->Key1Len,0);
        memcpy(Key,Tempstr,HMAC->Key1Len);
    }
    else
    {
        memcpy(Key,HMAC->Key1,HMAC->Key1Len);
    }

    HMAC->Key1=SetStrLen(HMAC->Key1,BlockSize);
    HMAC->Key2=SetStrLen(HMAC->Key2,BlockSize);
    HMAC->Key1Len=BlockSize;
    HMAC->Key2Len=BlockSize;

    for (i=0; i < BlockSize; i++)
    {
//inner key
        HMAC->Key1[i]=Key[i] ^ 0x36;
//outer key
        HMAC->Key2[i]=Key[i] ^ 0x5c;
    }


//first thing to be hashed is the inner key, then data is 'concatted' onto it
    HMACUpdate(HMAC, HMAC->Key1, HMAC->Key1Len);
    HMACUpdate(HMAC, Data, Len);
    HMAC->Update=HMACUpdate;

    DestroyString(Tempstr);
    DestroyString(Key);
}


HASH *HMACInit(const char *Type)
{
    HASH *Hash;

    Hash=(HASH *) calloc(1, sizeof(HASH));
    Hash->Ctx=(void *) HashInit(Type);
    Hash->Type=MCopyStr(Hash->Type, "hmac-", Type, NULL);
    Hash->Update=HMACPrepare;
    Hash->Finish=HMACFinish;

    return(Hash);
}


void HMACSetKey(HASH *HMAC, const char *Key, int Len)
{
    HMAC->Key1=SetStrLen(HMAC->Key1,Len);
    memcpy(HMAC->Key1,Key,Len);
    HMAC->Key1Len=Len;
}

void HMACDestroy(void *p_HMAC)
{
    HASH *HMAC;

    HMAC=(HASH *) p_HMAC;
    HashDestroy(HMAC->Ctx);
    HMAC->Ctx=NULL;
    HashDestroy(HMAC);
}


int HMACBytes(char **RetStr, const char *Type, const char *Key, int KeyLen, const char *Data, int DataLen, int Encoding)
{
    HASH *Hash;
    char *Tempstr=NULL;
    int result;

    Hash=HMACInit(Type);
    HMACSetKey(Hash, Key, KeyLen);
    HMACPrepare(Hash, Data, DataLen);

    if (Encoding > 0)
    {
        result=HMACFinish(Hash, &Tempstr);
        *RetStr=EncodeBytes(*RetStr, Tempstr, result, Encoding);
    }
    else result=HMACFinish(Hash, RetStr);
    HMACDestroy(Hash);

    return(result);
}

