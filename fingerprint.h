#ifndef HASHRAT_FINGERPRINT_H
#define HASHRAT_FINGERPRINT_H

#include "common.h"

void FingerprintDestroy(void *p_FP);
int FingerprintRead(STREAM *S, TFingerprint *FP);
int FingerprintCompare(const void *v1, const void *v2);

#endif

