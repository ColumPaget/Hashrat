#ifndef HASHRAT_FINGERPRINT_H
#define HASHRAT_FINGERPRINT_H

#include "common.h"

void FingerprintDestroy(void *p_FP);
TFingerprint *FingerprintRead(STREAM *S);
int FingerprintCompare(const void *v1, const void *v2);

#endif

