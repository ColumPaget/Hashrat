#ifndef HASHRAT_CHECK_H
#define HASHRAT_CHECK_H

#include "common.h"


int CheckHashesFromList(HashratCtx *Ctx);
void HandleCheckFail(char *Path, char *ErrorMessage);
int HashratCheckFile(HashratCtx *Ctx, char *Path, struct stat *ExpectedStat, struct stat *ActualStat, char *ExpectedHash, char *ActualHash);

#endif
