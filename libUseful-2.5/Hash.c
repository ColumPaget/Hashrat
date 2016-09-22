#include "Hash.h"
#include "string.h"

#define HMAC_BLOCKSIZE 64


typedef void (*HASH_INIT_FUNC)(THash *Hash, int Len);


char *HashTypes[]={"md5","sha1","sha256","sha512","whirlpool","jh-224","jh-256","jh-384","jh-512",NULL};


void HMACUpdate(THash *HMAC, char *Data, int Len)
{
THash *Hash;

Hash=(THash *) HMAC->Ctx;
Hash->Update(Hash,Data,Len);
}



int HMACFinish(THash *HMAC, int Encoding, char **HashStr)
{
THash *Hash;
int len, result;


Hash=(THash *) HMAC->Ctx;

//We've done with this now, blank it and reuse for the inner result
HMAC->Key1=CopyStr(HMAC->Key1,"");
len=Hash->Finish(Hash,&HMAC->Key1);

HMAC->Key2=SetStrLen(HMAC->Key2,HMAC_BLOCKSIZE+len);
memcpy(HMAC->Key2+HMAC_BLOCKSIZE,HMAC->Key1,len);

//Hash->Type
result=HashBytes(HashStr,Hash->Type,HMAC->Key2,HMAC_BLOCKSIZE+len,Encoding);

return(result);
}


void HMACPrepare(THash *HMAC, char *Data, int Len)
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


void HMACInit(THash *Hash)
{
Hash->Ctx=(void *) HashInit(Hash->Type+5);

Hash->Update=HMACPrepare;
Hash->Finish=HMACFinish;
}


void HMACSetKey(THash *HMAC, char *Key, int Len)
{
HMAC->Key1=SetStrLen(HMAC->Key1,Len);
memcpy(HMAC->Key1,Key,Len);
HMAC->Key1Len=Len;
}



#include "crc32.h"

void HashUpdateCRC(THash *Hash, char *Data, int Len)
{
crc32Update((unsigned long *) &Hash->Ctx, (unsigned char *) Data, Len);
}


THash *HashCloneCRC(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(unsigned long));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(unsigned long));

return(NewHash);
}


int HashFinishCRC(THash *Hash, char **HashStr)
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


void HashInitCRC(THash *Hash, int Len)
{
Hash->Ctx=(void *) calloc(1,sizeof(unsigned long));
crc32Init((unsigned long *) Hash->Ctx);
Hash->Update=HashUpdateCRC;
Hash->Finish=HashFinishCRC;
Hash->Clone=HashCloneCRC;
}


#include "md5.h"
#define MD5LEN 16

void HashUpdateMD5(THash *Hash, char *Data, int Len)
{
MD5Update((MD5_CTX *) Hash->Ctx, Data, Len);
}


THash *HashCloneMD5(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(MD5_CTX));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(MD5_CTX));
NewHash->Update=Hash->Update;
NewHash->Finish=Hash->Finish;
NewHash->Clone=Hash->Clone;

return(NewHash);
}


int HashFinishMD5(THash *Hash, char **HashStr)
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


void HashInitMD5(THash *Hash, int Len)
{
Hash->Ctx=(void *) calloc(1,sizeof(MD5_CTX));
MD5Init((MD5_CTX *) Hash->Ctx);
Hash->Update=HashUpdateMD5;
Hash->Finish=HashFinishMD5;
Hash->Clone=HashCloneMD5;
}

#include "sha1.h"
#define SHA1LEN 20

void HashUpdateSHA1(THash *Hash, char *Data, int Len)
{
sha1_process_bytes(Data,Len,(struct sha1_ctx *) Hash->Ctx);
}


THash *HashCloneSHA1(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(struct sha1_ctx));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(struct sha1_ctx));

return(NewHash);
}


int HashFinishSHA1(THash *Hash, char **HashStr)
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

int HashFinishSHA256(THash *Hash, char **HashStr)
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


THash *HashCloneSHA256(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(SHA2_SHA256_CTX));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(SHA2_SHA256_CTX));

return(NewHash);
}


void HashUpdateSHA256(THash *Hash, char *Data, int Len)
{
SHA2_SHA256_Update((SHA2_SHA256_CTX *) Hash->Ctx, (unsigned char *) Data, Len);
}


int HashFinishSHA512(THash *Hash, char **HashStr)
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


void HashUpdateSHA512(THash *Hash, char *Data, int Len)
{
SHA2_SHA512_Update((SHA2_SHA512_CTX *) Hash->Ctx, (unsigned char *) Data, Len);
}


THash *HashCloneSHA512(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(SHA2_SHA512_CTX));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(SHA2_SHA512_CTX));

return(NewHash);
}




void HashInitSHA(THash *Hash, int Len)
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

int HashFinishWhirlpool(THash *Hash, char **HashStr)
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


void HashUpdateWhirlpool(THash *Hash, char *Data, int Len)
{
WHIRLPOOLadd((unsigned char *) Data, Len * 8, (WHIRLPOOLstruct *) Hash->Ctx);
}


THash *HashCloneWhirlpool(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(WHIRLPOOLstruct *));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(WHIRLPOOLstruct *));

return(NewHash);
}



void HashInitWhirlpool(THash *Hash, int Len)
{
Hash->Ctx=(void *) calloc(1,sizeof(WHIRLPOOLstruct));
WHIRLPOOLinit((WHIRLPOOLstruct *) Hash->Ctx);
Hash->Update=HashUpdateWhirlpool;
Hash->Finish=HashFinishWhirlpool;
Hash->Clone=HashCloneWhirlpool;
}


#include "jh_ref.h"

int HashFinishJH(THash *Hash, char **HashStr)
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



void HashUpdateJH(THash *Hash, char *Data, int Len)
{
	JHUpdate( (hashState *) Hash->Ctx, (unsigned char *) Data, Len);
}


THash *HashCloneJH(THash *Hash)
{
THash *NewHash;

NewHash=(THash *) calloc(1,sizeof(THash));
NewHash->Type=CopyStr(NewHash->Type,Hash->Type);
NewHash->Ctx=(void *) calloc(1,sizeof(hashState *));
memcpy(NewHash->Ctx, Hash->Ctx, sizeof(hashState *));

return(NewHash);
}



int HashInitJH(THash *Hash, int Length)
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







void HashDestroy(THash *Hash)
{
//Hash->Ctx is destroyed in 'HashFinish'
DestroyString(Hash->Key1);
DestroyString(Hash->Key2);
DestroyString(Hash->Type);
if (Hash->Ctx) free(Hash->Ctx);
free(Hash);
}


void HashAvailableTypes(ListNode *Vars)
{
int i;

for (i=0; HashTypes[i] !=NULL; i++) SetVar(Vars,HashTypes[i], HashTypes[i]);

}


THash *HashInit(const char *Type)
{
THash *Hash=NULL;
char *InitialType=NULL;

Hash=(THash *) calloc(1,sizeof(THash));
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
	HashDestroy(Hash);
	Hash=NULL;
}

DestroyString(InitialType);
return(Hash);
}


int HashFinish(THash *Hash, int Encoding, char **Return)
{
char *Token=NULL, *Bytes=NULL, *Hashed=NULL, *ptr;
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

HashDestroy(Hash);
DestroyString(Hashed);
DestroyString(Token);
DestroyString(Bytes);

return(len);
}


int HashBytes(char **Return, char *Type, char *text, int len, int Encoding)
{
THash *Hash;
int result;

Hash=HashInit(Type);
if (! Hash) return(0);
Hash->Update(Hash, text, len);
result=HashFinish(Hash, Encoding, Return);

return(result);
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

StrLen(*Return);
}


int HashFile(char **Return, char *Type, char *Path, int Encoding)
{
THash *Hash;
STREAM *S;
char *Tempstr=NULL;
int result;

S=STREAMOpenFile(Path,SF_RDONLY);
if (! S) return(FALSE);

Hash=HashInit(Type);
if (! Hash) 
{
	STREAMClose(S);
	return(FALSE);
}


Tempstr=SetStrLen(Tempstr,4096);
result=STREAMReadBytes(S,Tempstr,4096);
while (result !=EOF)
{
	Hash->Update(Hash, Tempstr, result);
	result=STREAMReadBytes(S,Tempstr,4096);
}

DestroyString(Tempstr);
STREAMClose(S);

result=HashFinish(Hash, Encoding, Return);

return(result);
}




