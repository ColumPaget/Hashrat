#ifndef LIBUSEFUL_UNICODE_H
#define LIBUSEFUL_UNICODE_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

// set the GLOBAL unicode level.
// this is a global value that represents the unicode abilities of an OS or device. Functions like 'UnicodeStr' and 'StrAddUnicodeChar' use this
// information in all their operations.  

// There are 3 values
// level 0: no unicode support, unicode chars will be replaced with '?'
// level 1: unicode support up to 0x800, chars above this will be replaced with '?'
// level 2: unicode support up to 0x10000, chars above this will be replaced with '?'
// level 3: unicode support up to 0x1FFFF, chars above this will be replaced with '?'
void UnicodeSetUTF8(int level);

//decode a single UTF-8 sequence pointed to by ptr and return it as an unsigned int, incrementing ptr to 
//point beyond the just decoded sequence
unsigned int UnicodeDecode(const char **ptr);

//encode a single unicode value ('Code') to a UTF-8 string using the supplied UnicodeLevel 
//rather than the global unicode level set by UnicodeSetUTF9
char *UnicodeEncodeChar(char *RetStr, int UnicodeLevel, int Code);

//encode a single unicode value ('Code') to a unicode string honoring the global unicode level
char *UnicodeStr(char *RetStr, int Code);

//encode a single unicode value ('Code') to a unicode string honoring the global unicode level, and append that string to a character string
char *StrAddUnicodeChar(char *RetStr, int uchar);

//lookup a unicode string by name at the specified Unicode support level
char *UnicodeStrFromNameAtLevel(char *RetStr, int UnicodeLevel, const char *Name);

//lookup a unicode string by name, honoring the global unicode level
char *UnicodeStrFromName(char *RetStr, const char *Name);


#ifdef __cplusplus
}
#endif

#endif

