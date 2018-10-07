
#ifndef HASHRAT_INCLUDE_EXCLUDE_H
#define HASHRAT_INCLUDE_EXCLUDE_H

#include "common.h"

void IncludeExcludeAdd(HashratCtx *Ctx, int Type, const char *Item);
void IncludeExcludeLoadExcludesFromFile(const char *Path, HashratCtx *Ctx);
int IncludeExcludeCheck(HashratCtx *Ctx, const char *Path, struct stat *FStat);

#endif
