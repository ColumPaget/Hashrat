#ifndef HASHRAT_CHECK_H
#define HASHRAT_CHECK_H

#include "common.h"


int CheckHashesFromList(HashratCtx *Ctx);
void HandleCheckFail(char *Path, char *ErrorMessage);
int HashratCheckFile(HashratCtx *Ctx, char *Path, char *ExpectedHash, struct stat *FPStat);

#endif
