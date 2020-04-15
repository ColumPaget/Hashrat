#ifndef LIBUSEFUL_UNICODE_H
#define LIBUSEFUL_UNICODE_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

void UnicodeSetUTF8(int level);
unsigned int UnicodeDecode(const char **ptr);
char *UnicodeStr(char *RetStr, int Code);
char *StrAddUnicodeChar(char *RetStr, int uchar);

#ifdef __cplusplus
}
#endif

#endif

