/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_URL_H
#define LIBUSEFUL_URL_H

#include "includes.h"

/*
These functions break a URL of the form <protocol>://<user>:<password>@<host>:<port>/<path>?<args> into its component parts

In general any of the destination arguments can be NULL. So, if you don't want user and password, just set the return args
for those to NULL
*/

#ifdef __cplusplus
extern "C" {
#endif


//Parse a full URL with all parts. DO NOT TRY TO GUESS PORT if no port in the URL itself
void UnpackURL(const char *URL, char **Proto, char **Host, char **Port, char **User, char **Password, char **Path, char **Args);


//Parse a full URL with all parts. Guess port for well known protocols (so https://myhost/ returns a port of 80)
void ParseURL(const char *URL, char **Proto, char **Host, char **Port, char **User, char **Password, char **Path, char **Args);

//Parse a URL type that lacks a protocol or Args part, something like "myhost.com:22"
const char *ParseHostDetails(const char *Data,char **Host,char **Port,char **User, char **Password);

//Parse URL, but allow two arguments after the url '-password' and '-keyfile'. 
// e.g. ssh:user@host -keyfile ./id_rsa
//These are really intended for ssh urls
void ParseConnectDetails(const char *Str, char **Proto, char **Host, char **Port, char **User, char **Pass, char **InitDir);

//for webpage-style partial urls, that can be relative to a parent url, resolve the url relative to it's parent
//in order to produce a full url
char *ResolveURL(char *RetStr, const char *Parent, const char *SubItem);

#ifdef __cplusplus
}
#endif

#endif
