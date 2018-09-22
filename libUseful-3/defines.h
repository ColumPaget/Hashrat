/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_DEFINES_H
#define LIBUSEFUL_DEFINES_H

#define FALSE 0
#define TRUE  1

#define FTIMEOUT -2


#define SUBS_QUOTE_VARS 1
#define SUBS_CASE_VARNAMES 2
#define SUBS_STRIP_VARS_WHITESPACE 4
#define SUBS_INTERPRET_BACKSLASH   8
#define SUBS_QUOTES  16


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*BASIC_FUNC)(void *Data, int Flags);


#ifdef __cplusplus
}
#endif

#endif
