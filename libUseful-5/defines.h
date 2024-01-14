/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_DEFINES_H
#define LIBUSEFUL_DEFINES_H

//this file is for general, globally defined values like 'TRUE' and 'FALSE'

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


#ifdef __cplusplus
extern "C" {
#endif

//this is a basic call-back function type that gets used in a number of places
typedef int (*BASIC_FUNC)(void *Data, int Flags);


#ifdef __cplusplus
}
#endif

#endif
