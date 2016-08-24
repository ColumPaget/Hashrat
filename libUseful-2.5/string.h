#ifndef LIBUSEFUL_STRING
#define LIBUSEFUL_STRING

#include <stdarg.h>
#include <string.h> //for strlen, used below in StrLen
#include "defines.h"

#define MATCH_TOKEN_PART 1
#define MATCH_TOKEN_CASE 2

#ifdef __cplusplus
extern "C" {
#endif

//A few very simple and frequently used functions can be reduced
//down to macros using weird stuff like the ternary condition 
//operator '?' and the dreaded comma operator ','
#define StrLen(str) ( str ? strlen(str) : 0 )
#define StrValid(str) ( (str && (*(const char *) str != '\0')) ? TRUE : FALSE )
//#define StrEnd(str) ( ((! str) || (*str == '\0') || (str > __builtin_frame_address (0)) ? TRUE : FALSE )
#define StrEnd(str) ( (str &&  (*(const char *) str != '\0')) ? FALSE : TRUE )

//size_t StrLen(const char *Str);
void DestroyString(void *);
int CompareStr(const char *S1, const char *S2);
char *CopyStrLen(char *,const char *,size_t);
char *CopyStr(char *, const char *);
char *MCatStr(char *, const char *, ...);
char *MCopyStr(char *, const char *, ...);
char *CatStr(char *, const char *);
char *CatStrLen(char *,const char *,size_t);
char *PadStr(char*Dest, char Pad, int PadLen);
char *CopyPadStr(char*Dest, char *Src, char Pad, int PadLen);
char *VFormatStr(char *,const char *,va_list);
char *FormatStr(char *,const char *,...);
char *AddCharToStr(char *,char);
char *AddCharToBuffer(char *Buffer, size_t BuffLen, char Char);
char *AddBytesToBuffer(char *Buffer, size_t BuffLen, char *Bytes, size_t Len);
char *SetStrLen(char *,size_t);
char *strupr(char *);
char *strlwr(char *);
char *strrep(char *,char, char);
char *strmrep(char *str, char *oldchars, char newchar);
char *CloneString(const char *);
void StripTrailingWhitespace(char *);
void StripLeadingWhitespace(char *);
void StripCRLF(char *);
void StripQuotes(char *);
char *QuoteCharsInStr(char *Buffer, const char *String,const  char *QuoteChars);
char *UnQuoteStr(char *Buffer, const char *Line);
char *EnquoteStr(char *Out, const char *In);
int MatchTokenFromList(const char *Token,const char **List, int Flags);
int MatchLineStartFromList(const char *Token,char **List);

#ifdef __cplusplus
}
#endif




#endif

