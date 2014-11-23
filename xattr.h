#ifndef HASHRAT_XATTR_H
#define HASHRAT_XATTR_H

#include "common.h"

void SetupXAttrList(char *Arg);
TFingerprint *XAttrLoadHash(HashratCtx *Ctx, char *Path);
int XAttrGetHash(HashratCtx *Ctx, char *XattrType, char *HashType, char *Path, struct stat *FStat, char **Hash);
void HashRatSetXAttr(STREAM *Out, char *Path, struct stat *Stat, char *HashType, char *Hash);

#endif
