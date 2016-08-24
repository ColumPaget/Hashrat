#ifndef LIBUSEFUL_TOKEN_H
#define LIBUSEFUL_TOKEN_H

#include <stdarg.h>
#include <string.h>

#define GETTOKEN_MULTI_SEPARATORS 1
#define GETTOKEN_MULTI_SEP 1
#define GETTOKEN_HONOR_QUOTES 2
#define GETTOKEN_STRIP_QUOTES 4
#define GETTOKEN_QUOTES 6
#define GETTOKEN_INCLUDE_SEPARATORS 8
#define GETTOKEN_INCLUDE_SEP 8
#define GETTOKEN_APPEND_SEPARATORS 16
#define GETTOKEN_APPEND_SEP 16
#define GETTOKEN_BACKSLASH  32
#define GETTOKEN_STRIP_SPACE 64
#define GETTOKEN_NOEMMS   4096

#ifdef __cplusplus
extern "C" {
#endif

char *GetToken(const char *SearchStr, const char *Delim, char **Token, int Flags);
char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value);

#ifdef __cplusplus
}
#endif




#endif

