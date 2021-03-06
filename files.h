
#ifndef HASHRAT_FILES_H
#define HASHRAT_FILES_H

#include "common.h"

#define FT_FILE 0
#define FT_DIR 1
#define FT_LNK 2
#define FT_BLK 4
#define FT_CHR 8
#define FT_SOCK 16
#define FT_FIFO 32
#define FT_STDIN 64
#define FT_HTTP 256
#define FT_SSH  512



int StatFile(HashratCtx *Ctx, const char *Path, struct stat *Stat);
void GlobFiles(HashratCtx *Ctx, const char *Path, int FType, ListNode *Dirs);
int HashSingleFile(char **RetStr, HashratCtx *Ctx, int Type,char *Path);
void ProcessData(char **RetStr, HashratCtx *Ctx, const char *Data, int DataLen);
int HashItem(HashratCtx *Ctx, const char *HashType, const char *Path, struct stat *FStat, char **HashStr);
int ProcessItem(HashratCtx *Ctx, const char *Path, struct stat *Stat, int IsTopLevel);

void HashratFinishHash(char **RetStr, HashratCtx *Ctx, HASH *Hash);
int HashratHashFile(HashratCtx *Ctx, HASH *Hash, int Type, const char *Path, struct stat *FStat);
int HashratHashSingleFile(HashratCtx *Ctx, const char *HashType, int FileType, const char *Path, struct stat *FStat, char **RetStr);

#endif
