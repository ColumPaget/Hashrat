
#ifndef HASHRAT_COMMON_H
#define HASHRAT_COMMON_H

#include "libUseful-2.0/libUseful.h"
#include "glob.h"


#define ACT_NONE 0
#define ACT_HASH 1
#define ACT_CHECK 2
#define ACT_HASH_LISTFILE 3
#define ACT_PRINTUSAGE 4
#define ACT_XATTR 5
#define ACT_CGI 6

#define FLAG_RECURSE 1
#define FLAG_VERBOSE 2
#define FLAG_DIRMODE 4
#define FLAG_DEVMODE 8
#define FLAG_CHECK   16
#define FLAG_STDIN	 32
#define FLAG_ONE_FS  64
#define FLAG_DIR_INFO 128
#define FLAG_XATTR   256
#define FLAG_FROM_LISTFILE 512
#define FLAG_OUTPUT_FAILS 1024 
#define FLAG_TRAD_OUTPUT 2048 
#define FLAG_INCLUDE 4096
#define FLAG_EXCLUDE 8192
#define FLAG_DEREFERENCE 16384
#define FLAG_HMAC 32768
#define FLAG_ARG_NAMEVALUE 131072
#define FLAG_FULLCHECK 262144
#define FLAG_NET 524288
#define FLAG_LINEMODE 1048576
#define FLAG_RAW 2097152
#define FLAG_BASE8  4194304
#define FLAG_BASE10 8388608
#define FLAG_BASE64 16777216
#define FLAG_HEXUPPER 33554432

#define FP_HASSTAT 1

#define BLOCKSIZE 4096

#define VERSION "0.1"


typedef struct
{
int Flags;
char *Path;
char *Hash;
char *HashType;
struct stat FStat;
} TFingerprint;


typedef struct
{
STREAM *NetCon;
STREAM *Out; 
THash *Hash;
char *HashType;
char *ListPath;
char *HashStr;
int Action;
int Encoding;
ListNode *Vars;
} HashratCtx;


extern int Flags;
extern char *DiffHook;
extern char *Key;

void HashratCtxDestroy(void *p_Ctx);
int HashratCheckFile(HashratCtx *Ctx,TFingerprint *FP);
int HashratAction(HashratCtx *Ctx, char *Path, struct stat *Stat, char *Hash);


#endif
