#include "HashWhirlpool.h"
#include "whirlpool.h"

int HashFinishWhirlpool(HASH *Hash, char **HashStr)
{
    int len;
    char *DigestBuff=NULL;

    DigestBuff=(char *) calloc(1,WHIRLPOOL_DIGESTBYTES+1);
    WHIRLPOOLfinalize((WHIRLPOOLstruct *) Hash->Ctx, (unsigned char *) DigestBuff);

    len=WHIRLPOOL_DIGESTBYTES;
    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,DigestBuff,len);

    DestroyString(DigestBuff);

    return(len);
}


void HashUpdateWhirlpool(HASH *Hash, const char *Data, int Len)
{
    WHIRLPOOLadd((unsigned char *) Data, Len * 8, (WHIRLPOOLstruct *) Hash->Ctx);
}


HASH *HashCloneWhirlpool(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(WHIRLPOOLstruct *));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(WHIRLPOOLstruct *));

    return(NewHash);
}



int HashInitWhirlpool(HASH *Hash, const char *Name, int Len)
{
    Hash->Ctx=(void *) calloc(1,sizeof(WHIRLPOOLstruct));
    WHIRLPOOLinit((WHIRLPOOLstruct *) Hash->Ctx);
    Hash->Update=HashUpdateWhirlpool;
    Hash->Finish=HashFinishWhirlpool;
    Hash->Clone=HashCloneWhirlpool;
    return(TRUE);
}

void HashRegisterWhirlpool()
{
    HashRegister("whirl", 0, HashInitWhirlpool);
    HashRegister("whirlpool", 0, HashInitWhirlpool);
}
