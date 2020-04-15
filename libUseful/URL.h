/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_URL
#define LIBUSEFUL_URL

#include "includes.h"

/*
These functions break a URL of the form <protocol>://<user>:<password>@<host>:<port>/<path>?<args> into its component parts

In general any of the destination arguments can be NULL. So, if you don't want user and password, just set the return args
for those to NULL
*/

#ifdef __cplusplus
extern "C" {
#endif

//Parse a full URL with all parts. You can just use this for everything.
void ParseURL(const char *URL, char **Proto, char **Host, char **Port, char **User, char **Password, char **Path, char **Args);

//Parse a URL type that lacks a protocol or Args part, something like "myhost.com:22"
const char *ParseHostDetails(const char *Data,char **Host,char **Port,char **User, char **Password);

void ParseConnectDetails(const char *Str, char **Proto, char **Host, char **Port, char **User, char **Pass, char **InitDir);
char *ResolveURL(char *RetStr, const char *Parent, const char *SubItem);

#ifdef __cplusplus
}
#endif

#endif
