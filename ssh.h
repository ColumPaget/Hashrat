
#ifndef HASHRAT_SSH_H
#define HASHRAT_SSH_H

#include "common.h"

STREAM *SSHGet(HashratCtx *Ctx, char *URL);
int SSHGlob(HashratCtx *Ctx, char *URL, ListNode *Files);

#endif

