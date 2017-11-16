
#ifndef HASHRAT_SSH_H
#define HASHRAT_SSH_H

#include "common.h"

STREAM *SSHGet(HashratCtx *Ctx, const char *URL);
int SSHGlob(HashratCtx *Ctx, const char *URL, ListNode *Files);

#endif

