#ifndef LIBUSEFUL_DEFINES_H
#define LIBUSEFUL_DEFINES_H

#define FALSE 0
#define TRUE  1

#define FTIMEOUT -2


#define ENCODE_NONE 0
#define ENCODE_OCTAL 1
#define ENCODE_DECIMAL 2
#define ENCODE_HEX  3
#define ENCODE_HEXUPPER 4
#define ENCODE_BASE64 5


#define SUBS_QUOTE_VARS 1
#define SUBS_CASE_VARNAMES 2
#define SUBS_STRIP_VARS_WHITESPACE 4

#define HEX_CHARS "0123456789ABCDEF"
#define ALPHA_CHARS "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*BASIC_FUNC)(void *Data); 


#ifdef __cplusplus
}
#endif

#endif
