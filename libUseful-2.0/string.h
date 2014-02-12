#ifndef LIBUSEFUL_STRING
#define LIBUSEFUL_STRING

#include <stdarg.h>
#include <string.h> //for strlen, used below in StrLen

#define GETTOKEN_QUOTES 1
#define GETTOKEN_MULTI_SEPARATORS 2

#define MATCH_TOKEN_PART 1
#define MATCH_TOKEN_CASE 2

#ifdef __cplusplus
extern "C" {
#endif

//A few very simple and frequently used functions can be reduced
//down to macros using weird stuff like the ternary condition 
//operator '?' and the dreaded comma operator ','
#define StrLen(str) ( str ? strlen(str) : 0 )

//int StrLen(char *Str);
char *DestroyString(char *);
int CompareStr(const char *S1, const char *S2);
char *CopyStrLen(char *,const char *,int);
char *CopyStr(char *, const char *);
char *MCatStr(char *, const char *, ...);
char *MCopyStr(char *, const char *, ...);
char *CatStr(char *, const char *);
char *CatStrLen(char *,const char *,int);
char *VFormatStr(char *,const char *,va_list);
char *FormatStr(char *,const char *,...);
char *AddCharToStr(char *,char);
char *AddCharToBuffer(char *Buffer, int BuffLen, char Char);
char *AddBytesToBuffer(char *Buffer, int BuffLen, char *Bytes, int Len);
char *SetStrLen(char *,int);
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
char *DeQuoteStr(char *Buffer, const char *Line);
char *EnquoteStr(char *Out, const char *In);
int MatchTokenFromList(const char *Token,char **List, int Flags);
int MatchLineStartFromList(const char *Token,char **List);
char *GetToken(const char *SearchStr, const char *Delim, char **Token, int Flags);

#ifdef __cplusplus
}
#endif




#endif

