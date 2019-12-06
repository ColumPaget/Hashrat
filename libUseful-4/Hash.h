/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_HASH_H
#define LIBUSEFUL_HASH_H

/* Hashing functions. Mostly you'll use:

len=HashBytes(&Hashed, "md5", "string to hash", 14, ENCODE_HEX);

or

len=HashFile(&Hashed, "sha1", "/tmp/filetohash.txt", ENCODE_BASE64);

You can get a list of available hash types with HashAvailableTypes. This should be MD5, various SHA variants, whirlpool, and variants of the jh hash
from Dr Hongjun Wu. More hash algorithms will likely be added in future.


if you need more granulated control of hashing, then it's done like this

Hash=HashInit("md5");
S=STREAMOpen("/tmp/testfile.txt","r");
result=STREAMReadBytes(S, Buffer BUFSIZ);
while (result > 0)
{
HashUpdate(Hash, Buffer, result);
result=STREAMReadBytes(S, Buffer BUFSIZ);
}
len=HashFinish(Hash, ENCODE_BASE64, HashStr);

printf("Hash was: %s\n",HashStr);

*/



//if you load Hash.h you'll want Encodings.h too
#include "Encodings.h"
#include "Stream.h"
#include "includes.h"

#define HashUpdate(Hash, text, len) (Hash->Update(Hash, text, len))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_hash HASH;

typedef void (*HASH_UPDATE)(HASH *Hash, const char *Data, int DataLen);
typedef HASH *(*HASH_CLONE)(HASH *Hash);
typedef int (*HASH_FINISH)(HASH *Hash, char **RetStr);

struct t_hash
{
    char *Type;
//this is a placeholder used only by classlibraries for scripting langauges
    int Encoding;
    char *Key1;
    unsigned int Key1Len;
    char *Key2;
    unsigned int Key2Len;
    void *Ctx;
    HASH_UPDATE Update;
    HASH_FINISH Finish;
    HASH_CLONE Clone;
};

char *HashAvailableTypes(char *RetStr);
int HashEncodingFromStr(const char *Str);
HASH *HashInit(const char *Type);
int HashFinish(HASH *Hash, int Encoding, char **Return);
void HMACSetKey(HASH *HMAC, const char *Key, int Len);
void HashDestroy(HASH *Hash);
int HashBytes(char **Return, const char *Type, const char *text, int len, int Encoding);
int HashBytes2(const char *Type, int Encoding, const char *text, int len, char **RetStr);
int HashSTREAM(char **Return, const char *Type, STREAM *S, int Encoding);
int HashFile(char **Return, const char *Type, const char *Path, int Encoding);
int PBK2DF2(char **Return, char *Type, char *Bytes, int Len, char *Salt, int SaltLen, uint32_t Rounds, int Encoding);

#ifdef __cplusplus
}
#endif



#endif
