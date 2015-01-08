#ifndef HASHRAT_FINGERPRINT_H
#define HASHRAT_FINGERPRINT_H

#include "common.h"



void TFingerprintDestroy(void *p_Fingerprint);
TFingerprint *TFingerprintCreate(const char *Hash, const char *HashType, const char *Data, const char *Path);
TFingerprint *FingerprintRead(STREAM *S);
int FingerprintCompare(const void *v1, const void *v2);

#endif

