#include "HashCRC32.h"
#include "crc32.h"

void HashUpdateCRC(HASH *Hash, const char *Data, int Len)
{
    crc32Update((unsigned long *) Hash->Ctx, (unsigned char *) Data, Len);
}


HASH *HashCloneCRC(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(unsigned long));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(unsigned long));

    return(NewHash);
}


int HashFinishCRC(HASH *Hash, char **HashStr)
{
    unsigned long crc;
    int len=0;

    *HashStr=CopyStr(*HashStr, "");
    len=sizeof(unsigned long);
    crc32Finish((unsigned long *) Hash->Ctx);
    crc=htonl(* (unsigned long *) Hash->Ctx);

    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,&crc,len);
    return(len);
}


int HashInitCRC(HASH *Hash, const char *Name, int Len)
{
    Hash->Ctx=(void *) calloc(1,sizeof(unsigned long));
    crc32Init((unsigned long *) Hash->Ctx);
    Hash->Update=HashUpdateCRC;
    Hash->Finish=HashFinishCRC;
    Hash->Clone=HashCloneCRC;
    return(TRUE);
}

void HashRegisterCRC32()
{
    HashRegister("crc32", 32, HashInitCRC);
}
