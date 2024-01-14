/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_MARKUP_H
#define LIBUSEFUL_MARKUP_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

// These functions act on XML and HTML style markup documents

//read the next XML/HTML tag from memory. extract the namespace, tag type and Tag arguments and return a pointer to the
//memory just after the tag. Namespace, TagType and TagData must be pointers to libUseful style strings
const char *XMLGetTag(const char *Input, char **Namespace, char **TagType, char **TagData);

//copy Str to RetBuff, applying HTML quoting as we go. RetBuff must be a libUseful style string
char *HTMLQuote(char *RetBuff, const char *Str);

//copy Str to RetBuff, expanding HTML quoting as we go. RetBuff must be a libUseful style string
char *HTMLUnQuote(char *RetStr, const char *Str);

#ifdef __cplusplus
}
#endif



#endif
