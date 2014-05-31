#ifndef HASHRAT_XATTR_H
#define HASHRAT_XATTR_H

#include "common.h"

void SetupXAttrList(char *Arg);
void HashRatSetXAttr(STREAM *Out, char *Path, struct stat *Stat, char *HashType, char *Hash);
void XAttrLoadHash(TFingerprint *FP);

#endif
