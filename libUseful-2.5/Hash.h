#ifndef LIBUSEFUL_HASH_H
#define LIBUSEFUL_HASH_H

#include "file.h"
#include "includes.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_hash THash;

typedef void (*HASH_UPDATE)(THash *Hash, char *Data, int DataLen);
typedef THash *(*HASH_CLONE)(THash *Hash);
typedef int (*HASH_FINISH)(THash *Hash, int Encoding, char **RetStr);

struct t_hash
{
char *Type;
char *Key1;
unsigned int Key1Len;
char *Key2;
unsigned int Key2Len;
void *Ctx;
HASH_UPDATE Update;
HASH_FINISH Finish;
HASH_CLONE Clone;
};

void HashAvailableTypes(ListNode *Vars);
THash *HashInit(char *Type);
void HMACSetKey(THash *HMAC, char *Key, int Len);
void HashDestroy(THash *Hash);
int HashBytes(char **Return, char *Type, char *text, int len, int Encoding);
int HashFile(char **Return, char *Type, char *Path, int Encoding);
int PBK2DF2(char **Return, char *Type, char *Bytes, int Len, char *Salt, int SaltLen, uint32_t Rounds, int Encoding);

#ifdef __cplusplus
}
#endif



#endif
