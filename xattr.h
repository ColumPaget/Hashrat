#ifndef HASHRAT_XATTR_H
#define HASHRAT_XATTR_H

#include "common.h"

void SetupXAttrList(const char *Arg);
TFingerprint *XAttrLoadHash(HashratCtx *Ctx, const char *Path);
int XAttrGetHash(HashratCtx *Ctx, const char *XattrType, const char *HashType, const char *Path, struct stat *FStat, char **Hash);
void HashRatSetXAttr(HashratCtx *Ctx, const char *Path, struct stat *Stat, const char *HashType, const char *Hash);

#endif
