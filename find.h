
#ifndef HASHRAT_FIND_H
#define HASHRAT_FIND_H

#include "common.h"


void *MatchesLoad(HashratCtx *Ctx, int Flags);
void LoadMatchesToMemcache();
void OutputUnmatched(HashratCtx *Ctx);
int MatchAdd(TFingerprint *FP, const char *Path, int Flags);
TFingerprint *CheckForMatch(HashratCtx *Ctx, const char *Path, struct stat *FStat, const char *HashStr);


#endif
