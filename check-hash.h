#ifndef HASHRAT_CHECK_H
#define HASHRAT_CHECK_H

#include "common.h"


int CheckHashesFromList(HashratCtx *Ctx);
void HandleCheckFail(const char *Path, const char *ErrorMessage);
int HashratCheckFile(HashratCtx *Ctx, const char *Path, struct stat *ActualStat, const char *ActualHash, TFingerprint *FP);

#endif
