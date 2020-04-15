#include "Hash.h"
#include "String.h"

#define HMAC_BLOCKSIZE 64


typedef void (*HASH_INIT_FUNC)(HASH *Hash, int Len);


static const char *HashTypes[]= {"md5","sha1","sha256","sha512","whirlpool","jh-224","jh-256","jh-384","jh-512",NULL};

//mostly a helper function for environments where integer constants are not convinient
int HashEncodingFromStr(const char *Str)
{
    int Encode=ENCODE_HEX;

    if (StrValid(Str))
    {
        switch (*Str)
        {
        case '1':
            if (strcasecmp(Str,"16")==0) Encode=ENCODE_HEX;
            else if (strcasecmp(Str,"10")==0) Encode=ENCODE_DECIMAL;
            break;

        case '6':
        case '8':
            if (strcasecmp(Str,"64")==0) Encode=ENCODE_BASE64;
            else if (strcasecmp(Str,"8")==0) Encode=ENCODE_OCTAL;
            break;

        case 'a':
        case 'A':
            if (strcasecmp(Str,"a85")==0) Encode=ENCODE_ASCII85;
            else if (strcasecmp(Str,"ascii85")==0) Encode=ENCODE_ASCII85;
            else if (strcasecmp(Str,"asci85")==0) Encode=ENCODE_ASCII85;
            break;

        case 'b':
        case 'B':
            if (strcasecmp(Str,"base64")==0) Encode=ENCODE_BASE64;
            else if (strcasecmp(Str,"b64")==0) Encode=ENCODE_BASE64;
            break;

        case 'd':
        case 'h':
        case 'o':
        case 'D':
        case 'H':
        case 'O':
            if (strcasecmp(Str,"hex")==0) Encode=ENCODE_HEX;
            else if (strcasecmp(Str,"hexupper")==0) Encode=ENCODE_HEXUPPER;
            else if (strcasecmp(Str,"oct")==0) Encode=ENCODE_OCTAL;
            else if (strcasecmp(Str,"octal")==0) Encode=ENCODE_OCTAL;
            else if (strcasecmp(Str,"dec")==0) Encode=ENCODE_DECIMAL;
            else if (strcasecmp(Str,"decimal")==0) Encode=ENCODE_DECIMAL;
            break;

        case 'i':
        case 'I':
            if (strcasecmp(Str,"ibase64")==0) Encode=ENCODE_IBASE64;
            else if (strcasecmp(Str,"i64")==0) Encode=ENCODE_IBASE64;
            break;

        case 'p':
        case 'P':
            if (strcasecmp(Str,"pbase64")==0) Encode=ENCODE_PBASE64;
            else if (strcasecmp(Str,"p64")==0) Encode=ENCODE_PBASE64;
            break;

        case 'u':
        case 'U':
            if (strcasecmp(Str,"uu")==0) Encode=ENCODE_UUENC;
            else if (strcasecmp(Str,"uuencode")==0) Encode=ENCODE_UUENC;
            else if (strcasecmp(Str,"uuenc")==0) Encode=ENCODE_UUENC;
            break;

        case 'x':
        case 'X':
            if (strcasecmp(Str,"xx")==0) Encode=ENCODE_XXENC;
            else if (strcasecmp(Str,"xxencode")==0) Encode=ENCODE_XXENC;
            else if (strcasecmp(Str,"xxenc")==0) Encode=ENCODE_XXENC;
            break;

        case 'z':
        case 'Z':
            if (strcasecmp(Str,"z85")==0) Encode=ENCODE_Z85;
            break;
        }
    }

    return(Encode);
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
    int len, result;


    Hash=(HASH *) HMAC->Ctx;

//We've done with this now, blank it and reuse for the inner result
    HMAC->Key1=CopyStr(HMAC->Key1,"");
    len=Hash->Finish(Hash,&HMAC->Key1);

    HMAC->Key2=SetStrLen(HMAC->Key2,HMAC_BLOCKSIZE+len);
    memcpy(HMAC->Key2+HMAC_BLOCKSIZE,HMAC->Key1,len);

//Hash->Type
    result=HashBytes(HashStr,Hash->Type,HMAC->Key2,HMAC_BLOCKSIZE+len,0);

    return(result);
}


void HMACPrepare(HASH *HMAC, const char *Data, int Len)
{
    int i;
    char *Key=NULL, *Tempstr=NULL;

//Whatever we've been given as a key, we have to turn it into a
//key of 'HMAC_BLOCKSIZE', either by hashing it to make it shorter
//or by padding with NULLS
    Key=SetStrLen(Key,HMAC_BLOCKSIZE);
    memset(Key,0,HMAC_BLOCKSIZE);

    if (Len > HMAC_BLOCKSIZE)
    {
        HMAC->Key1Len=HashBytes(&Tempstr,HMAC->Type,HMAC->Key1,HMAC->Key1Len,0);
        memcpy(Key,Tempstr,HMAC->Key1Len);
    }
    else
    {
        memcpy(Key,HMAC->Key1,HMAC->Key1Len);
    }

    HMAC->Key1=SetStrLen(HMAC->Key1,HMAC_BLOCKSIZE);
    HMAC->Key2=SetStrLen(HMAC->Key2,HMAC_BLOCKSIZE);
    HMAC->Key1Len=HMAC_BLOCKSIZE;
    HMAC->Key2Len=HMAC_BLOCKSIZE;

    for (i=0; i < HMAC_BLOCKSIZE; i++)
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


void HMACInit(HASH *Hash)
{
    Hash->Ctx=(void *) HashInit(Hash->Type+5);

    Hash->Update=HMACPrepare;
    Hash->Finish=HMACFinish;
}


void HMACSetKey(HASH *HMAC, const char *Key, int Len)
{
    HMAC->Key1=SetStrLen(HMAC->Key1,Len);
    memcpy(HMAC->Key1,Key,Len);
    HMAC->Key1Len=Len;
}



#include "crc32.h"

void HashUpdateCRC(HASH *Hash, const char *Data, int Len)
{
    crc32Update((unsigned long *) &Hash->Ctx, (unsigned char *) Data, Len);
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
    int len;

    len=sizeof(unsigned long);
    crc32Finish((unsigned long *) Hash->Ctx);
    crc=htonl(* (unsigned long *) Hash->Ctx);

    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,&crc,len);
    return(len);
}


void HashInitCRC(HASH *Hash, int Len)
{
    Hash->Ctx=(void *) calloc(1,sizeof(unsigned long));
    crc32Init((unsigned long *) Hash->Ctx);
    Hash->Update=HashUpdateCRC;
    Hash->Finish=HashFinishCRC;
    Hash->Clone=HashCloneCRC;
}


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


void HashInitMD5(HASH *Hash, int Len)
{
    Hash->Ctx=(void *) calloc(1,sizeof(MD5_CTX));
    MD5Init((MD5_CTX *) Hash->Ctx);
    Hash->Update=HashUpdateMD5;
    Hash->Finish=HashFinishMD5;
    Hash->Clone=HashCloneMD5;
}

#include "sha1.h"
#define SHA1LEN 20

void HashUpdateSHA1(HASH *Hash, const char *Data, int Len)
{
    sha1_process_bytes(Data,Len,(struct sha1_ctx *) Hash->Ctx);
}


HASH *HashCloneSHA1(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(struct sha1_ctx));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(struct sha1_ctx));

    return(NewHash);
}


int HashFinishSHA1(HASH *Hash, char **HashStr)
{
    int len;
    char *DigestBuff=NULL;

    DigestBuff=(char *) calloc(1,SHA1LEN+1);
    sha1_finish_ctx((struct sha1_ctx *) Hash->Ctx, DigestBuff);

    len=SHA1LEN;
    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,DigestBuff,len);

    DestroyString(DigestBuff);

    return(len);
}



#include "sha2.h"

int HashFinishSHA256(HASH *Hash, char **HashStr)
{
    int len;
    char *DigestBuff=NULL;

    DigestBuff=(char *) calloc(1,SHA2_SHA256_DIGEST_LENGTH+1);
    SHA2_SHA256_Final((unsigned char *) DigestBuff, (SHA2_SHA256_CTX *) Hash->Ctx);

    len=SHA2_SHA256_DIGEST_LENGTH;
    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,DigestBuff,len);

    DestroyString(DigestBuff);

    return(len);
}


HASH *HashCloneSHA256(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(SHA2_SHA256_CTX));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(SHA2_SHA256_CTX));

    return(NewHash);
}


void HashUpdateSHA256(HASH *Hash, const char *Data, int Len)
{
    SHA2_SHA256_Update((SHA2_SHA256_CTX *) Hash->Ctx, (unsigned char *) Data, Len);
}


int HashFinishSHA512(HASH *Hash, char **HashStr)
{
    int len;
    char *DigestBuff=NULL;

    DigestBuff=(char *) calloc(1,SHA2_SHA512_DIGEST_LENGTH+1);
    SHA2_SHA512_Final((unsigned char *) DigestBuff, (SHA2_SHA512_CTX *) Hash->Ctx);

    len=SHA2_SHA512_DIGEST_LENGTH;
    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,DigestBuff,len);

    DestroyString(DigestBuff);

    return(len);
}


void HashUpdateSHA512(HASH *Hash, const char *Data, int Len)
{
    SHA2_SHA512_Update((SHA2_SHA512_CTX *) Hash->Ctx, (unsigned char *) Data, Len);
}


HASH *HashCloneSHA512(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(SHA2_SHA512_CTX));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(SHA2_SHA512_CTX));

    return(NewHash);
}




void HashInitSHA(HASH *Hash, int Len)
{

    switch (Len)
    {
    case 512:
        Hash->Ctx=(void *) calloc(1,sizeof(SHA2_SHA512_CTX));
        SHA2_SHA512_Init((SHA2_SHA512_CTX *) Hash->Ctx);
        Hash->Update=HashUpdateSHA512;
        Hash->Finish=HashFinishSHA512;
        Hash->Clone=HashCloneSHA512;
        break;

    case 256:
        Hash->Ctx=(void *) calloc(1,sizeof(SHA2_SHA256_CTX));
        SHA2_SHA256_Init((SHA2_SHA256_CTX *) Hash->Ctx);
        Hash->Update=HashUpdateSHA256;
        Hash->Finish=HashFinishSHA256;
        Hash->Clone=HashCloneSHA256;
        break;

    default:
        Hash->Ctx=(void *) calloc(1,sizeof(struct sha1_ctx));
        sha1_init_ctx((struct sha1_ctx *) Hash->Ctx);
        Hash->Update=HashUpdateSHA1;
        Hash->Finish=HashFinishSHA1;
        Hash->Clone=HashCloneSHA1;
        break;
    }

}



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



void HashInitWhirlpool(HASH *Hash, int Len)
{
    Hash->Ctx=(void *) calloc(1,sizeof(WHIRLPOOLstruct));
    WHIRLPOOLinit((WHIRLPOOLstruct *) Hash->Ctx);
    Hash->Update=HashUpdateWhirlpool;
    Hash->Finish=HashFinishWhirlpool;
    Hash->Clone=HashCloneWhirlpool;
}


#include "jh_ref.h"

int HashFinishJH(HASH *Hash, char **HashStr)
{
    int len;
    char *DigestBuff=NULL;

    DigestBuff=(char *) calloc(1,1024);

    len=JHFinal((hashState *) Hash->Ctx, (unsigned char *) DigestBuff);
    *HashStr=SetStrLen(*HashStr,len);
    memcpy(*HashStr,DigestBuff,len);

    DestroyString(DigestBuff);

    return(len);
}



void HashUpdateJH(HASH *Hash, const char *Data, int Len)
{
    JHUpdate( (hashState *) Hash->Ctx, (unsigned char *) Data, Len);
}


HASH *HashCloneJH(HASH *Hash)
{
    HASH *NewHash;

    NewHash=(HASH *) calloc(1,sizeof(HASH));
    NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
    NewHash->Ctx=(void *) calloc(1,sizeof(hashState *));
    memcpy(NewHash->Ctx, Hash->Ctx, sizeof(hashState *));

    return(NewHash);
}



int HashInitJH(HASH *Hash, int Length)
{

    switch (Length)
    {
    case 224:
    case 256:
    case 384:
    case 512:
        Hash->Ctx=(void *) calloc(1,sizeof(hashState));
        JHInit((hashState *) Hash->Ctx, Length);
        Hash->Update=HashUpdateJH;
        Hash->Finish=HashFinishJH;
        Hash->Clone=HashCloneJH;
        break;

    default:
        return(FALSE);
        break;
    }

    return(TRUE);
}







void HashDestroy(HASH *Hash)
{
//Hash->Ctx is destroyed in 'HashFinish'
    DestroyString(Hash->Key1);
    DestroyString(Hash->Key2);
    DestroyString(Hash->Type);
    if (Hash->Ctx) free(Hash->Ctx);
    free(Hash);
}


char *HashAvailableTypes(char *RetStr)
{
    int i;

    RetStr=CopyStr(RetStr,"");
    for (i=0; HashTypes[i] !=NULL; i++)
    {
        RetStr=MCatStr(RetStr, HashTypes[i], ",", NULL);
    }
    return(RetStr);
}


HASH *HashInit(const char *Type)
{
    HASH *Hash=NULL;
    char *InitialType=NULL;

    Hash=(HASH *) calloc(1,sizeof(HASH));
    Hash->Type=CopyStr(Hash->Type,Type);
    strrep(Hash->Type,',',' ');

    GetToken(Hash->Type,"\\S",&InitialType,0);
    if (strcasecmp(InitialType,"md5")==0) HashInitMD5(Hash, 0);
    else if (strcasecmp(InitialType,"sha")==0) HashInitSHA(Hash, 0);
    else if (strcasecmp(InitialType,"sha1")==0) HashInitSHA(Hash, 0);
    else if (strcasecmp(InitialType,"sha256")==0) HashInitSHA(Hash, 256);
    else if (strcasecmp(InitialType,"sha512")==0) HashInitSHA(Hash, 512);
    else if (strcasecmp(InitialType,"sha-256")==0) HashInitSHA(Hash, 256);
    else if (strcasecmp(InitialType,"sha-512")==0) HashInitSHA(Hash, 512);
    else if (strcasecmp(InitialType,"whirl")==0) HashInitWhirlpool(Hash, 0);
    else if (strcasecmp(InitialType,"whirlpool")==0) HashInitWhirlpool(Hash, 0);
    else if (strcasecmp(InitialType,"jh224")==0) HashInitJH(Hash,224);
    else if (strcasecmp(InitialType,"jh256")==0) HashInitJH(Hash,256);
    else if (strcasecmp(InitialType,"jh384")==0) HashInitJH(Hash,384);
    else if (strcasecmp(InitialType,"jh512")==0) HashInitJH(Hash,512);
    else if (strcasecmp(InitialType,"jh-224")==0) HashInitJH(Hash,224);
    else if (strcasecmp(InitialType,"jh-256")==0) HashInitJH(Hash,256);
    else if (strcasecmp(InitialType,"jh-384")==0) HashInitJH(Hash,384);
    else if (strcasecmp(InitialType,"jh-512")==0) HashInitJH(Hash,512);
//else if (strcasecmp(InitialType,"crc32")==0) HashInitCRC(Hash, 0);
    else if (strncasecmp(InitialType,"hmac-",5)==0) HMACInit(Hash);
    else
    {
        RaiseError(0, "HashInit", "Unsupported Hash Type '%s'",InitialType);
        HashDestroy(Hash);
        Hash=NULL;
    }

    DestroyString(InitialType);
    return(Hash);
}


int HashFinish(HASH *Hash, int Encoding, char **Return)
{
    char *Token=NULL, *Bytes=NULL, *Hashed=NULL;
    const char *ptr;
    int len;

    ptr=GetToken(Hash->Type, "\\S", &Token, 0);
    len=Hash->Finish(Hash, &Bytes);

    while (StrValid(ptr))
    {
        ptr=GetToken(ptr, "\\S", &Token, 0);
        len=HashBytes(&Hashed, Token, Bytes, len, 0);
        Bytes=SetStrLen(Bytes, len);
        memcpy(Bytes,Hashed,len);
    }

    if (Encoding > 0)
    {
        *Return=EncodeBytes(*Return, Bytes, len, Encoding);
        len=StrLen(*Return);
    }
    else
    {
        *Return=SetStrLen(*Return, len);
        memcpy(*Return, Bytes, len);
    }

    DestroyString(Hashed);
    DestroyString(Token);
    DestroyString(Bytes);

    return(len);
}


int HashBytes(char **Return, const char *Type, const char *text, int len, int Encoding)
{
    HASH *Hash;
    int result;

    Hash=HashInit(Type);
    if (! Hash) return(0);
    Hash->Update(Hash, text, len);
    result=HashFinish(Hash, Encoding, Return);

    return(result);
}

int HashBytes2(const char *Type, int Encoding, const char *text, int len, char **RetStr)
{
    return(HashBytes(RetStr, Type, text, len, Encoding));
}

int PBK2DF2(char **Return, char *Type, char *Bytes, int Len, char *Salt, int SaltLen, uint32_t Rounds, int Encoding)
{
    char *Tempstr=NULL, *Hash=NULL;
    uint32_t RoundsBE;
    int i, len, hlen;

//Network byte order is big endian
    RoundsBE=htonl(Rounds);

    Tempstr=SetStrLen(Tempstr, Len + SaltLen + 20);
    memcpy(Tempstr, Bytes, Len);
    memcpy(Tempstr+Len, Salt, SaltLen);
    memcpy(Tempstr+Len+SaltLen, &RoundsBE, sizeof(uint32_t));
    len=Len+SaltLen+sizeof(uint32_t);

    for (i=0; i <Rounds; i++)
    {
        hlen=HashBytes(&Hash, Type, Tempstr, len, 0);
        Tempstr=SetStrLen(Tempstr, Len + hlen + 20);
        memcpy(Tempstr, Bytes, Len);
        memcpy(Tempstr+Len, Hash, hlen);
        len=Len + hlen;
    }

    *Return=EncodeBytes(*Return, Hash, hlen, Encoding);

    DestroyString(Tempstr);
    DestroyString(Hash);

    return(StrLen(*Return));
}


int HashSTREAM(char **Return, const char *Type, STREAM *S, int Encoding)
{
    HASH *Hash;
    char *Tempstr=NULL;
    int result;

    if (! S) return(FALSE);

    Hash=HashInit(Type);
    if (! Hash) return(FALSE);

    Tempstr=SetStrLen(Tempstr,4096);
    result=STREAMReadBytes(S,Tempstr,4096);
    while (result !=EOF)
    {
        Hash->Update(Hash, Tempstr, result);
        result=STREAMReadBytes(S,Tempstr,4096);
    }

    DestroyString(Tempstr);

    result=HashFinish(Hash, Encoding, Return);

    return(result);
}


int HashFile(char **Return, const char *Type, const char *Path, int Encoding)
{
int result=FALSE;
STREAM *S;

S=STREAMOpen(Path,"r");
if (S) 
{
	result=HashSTREAM(Return, Type, S, Encoding);
	STREAMClose(S);
}

return(result);
}


