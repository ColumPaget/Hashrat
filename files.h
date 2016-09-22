
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



int StatFile(HashratCtx *Ctx, char *Path, struct stat *Stat);
int HashSingleFile(char **RetStr, HashratCtx *Ctx, int Type,char *Path);
void ProcessData(char **RetStr, HashratCtx *Ctx, char *Data, int DataLen);
int HashItem(HashratCtx *Ctx, char *HashType, char *Path, struct stat *FStat, char **HashStr);
int ProcessItem(HashratCtx *Ctx, char *Path, struct stat *Stat);
int ProcessDir(HashratCtx *Ctx, char *Dir, char *HashType);

void HashratFinishHash(char **RetStr, HashratCtx *Ctx, THash *Hash);
int HashratHashFile(HashratCtx *Ctx, THash *Hash, int Type, char *Path, off_t FileSize);
int HashratHashSingleFile(HashratCtx *Ctx, char *HashType, int FileType, char *Path, struct stat *FStat, char **RetStr);

#endif
