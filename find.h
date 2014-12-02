
#ifndef HASHRAT_FIND_H
#define HASHRAT_FIND_H

#include "common.h"

void LoadMatchesToMemcache();
int CheckForMatch(HashratCtx *Ctx, char *Path, struct stat *FStat, char *HashStr);


#endif
