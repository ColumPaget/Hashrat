
#ifndef HASHRAT_FILES_H
#define HASHRAT_FILES_H

#include "common.h"

//if first item added to Include/Exclude is an include
//then the program will exclude by default
void AddIncludeExclude(int Type, const char *Item);
char *HashSingleFile(char *RetStr, HashratCtx *Ctx, int Type,char *Path);
char *ProcessData(char *RetStr, HashratCtx *Ctx, char *Data, int DataLen);
int HashItem(HashratCtx *Ctx, char *Path, struct stat *FStat, char **HashStr);
void ProcessItem(HashratCtx *Ctx, char *Path, struct stat *Stat);

#endif
