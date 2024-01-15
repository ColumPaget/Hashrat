
#ifndef HASHRAT_COMMON_H
#define HASHRAT_COMMON_H

#include "libUseful-5/libUseful.h"
#include "glob.h"

#define VERSION "1.19"

#define ACT_NONE 0
#define ACT_HASH 1
#define ACT_HASHDIR 2
#define ACT_HASH_LISTFILE 3
#define ACT_PRINTUSAGE 4
#define ACT_CGI 6
#define ACT_SIGN 7
#define ACT_CHECKSIGN 8
#define ACT_XDIALOG 9
#define ACT_CHECK       10
#define ACT_CHECK_LIST  11
#define ACT_CHECK_MEMCACHED 12
#define ACT_CHECK_XATTR 13
#define ACT_LOADMATCHES 20
#define ACT_FINDMATCHES 21
#define ACT_FINDMATCHES_MEMCACHED 22
#define ACT_FINDDUPLICATES 23
#define ACT_BACKUP 24
#define ACT_CHECKBACKUP 25
#define ACT_OTP 26

#define FLAG_NEXTARG 1
//Two flags with the same values, but used in different contexts
#define FLAG_ERROR 2
#define FLAG_VERBOSE 2
//---------------------------
#define FLAG_DEVMODE         8
#define FLAG_OUTPUT_FAILS   16 
#define FLAG_DIR_INFO       32
#define FLAG_TRAD_OUTPUT    64
#define FLAG_BSD_OUTPUT    128 
#define FLAG_XATTR         512
#define FLAG_TXATTR       1024
#define FLAG_MEMCACHED    2048
#define FLAG_XSELECT      4096 
#define FLAG_CLIPBOARD    8192 
#define FLAG_QRCODE      16384 
#define FLAG_FULLCHECK   32768
#define FLAG_HMAC        65536
#define FLAG_HIDE_INPUT 131072
#define FLAG_STAR_INPUT 262144
#define FLAG_LINEMODE  1048576
#define FLAG_RAW       2097152
#define FLAG_COLOR     4194304
#define FLAG_NET       8388608
#define FLAG_UPDATE   16777216

//map cgi options to console-mode options that make no sense in .cgi mode
#define CGI_DOHASH FLAG_NEXTARG
#define CGI_NOOPTIONS FLAG_TRAD_OUTPUT
#define CGI_HIDETEXT  FLAG_BSD_OUTPUT
#define CGI_SHOWTEXT  FLAG_XSELECT

//the first few of these must be low because they will go in a 16-bit value in
//the 'include/exclude' list
#define CTX_INCLUDE 2
#define CTX_EXCLUDE 4
#define CTX_MTIME   8
#define CTX_MMIN    16
#define CTX_MYEAR    32
#define CTX_RECURSE  128
#define CTX_HASH_AND_RECURSE  256
#define CTX_DEREFERENCE 512
#define CTX_XATTR 1024
#define CTX_MEMCACHED 2048
#define CTX_STORE_XATTR 4096
#define CTX_STORE_MEMCACHED 8192
#define CTX_STORE_FILE  16384
#define CTX_XATTR_ROOT  32768
#define CTX_XATTR_CACHE 65536
#define CTX_EXES  131072          
#define CTX_CACHED 262144
#define CTX_ONE_FS 524288
#define CTX_HIDDEN 1048576
#define CTX_REFORMAT 2097152


#define RESULT_PASS 1
#define RESULT_FAIL 2
#define RESULT_WARN 4
#define RESULT_LOCATED 8
#define FLAG_RESULT_MASK 15
#define RESULT_RUNHOOK 8192



#define FP_HASSTAT 1

#define BLOCKSIZE 4096

#define IGNORE -1



typedef struct
{
int Flags;
char *Path;
char *Hash;
char *HashType;
char *Data;
struct stat FStat;
void *Next;
} TFingerprint;


typedef struct
{
STREAM *NetCon;
STREAM *Out; 
STREAM *Aux; 
HASH *Hash;
char *HashType;
char *HashStr;
char *Targets;
int Action;
int Flags;
int Encoding;  
int OutputLength;
int SegmentLength;
int SegmentChar;
dev_t StartingFS;
ListNode *Vars;
} HashratCtx;


extern int Flags;
extern char *DiffHook;
extern char *Key;
extern char *LocalHost;
extern const char *HashratHashTypes[];
extern ListNode *IncludeExclude;
extern int MatchCount, DiffCount;
extern time_t Now;
extern uint64_t HashStartTime;

TFingerprint *TFingerprintCreate(const char *Hash, const char *HashType, const char *Data, const char *Path);
void HashratCtxDestroy(void *p_Ctx);
void HashratStoreHash(HashratCtx *Ctx, const char *Path, struct stat *Stat, const char *Hash);
int HashratOutputFileInfo(HashratCtx *Ctx, STREAM *S, const char *Path, struct stat *Stat, const char *Hash);
void RunHookScript(const char *Hook, const char *Path, const char *Other);
char *ReformatHash(char *RetStr, const char *Str, HashratCtx *Ctx);

#endif
