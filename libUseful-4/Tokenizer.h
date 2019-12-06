/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_TOKEN_H
#define LIBUSEFUL_TOKEN_H

#include <stdarg.h>
#include <string.h>

/*
These functions break a string up into tokens. GetToken works like:

ptr=GetToken(Str, "::",&Token,0);
while (ptr)
{
printf("%s\n",Token);
ptr=GetToken(ptr, "::",&Token,0);
}

This imagines a string broken up with "::" separators, like "this::that::theother", illustrating that tokens can be more than one character long. You can also use the 'GETTOKEN_MULTI_SEPARATORS' flag to pass multiple separators to GetToken, like this:


ptr=GetToken(Str, ",|;|\n",&Token,GETTOKEN_MULTI_SEPARATORS);
while (ptr)
{
printf("%s\n",Token);
ptr=GetToken(ptr, ",|;|\n",&Token,GETTOKEN_MULTI_SEPARATORS);
}

This imagines that Str is broken up by three types of separator, commas, semicolons and newlines. The '|' pipe symbol is a divider used to indicate different spearators, as again separators can be more than one character long.

GetToken also accepts some special separator types. "\\S" means 'any whitespace' and "\\X" means 'code separators', which is any whitespace plus "(", ")", "=", "!", "<" and ">", which is intended for tokenizing simple conditional expressions.

For example:

ptr=GetToken(Str, "\\S",&Token,0);
while (ptr)
{
printf("%s\n",Token);
ptr=GetToken(ptr, "\\S",&Token,0);
}


Will break a string up by whitespace. (it has to be "\\S" unfortunately because C will treat a single '\' as a quote, and so \\S becomes \S on compilation.


GetToken also understands quotes in the target string. This is activated by passing the "GETTOKEN_QUOTES", like this:

ptr=GetToken(Str, "\\S",&Token,GETTOKEN_QUOTES);
while (ptr)
{
printf("%s\n",Token);
ptr=GetToken(ptr, "\\S",&Token,GETTOKEN_QUOTES);
}


This will break a string up by whitespace, but any substrings that contain whitespace within quotes will not be broken up. So a string like:

one two three four "five six seven" eight nine

will break up into

one
two
three
four
five six seven
eight
nine

Notice that 'GETTOKEN_QUOTES' also strips quotes from tokens. If you don't want the quotes stripped off, use GETTOKEN_HONOR_QUOTES instead.

The GETTOKEN_INCLUDE_SEPARATORS flag causes separators to be passed as tokens, so


ptr=GetToken(Str, ";", &Token, GETTOKEN_INCLUDE_SEPARATORS);

would break the string "this ; that" up into:

this
;
that

Alternatively the GETTOKEN_APPEND_SEPARATORS Flag adds a separator to the end of a token, so now we'd get

this;
that

GETTOKEN_STRIP_SPACE will cause whitespace to be stripped from the start and end of tokens, even if the separator character is not whitespace.


see examples/Tokenizer.c for examples
*/







//Flags for GetToken

#define GETTOKEN_MULTI_SEPARATORS 1 //multiple seperators divided by '|'
#define GETTOKEN_MULTI_SEP 1
#define GETTOKEN_HONOR_QUOTES 2  //honor quotes but don't strip them
#define GETTOKEN_STRIP_QUOTES 4  //strip quotes (but otherwise ignore)
#define GETTOKEN_QUOTES 6  //honor and strip quotes
#define GETTOKEN_INCLUDE_SEPARATORS 8  //include separators as tokens
#define GETTOKEN_INCLUDE_SEP 8
#define GETTOKEN_APPEND_SEPARATORS 16 //append separators to previous token
#define GETTOKEN_APPEND_SEP 16
#define GETTOKEN_BACKSLASH  32  //treat backslashes as normal characters, rather than a form of quoting
#define GETTOKEN_STRIP_SPACE 64 //strip whitespace from start and end of token



#ifdef __cplusplus
extern "C" {
#endif

//this is a function that converts a config string into flags for GETTOKEN. You wouldn't normally use this unless you have libUseful bound
//to a language that struggles with passing bitmasks of flags (libUseful-lua uses this function)
int GetTokenParseConfig(const char *Config);

//this is the main 'GetToken' function. Given a Search string, and a delimiter, copy the next Token into 'Token' and return a pointer to the remaining string
const char *GetToken(const char *SearchStr, const char *Delim, char **Token, int Flags);

//for strings of the form "name1=value1 name2=value2" get the next name and value using the supplied delimiters
const char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value);


//for strings of the form "name1=value1 name2=value2" get the value for the name/value pair named by 'SearchName'
char *GetNameValue(char *RetStr, const char *Input, const char *PairDelim, const char *NameValueDelim, const char *SearchName);


#ifdef __cplusplus
}
#endif




#endif

