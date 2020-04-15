/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_DEFINES_H
#define LIBUSEFUL_DEFINES_H

#ifndef __cplusplus
//this mostly exists for libUseful-lua, so that swig can recognize functions that return boolean values
//we don't want to define this if we're using libUseful under c++ though, as c++ already has a boolean
//datatype, and if libUseful is the first thing included before the c++ standard includes, then it will
//go ahead and define it's own bool type
#define bool int
#endif

#define FALSE 0
#define TRUE  1

#define FTIMEOUT -2


//these are used by SubstituteVarsInString and will probably move to a more appropriate place one day
#define SUBS_QUOTE_VARS 1
#define SUBS_CASE_VARNAMES 2
#define SUBS_STRIP_VARS_WHITESPACE 4
#define SUBS_INTERPRET_BACKSLASH   8
#define SUBS_QUOTES  16
#define SUBS_SHELL_SAFE  32


#ifdef __cplusplus
extern "C" {
#endif

//this is a basic call-back function type that gets used in a number of places
typedef int (*BASIC_FUNC)(void *Data, int Flags);


#ifdef __cplusplus
}
#endif

#endif
