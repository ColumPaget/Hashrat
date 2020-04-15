/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_STRING
#define LIBUSEFUL_STRING

/*****************************************************************************
Functions related to resizeable strings.

From version 4.0 libUseful uses 'StrLenCache-ing'. This dramatically speeds up
programs that deal with long strings, as it's no longer needed to iterate 
through the entire string to calculate its length. Instead the lengths of the
most recently used strings are held in a cache. This means less data passes
through the CPU's L1/L2/L3 cache, which can also further speed things up.

The downside to this is that you can no longer just set a character to the 
null character (ascii zero or '/0') in order to truncate the string, because
the cache will still think the string has it's old length and functions like
CatStr will misbehave, as characters will be added *after* the terminating 
null character, and so lost. This you must use the 'StrTrunc', 'StrTruncChar'
and 'StrRTruncChar' functions to truncate strings
*****************************************************************************/

#include <stdarg.h>
#include <string.h> //for strlen, used below in StrLen
#include "defines.h"
#include "GeneralFunctions.h"

//UsedW with 'MatchTokenFromList'
#define MATCH_TOKEN_PART 1
#define MATCH_TOKEN_CASE 2

#ifdef __cplusplus
extern "C" {
#endif


// this is for compatiblity with older programs that use 'DestroyString', whose name
// was later changed to 'Destroy' as you can use it to free any object. It doesn't
// crash if you pass it a NULL pointer
#define DestroyString(s) (Destroy(s))

//A few very simple and frequently used functions can be reduced
//down to macros using weird stuff like the ternary condition
//operator '?' and the dreaded comma operator ','

//return length of a string. Doesn't crash if string is NULL, just returns 0
#define StrLen(str) ( (str) ? strlen(str) : 0 )


//if a string is not null, and not empty (contains chars) then return true. Doesn't call 'strlen' or iterate through entire string
//so is more efficient then using StrLen for the same purpose
#define StrValid(str) ( (str && (*(const char *) str != '\0')) ? TRUE : FALSE )


//returns true if string is NULL or empty
#define StrEnd(str) ( (str &&  (*(const char *) str != '\0')) ? FALSE : TRUE )

//copy a number of strings into Dest. Like this:
//   Dest=MCopyStr(Dest, Str1, Str2, Str3, NULL);
#define MCopyStr(Dest, ...) InternalMCopyStr(Dest, __VA_ARGS__, NULL)

//list MCopyStr but concatanantes strings onto Dest rather than copying into it
#define MCatStr(Dest, ...) InternalMCatStr(Dest, __VA_ARGS__, NULL)

//return a copy of a string
#define CloneStr(Str) (CopyStr(NULL,Str))

//Concat 'Src' onto 'Dest'
//yes, we need the strlen even though it means traversing the string twice. 
//We need to know how much room 'realloc' needs
#define CatStr(Dest, Src) (CatStrLen(Dest,Src,StrLen(Src)))


//Quote some standard chars in a string with '\'. 
#define EnquoteStr(Dest, Src) (QuoteCharsInStr((Dest), (Src), "'\"\r\n"))

//free memory up. Doesn't crash if Obj is null
void Destroy(void *Obj);


//thse are used internally, you'll not normally use any of these functions
int StrLenFromCache(const char *Str);
void StrLenCacheDel(const char *Str);
void StrLenCacheUpdate(const char *Str, int incr);
void StrLenCacheAdd(const char *Str, size_t len);


//allocate or reallocate 'Len' bytes of memory to a resizeable string
char *SetStrLen(char *Str, size_t Len);

//strcmp that won't creash if str is null
int CompareStr(const char *S1, const char *S2);

//copy Len bytes from Src to Dest, resizing Dest if needed and return Dest
char *CopyStrLen(char *Dest, const char *Src, size_t Len);

//copy Src to Dest, resizing Dest as necessary
char *CopyStr(char *Dest, const char *Src);

//concatanate Len bytes from Src to Dest, resizing Dest if needed and return Dest
char *CatStrLen(char *Dest, const char *Src, size_t Len);

//Pad a str with 'PadLen' copies of character 'Pad'
char *PadStr(char*Dest, char Pad, int PadLen);

//Pad a str TO 'PadLen' with copies of character 'Pad'
char *PadStrTo(char*Dest, char Pad, int PadLen);

//copy a string and pad it with PadLen chars
char *CopyPadStr(char*Dest, const char *Src, char Pad, int PadLen);

//copy a string and pad it TO PadLen  length
char *CopyPadStrTo(char*Dest, const char *Src, char Pad, int PadLen);

char *ReplaceStr(char *RetStr, const char *Input, const char *Old, const char *New);

//these are the quivalent of 'sprintf', except with resizable strings. Furthermore %n is not supported.
char *VFormatStr(char *RetStr, const char *Format, va_list);
char *FormatStr(char *RetStr, const char *Format, ...);

//Add a single char to string. Has to internally strlen the string to find the end, so this can be slow if doing a lot
//of char add operations
char *AddCharToStr(char *Buffer, char Char);

//add char to a string of length 'Len'. Avoids doing a strlen so may be more efficient
char *AddCharToBuffer(char *Buffer, size_t Len, char Char);

//add 'Len' Bytes to a string already containing 'DestLen' characters
char *AddBytesToBuffer(char *Dest, size_t DestLen, char *Bytes, size_t Len);

//uppercase a string
char *strupr(char *Str);

//lowercase a string
char *strlwr(char *Str);

//replace every instance of character 'c1' with character 'c2'
char *strrep(char *Str, char c1, char c2);

//replace any character in the list 'oldchars' with the charcter 'newchar
char *strmrep(char *str, char *oldchars, char newchar);

//strtol, except only considering the first 'len' bytes
int strntol(const char **ptr, int len, int radix, long *value);


//returns 'true' if 'str' is 'true' or 'y' (case insensitive) or any non-zero number
int strtobool(const char *str);

// returns true if string only contains alphabetic characters
int istext(const char *Str);

//returns true if string only contains digits
int isnum(const char *Str);

//Truncate a string to so many chars
char *StrTrunc(char *Str, int Len);

//Truncate a string to a terminator character. 
//Returns 'TRUE' if character found, 'FALSE' otherwise
int StrTruncChar(char *Str, char Term);

//Truncate a string to a terminator character but search for character from end of string ('R' for 'Reverse')
//Returns 'TRUE' if character found, 'FALSE' otherwise
int StrRTruncChar(char *Str, char Term);

//strip trailing whitespace from a string
char *StripTrailingWhitespace(char *Str);

//strip leading whitespace from a string
char *StripLeadingWhitespace(char *Str);

//strip carriage-return and linefeed characters from the end of a string
char *StripCRLF(char *Str);

//if string stars and ends with either ' or " strip those
char *StripQuotes(char *Str);

//for any of the chars listed in 'QuoteChars' quote them using '\' style quotes.
char *QuoteCharsInStr(char *Buffer, const char *String, const char *QuoteChars);

//undo '\' style quoting and 
char *UnQuoteStr(char *Buffer, const char *Line);

//given a list of strings. match 'Token' against them and return the index of the first one to match
// 'Flags' can be:

//  MATCH_TOKEN_CASE    - perform a case sensitive match, default is case insensitive
//	MATCH_TOKEN_PART    - match just the length of the token in the list. So 'str' would match against 'string'
int MatchTokenFromList(const char *Token,const char **List, int Flags);


//Varidic Versions of CopyStr and CatStr. Always call these through the #defines above, as those add NULL
//in case you forgot to do so
char *InternalMCatStr(char *, const char *, ...);
char *InternalMCopyStr(char *, const char *, ...);

#ifdef __cplusplus
}
#endif




#endif

