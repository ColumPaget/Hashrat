/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

//These functions are 'utility' functions related to HTTP, but which
//might be used outside of http sometimes, such as quoting and unquoting
//strings with the HTTP quoting system

#ifndef LIBUSEFUL_HTTP_UTIL_H
#define LIBUSEFUL_HTTP_UTIL_H

#include "includes.h"


#ifdef __cplusplus
extern "C" {
#endif


//apply http quoting (e.g. space is '%20') to a string, quoting any character in 'CharList'
// e.g.  Quoted=HTTPQuoteChars(Quoted, "http://user:pass@somewhere.com",":@");
char *HTTPQuoteChars(char *RetBuff, const char *Str, const char *CharList);

//apply http quoting to a string, quoting standard problematic characters
// e.g.  Quoted=HTTPQuote(Quoted, "http://user:pass@somewhere.com");
char *HTTPQuote(char *RetBuff, const char *Str);

//unquote a string that contains HTTP style quoting
// e.g.  Str=HTTPUnQuote(Str, Quoted);
char *HTTPUnQuote(char *RetBuff, const char *Str);

//decode a base64 string in the format <username>:<password> as used in 'Basic' http authentication
//into individual user and password strings
void HTTPDecodeBasicAuth(const char *Auth, char **UserName, char **Password);



#ifdef __cplusplus
}
#endif


#endif
