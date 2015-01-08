
#ifndef HASHRAT_FIND_H
#define HASHRAT_FIND_H

#include "common.h"


void *MatchesLoad();
void LoadMatchesToMemcache();
void MatchAdd(const char *Hash, const char *HashType, const char *Data, const char *Path);
TFingerprint *CheckForMatch(HashratCtx *Ctx, char *Path, struct stat *FStat, char *HashStr);


#endif
