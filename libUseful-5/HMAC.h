/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_HMAC_H
#define LIBUSEFUL_HMAC_H

#include "Hash.h"

#ifdef __cplusplus
extern "C" {
#endif

void HMACUpdate(HASH *HMAC, const char *Data, int Len);
int HMACFinish(HASH *HMAC, char **HashStr);
void HMACPrepare(HASH *HMAC, const char *Data, int Len);
HASH *HMACInit(const char *Type);
void HMACSetKey(HASH *HMAC, const char *Key, int Len);
void HMACDestroy(void *p_HMAC);
int HMACBytes(char **RetStr, const char *Type, const char *Key, int KeyLen, const char *Data, int DataLen, int Encoding);


#ifdef __cplusplus
}
#endif


#endif
