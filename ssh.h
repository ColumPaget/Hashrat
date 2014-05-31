
#ifndef HASHRAT_SSH_H
#define HASHRAT_SSH_H

#include "common.h"

//STREAM *SSHConnect(char *URL, char *IDFile, char **Path);
int SSHGlob(HashratCtx *Ctx, char *URL, ListNode *Files);
STREAM *SSHGet(HashratCtx *Ctx, char *URL);

#endif

