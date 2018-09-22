#ifndef HASHRAT_FINGERPRINT_H
#define HASHRAT_FINGERPRINT_H

#include "common.h"

/*
The 'fingerprint' concept here is the combination of a path, its hash, the hash type, etc.
*/


void TFingerprintDestroy(void *p_Fingerprint);
TFingerprint *TFingerprintCreate(const char *Hash, const char *HashType, const char *Data, const char *Path);
TFingerprint *FingerprintRead(STREAM *S);
TFingerprint *TFingerprintParse(const char *Data);
int FingerprintCompare(const void *v1, const void *v2);

#endif

