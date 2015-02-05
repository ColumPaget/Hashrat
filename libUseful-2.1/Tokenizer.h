#ifndef LIBUSEFUL_TOKEN_H
#define LIBUSEFUL_TOKEN_H

#include <stdarg.h>
#include <string.h> //for strlen, used below in StrLen

#define GETTOKEN_MULTI_SEPARATORS 1
#define GETTOKEN_HONOR_QUOTES 2
#define GETTOKEN_STRIP_QUOTES 4
#define GETTOKEN_QUOTES 6

#ifdef __cplusplus
extern "C" {
#endif

char *GetToken(const char *SearchStr, const char *Delim, char **Token, int Flags);
char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value);

#ifdef __cplusplus
}
#endif




#endif

