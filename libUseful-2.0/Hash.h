#ifndef LIBUSEFUL_HASH_H
#define LIBUSEFUL_HASH_H

#include "file.h"
#include "includes.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct t_hash THash;

typedef void (*HASH_UPDATE)(THash *Hash, char *Data, int DataLen);
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
};

void HashAvailableTypes(ListNode *Vars);
THash *HashInit(char *Type);
void HMACSetKey(THash *HMAC, char *Key, int Len);
void HashDestroy(THash *Hash);
char *EncodeBase64(char *Return, char *Text, int len);
char *DecodeBase64(char *Return, int *len, char *Text);
int HashBytes(char **Return, char *Type, char *text, int len, int Encoding);
int HashFile(char **Return, char *Type, char *Path, int Encoding);

#ifdef __cplusplus
}
#endif



#endif
