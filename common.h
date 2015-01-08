
#ifndef HASHRAT_COMMON_H
#define HASHRAT_COMMON_H

#include "libUseful-2.0/libUseful.h"
#include "glob.h"


#define ACT_NONE 0
#define ACT_HASH 1
#define ACT_HASH_LISTFILE 3
#define ACT_PRINTUSAGE 4
#define ACT_CGI 6
#define ACT_SIGN 7
#define ACT_CHECKSIGN 8
#define ACT_CHECK       10
#define ACT_CHECK_XATTR 11
#define ACT_CHECK_MEMCACHED 12
#define ACT_LOADMATCHES 20
#define ACT_FINDMATCHES 21
#define ACT_FINDMATCHES_MEMCACHED 22
#define ACT_FINDDUPLICATES 23

#define FLAG_RECURSE 1
//Two flags with the same values, but used in different contexts
#define FLAG_ERROR 2
#define FLAG_VERBOSE 2
#define FLAG_DIRMODE 4
#define FLAG_DEVMODE 8
#define FLAG_ONE_FS  64
#define FLAG_DIR_INFO 128
#define FLAG_MEMCACHED  1024
#define FLAG_OUTPUT_FAILS 2048 
#define FLAG_TRAD_OUTPUT 4096 
#define FLAG_INCLUDE 8192
#define FLAG_EXCLUDE 16384
#define FLAG_HMAC 65536
#define FLAG_FULLCHECK 262144
#define FLAG_LINEMODE 1048576
#define FLAG_RAW 2097152
#define FLAG_COLOR 4194304
#define FLAG_NET 8388608
#define FLAG_UPDATE 16777216


#define CTX_CACHED		1
#define CTX_BASE10		4
#define CTX_BASE8			8
#define CTX_HEX				16
#define CTX_HEXUPPER	32
#define CTX_BASE64 		64
#define CTX_DEREFERENCE 128
#define CTX_STORE_XATTR 256
#define CTX_STORE_MEMCACHED 512
#define CTX_STORE_FILE 1024
#define CTX_XATTR_ROOT 2048
#define CTX_EXES 4096

#define RESULT_PASS 1
#define RESULT_FAIL 2
#define RESULT_WARN 4
#define RESULT_LOCATED 8
#define FLAG_RESULT_MASK 15
#define RESULT_RUNHOOK 8192

#define FP_HASSTAT 1

#define BLOCKSIZE 4096

#define VERSION "1.4.1"


typedef struct
{
int Flags;
char *Path;
char *Hash;
char *HashType;
char *Data;
struct stat FStat;
} TFingerprint;


typedef struct
{
STREAM *NetCon;
STREAM *Out; 
STREAM *Aux; 
THash *Hash;
char *HashType;
char *ListPath;
char *HashStr;
int Action;
int Flags;
ListNode *Vars;
} HashratCtx;


extern int Flags;
extern char *DiffHook;
extern char *Key;
extern char *LocalHost;
extern char *HashratHashTypes[];
extern ListNode *IncludeExclude;


TFingerprint *TFingerprintCreate(const char *Hash, const char *HashType, const char *Data, const char *Path);
void HashratCtxDestroy(void *p_Ctx);
void HashratStoreHash(HashratCtx *Ctx, char *Path, struct stat *Stat, char *Hash);
int HashratOutputInfo(HashratCtx *Ctx, STREAM *S, char *Path, struct stat *Stat, char *Hash);
void HandleCompareResult(char *Path, char *Status, int Flags, char *ErrorMessage);

#endif
