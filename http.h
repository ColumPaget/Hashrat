
#ifndef HASHRAT_HTTP
#define HASHRAT_HTTP

#include "common.h"

int HTTPGlob(HashratCtx *Ctx, const char *Path, ListNode *Dirs);
int HTTPStat(HashratCtx *Ctx, const char *Path, struct stat *FStat);


#endif
