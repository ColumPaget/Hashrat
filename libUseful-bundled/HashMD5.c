#include "HashMD5.h"

#include "md5.h"
#define MD5LEN 16

void HashUpdateMD5(HASH *Hash, const char *Data, int Len)
{
    MD5Update((MD5_CTX *) Hash->Ctx, (const unsigned char *) Data, Len);
}


HASH *HashCloneMD5(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(MD5_CTX));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(MD5_CTX));
    NewHash->Update=Hash->Update;
    NewHash->Finish=Hash->Finish;
    NewHash->Clone=Hash->Clone;

    return(NewHash);
}


int HashFinishMD5(HASH *Hash, char **HashStr)
{
    int len;
    char *DigestBuff=NULL;

    DigestBuff=(char *) calloc(1,MD5LEN+1);
    MD5Final((unsigned char *) DigestBuff, (MD5_CTX *) Hash->Ctx);

    len=MD5LEN;
    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,DigestBuff,len);

    DestroyString(DigestBuff);

    return(len);
}


int HashInitMD5(HASH *Hash, const char *Name, int Len)
{
    Hash->Ctx=(void *) calloc(1,sizeof(MD5_CTX));
    MD5Init((MD5_CTX *) Hash->Ctx);
    Hash->Update=HashUpdateMD5;
    Hash->Finish=HashFinishMD5;
    Hash->Clone=HashCloneMD5;

    return(TRUE);
}


void HashRegisterMD5()
{
    HashRegister("md5", 0, HashInitMD5);
}
