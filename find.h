
#ifndef HASHRAT_FIND_H
#define HASHRAT_FIND_H

#include "common.h"


void *MatchesLoad();
void LoadMatchesToMemcache();
int MatchAdd(TFingerprint *FP, const char *Path, int Flags);
TFingerprint *CheckForMatch(HashratCtx *Ctx, const char *Path, struct stat *FStat, const char *HashStr);


#endif
