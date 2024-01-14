#include "Hash.h"
#include "String.h"
#include "Encodings.h"
#include "List.h"
#include "HashCRC32.h"
#include "HashMD5.h"
#include "HashSHA.h"
#include "HashJH.h"
#include "HashWhirlpool.h"
#include "HashOpenSSL.h"


static ListNode *HashTypes=NULL;


int HashEncodingFromStr(const char *Str)
{
    return(EncodingParse(Str));
}




void HashRegister(const char *Name, int Len, HASH_INIT_FUNC Init)
{
    ListNode *Node;

    if (! HashTypes) HashTypes=ListCreate();
    if (! ListFindNamedItem(HashTypes, Name)) ListAddTypedItem(HashTypes, Len, Name, Init);
}


void HashRegisterAll()
{
    HashRegisterCRC32();
    HashRegisterMD5();
    HashRegisterSHA();
    HashRegisterJH();
    HashRegisterWhirlpool();
    HashRegisterOpenSSL();
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
    ListNode *Curr;

    if (! HashTypes) HashRegisterAll();
    RetStr=CopyStr(RetStr,"");
    Curr=ListGetNext(HashTypes);
    while (Curr)
    {
        RetStr=MCatStr(RetStr, Curr->Tag, ",", NULL);
        Curr=ListGetNext(Curr);
    }

    return(RetStr);
}


//Setup a hash. This can accept a "hash chain" in the form "whirl,sha1,md5" where the output
//of one has is fed into the next. This function sets up the "InitialType" (first hash in the list)
//the user then feeds data into this, and when 'HashFinish' is called the resulting hash output
//is hashed with the other hashes in the chain
HASH *HashInit(const char *Type)
{
    HASH *Hash=NULL;
    ListNode *Node;
    HASH_INIT_FUNC InitFunc;
    char *InitialType=NULL;

    if (! HashTypes) HashRegisterAll();

    GetToken(Type, ",", &InitialType, 0);
    Node=ListFindNamedItem(HashTypes, InitialType);
    if (Node)
    {
        InitFunc=(HASH_INIT_FUNC) Node->Item;
        Hash=(HASH *) calloc(1,sizeof(HASH));
        Hash->Type=CopyStr(Hash->Type,Type);
        if (! InitFunc(Hash, Node->Tag, Node->ItemType))
        {
            HashDestroy(Hash);
            Hash=NULL;
            RaiseError(0, "HashInit", "Failed to setup Hash Type: '%s'", InitialType);
        }
    }
    else RaiseError(0, "HashInit", "Unsupported Hash Type: '%s'", InitialType);

    Destroy(InitialType);

    return(Hash);
}


int HashFinish(HASH *Hash, int Encoding, char **Return)
{
    char *Token=NULL, *Bytes=NULL, *Hashed=NULL;
    const char *ptr;
    int len;

    len=Hash->Finish(Hash, &Bytes);

    //The first hashtype is the 'InitialType' of HashInit, and will
    //already have been processed, so throw it awway here
    ptr=GetToken(Hash->Type, ",", &Token, 0);
    while (StrValid(ptr))
    {
        //process each hash type in the hash chain
        ptr=GetToken(ptr, ",", &Token, 0);
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


