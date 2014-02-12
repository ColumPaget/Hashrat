#include "Hash.h"
#include "string.h"

#define HMAC_BLOCKSIZE 64

char *EncodeBase64(char *Return, char *Text, int len)
{
char *RetStr;

RetStr=SetStrLen(Return,len *2);
to64frombits(RetStr,Text,len);

return(RetStr);
}

char *DecodeBase64(char *Return, int *len, char *Text)
{
char *RetStr;

RetStr=SetStrLen(Return,StrLen(Text) *2);
*len=from64tobits(RetStr,Text);

return(RetStr);
}


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
len=Hash->Finish(Hash,0,&HMAC->Key1);

HMAC->Key2=SetStrLen(HMAC->Key2,HMAC_BLOCKSIZE+len);
memcpy(HMAC->Key2+HMAC_BLOCKSIZE,HMAC->Key1,len);

//Hash->Type
result=HashBytes(HashStr,Hash->Type,HMAC->Key2,HMAC_BLOCKSIZE+len,Encoding);

return(result);
}


void HMACPrepare(THash *HMAC, char *Data, int Len)
{
int i, len;
char *Key=NULL, *Tempstr=NULL;

//Whatever we've been given as a key, we have to turn it into a
//key of 'HMAC_BLOCKSIZE', either by hashing it to make it shorter
//or by padding with NULLS
Key=SetStrLen(Key,HMAC_BLOCKSIZE);
memset(Key,0,HMAC_BLOCKSIZE);

if (len > HMAC_BLOCKSIZE) 
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


char *EncodeHash(char *Buffer, char *Digest, int len, int Encoding)
{
char *Tempstr=NULL, *RetStr=NULL;
int i;

RetStr=SetStrLen(Buffer,128);
if (Encoding==ENCODE_BASE64) to64frombits(RetStr,Digest,len);
else
{
	for (i=0; i < len; i++)
	{
	Tempstr=FormatStr(Tempstr,"%02x",Digest[i] & 255);
	RetStr=CatStr(RetStr,Tempstr);
	}
}

DestroyString(Tempstr);
return(RetStr);
}




#include "crc32.h"

void HashUpdateCRC(THash *Hash, char *Data, int Len)
{
crc32Update((unsigned long *) &Hash->Ctx, Data, Len);
}


int HashFinishCRC(THash *Hash, int Encoding, char **HashStr)
{
unsigned long crc;
int len;

len=sizeof(unsigned long);
crc32Finish((unsigned long *) Hash->Ctx);
//crc=htonl((unsigned long *) Hash->Ctx);

free(Hash->Ctx);

if (Encoding > 0) 
{
*HashStr=EncodeHash(*HashStr, (char *) &crc, len, Encoding);
return(StrLen(*HashStr));
}
else
{
*HashStr=SetStrLen(*HashStr,len);
memcpy(*HashStr,&crc,len);
return(len);
}
}


void HashInitCRC(THash *Hash)
{
Hash->Ctx=(void *) calloc(1,sizeof(unsigned long));
crc32Init((unsigned long *) Hash->Ctx);
Hash->Update=HashUpdateCRC;
Hash->Finish=HashFinishCRC;
}


#include "md5.h"
#define MD5LEN 16

void HashUpdateMD5(THash *Hash, char *Data, int Len)
{
MD5Update((MD5_CTX *) Hash->Ctx, Data, Len);
}


int HashFinishMD5(THash *Hash, int Encoding, char **HashStr)
{
int count, len;
char *Tempstr=NULL, *DigestBuff=NULL;

DigestBuff=(char *) calloc(1,MD5LEN+1);
MD5Final(DigestBuff, (MD5_CTX *) Hash->Ctx);

free(Hash->Ctx);

if (Encoding > 0)
{
*HashStr=EncodeHash(*HashStr, DigestBuff, MD5LEN, Encoding);
len=StrLen(*HashStr);
}
else
{
len=MD5LEN;
*HashStr=SetStrLen(*HashStr,len);
memcpy(*HashStr,DigestBuff,len);
}

DestroyString(DigestBuff);
DestroyString(Tempstr);

return(len);
}


void HashInitMD5(THash *Hash)
{
Hash->Ctx=(void *) calloc(1,sizeof(MD5_CTX));
MD5Init((MD5_CTX *) Hash->Ctx);
Hash->Update=HashUpdateMD5;
Hash->Finish=HashFinishMD5;
}

#include "sha1.h"
#define SHA1LEN 20

void HashUpdateSHA1(THash *Hash, char *Data, int Len)
{
sha1_process_bytes(Data,Len,(struct sha1_ctx *) Hash->Ctx);
}


int HashFinishSHA1(THash *Hash, int Encoding, char **HashStr)
{
int count, len;
char *Tempstr=NULL, *DigestBuff=NULL;

DigestBuff=(char *) calloc(1,SHA1LEN+1);
sha1_finish_ctx((struct sha1_ctx *) Hash->Ctx, DigestBuff);
free(Hash->Ctx);

if (Encoding > 0)
{
	 *HashStr=EncodeHash(*HashStr, DigestBuff, SHA1LEN, Encoding);
	 len=StrLen(*HashStr);
}
else
{
len=SHA1LEN;
*HashStr=SetStrLen(*HashStr,len);
memcpy(*HashStr,DigestBuff,len);
}

DestroyString(DigestBuff);
DestroyString(Tempstr);

return(len);
}


void HashInitSHA1(THash *Hash)
{
Hash->Ctx=(void *) calloc(1,sizeof(struct sha1_ctx));
sha1_init_ctx((struct sha1_ctx *) Hash->Ctx);
Hash->Update=HashUpdateSHA1;
Hash->Finish=HashFinishSHA1;
}


#include "sha2.h"

int HashFinishSHA256(THash *Hash, int Encoding, char **HashStr)
{
int count, len;
char *Tempstr=NULL;
uint8_t *DigestBuff=NULL;

DigestBuff=(uint8_t *) calloc(1,SHA256_DIGEST_LENGTH+1);
SHA256_Final(DigestBuff, (SHA256_CTX *) Hash->Ctx);
free(Hash->Ctx);

if (Encoding > 0)
{
	 *HashStr=EncodeHash(*HashStr, DigestBuff, SHA256_DIGEST_LENGTH, Encoding);
	 len=StrLen(*HashStr);
}
else
{
len=SHA256_DIGEST_LENGTH;
*HashStr=SetStrLen(*HashStr,len);
memcpy(*HashStr,DigestBuff,len);
}

DestroyString(DigestBuff);
DestroyString(Tempstr);

return(len);
}


void HashUpdateSHA256(THash *Hash, char *Data, int Len)
{
SHA256_Update((SHA256_CTX *) Hash->Ctx, Data, Len);
}

void HashInitSHA256(THash *Hash)
{
Hash->Ctx=(void *) calloc(1,sizeof(SHA256_CTX));
SHA256_Init((SHA256_CTX *) Hash->Ctx);
Hash->Update=HashUpdateSHA256;
Hash->Finish=HashFinishSHA256;
}


int HashFinishSHA512(THash *Hash, int Encoding, char **HashStr)
{
int count, len;
char *Tempstr=NULL, *DigestBuff=NULL;

DigestBuff=(char *) calloc(1,SHA512_DIGEST_LENGTH+1);
SHA512_Final(DigestBuff, (SHA512_CTX *) Hash->Ctx);
free(Hash->Ctx);

if (Encoding > 0)
{
	 *HashStr=EncodeHash(*HashStr, DigestBuff, SHA512_DIGEST_LENGTH, Encoding);
	 len=StrLen(*HashStr);
}
else
{
len=SHA512_DIGEST_LENGTH;
*HashStr=SetStrLen(*HashStr,len);
memcpy(*HashStr,DigestBuff,len);
}

DestroyString(DigestBuff);
DestroyString(Tempstr);

return(len);
}


void HashUpdateSHA512(THash *Hash, char *Data, int Len)
{
SHA512_Update((SHA512_CTX *) Hash->Ctx, Data, Len);
}

void HashInitSHA512(THash *Hash)
{
Hash->Ctx=(void *) calloc(1,sizeof(SHA512_CTX));
SHA512_Init((SHA512_CTX *) Hash->Ctx);
Hash->Update=HashUpdateSHA512;
Hash->Finish=HashFinishSHA512;
}



void HashDestroy(THash *Hash)
{
//Hash->Ctx is destroyed in 'HashFinish'
DestroyString(Hash->Key1);
DestroyString(Hash->Key2);
DestroyString(Hash->Type);
free(Hash);
}



THash *HashInit(char *Type)
{
THash *Hash=NULL;

Hash=(THash *) calloc(1,sizeof(THash));
Hash->Type=CopyStr(Hash->Type,Type);
if (strcasecmp(Type,"md5")==0) HashInitMD5(Hash);
else if (strcasecmp(Type,"sha")==0) HashInitSHA1(Hash);
else if (strcasecmp(Type,"sha1")==0) HashInitSHA1(Hash);
else if (strcasecmp(Type,"sha256")==0) HashInitSHA256(Hash);
else if (strcasecmp(Type,"sha512")==0) HashInitSHA512(Hash);
else if (strcasecmp(Type,"crc32")==0) HashInitCRC(Hash);
else if (strncasecmp(Type,"hmac-",5)==0) HMACInit(Hash);
else 
{
	HashDestroy(Hash);
	Hash=NULL;
}

return(Hash);
}


int HashBytes(char **Return, char *Type, char *text, int len, int Encoding)
{
THash *Hash;
int result;

Hash=HashInit(Type);
if (! Hash) return(0);
Hash->Update(Hash, text, len);
result=Hash->Finish(Hash, Encoding, Return);
HashDestroy(Hash);

return(result);
}


int HashFile(char **Return, char *Type, char *Path, int Encoding)
{
THash *Hash;
STREAM *S;
char *Tempstr=NULL;
int result;

S=STREAMOpenFile(Path,O_RDONLY);
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

result=Hash->Finish(Hash, Encoding, Return);
HashDestroy(Hash);

return(result);
}

