
#ifndef HASHRAT_FILESIGN_H
#define HASHRAT_FILESIGN_H

#include "common.h"

void HashratSignFile(char *Path, HashratCtx *Ctx);
int HashratCheckSignedFile(char *Path, HashratCtx *Ctx);


#endif
