#include "includes.h"
#include "Tokenizer.h"
#include "string.h"

#define TOK_SPACE 1
#define TOK_CODE  2

#ifdef __MMX__
#include <mmintrin.h>
#endif

int GetTokenSepMatch(const char *Pattern,const char **start, const char **end, int Flags)
{
const char *pptr, *eptr;
char Quote='\0';
int MatchType=0;

//if start and end pointers are same, then we've no string
if (*start==*end) return(FALSE);

pptr=Pattern;
eptr=*start;


while (1)
{
//check the current 'pattern' char
switch (*pptr)
{
	//if we run out of pattern, then we got a match
	case '\0':
	*end=eptr;
	return(TRUE);
	break;

	//in multiseparator mode a '|' is the same as '\0'
	case '|':
	if (Flags & GETTOKEN_MULTI_SEPARATORS)
	{
	*end=eptr;
	return(TRUE);
	}
	break;


	//Quoted char
	case '\\':
  pptr++;
  if (*pptr=='S') MatchType=TOK_SPACE;
  if (*pptr=='X') MatchType=TOK_CODE;
	break;

}

switch (*eptr)
{
	//if we run out of string, then we got a part match, but its not
	//a full match, so we return fail
	case '\0': return(FALSE); break;

	case '\\':
	//if we got a quoted character we can't have found
	//the separator, so return false
	if (Flags & GETTOKEN_BACKSLASH)
	{
	if (*eptr != *pptr) return(FALSE);
	}
	else
	{
	eptr++;
	*start=eptr;
	return(FALSE);
	}
	break;

	case '"':
	case '\'':
	if (Flags & GETTOKEN_HONOR_QUOTES)
	{
		Quote=*eptr;
		eptr++;
		while ((*eptr != Quote) && (*eptr != '\0'))
		{
			//handle quoted chars
			if ((*eptr=='\\') && (*(eptr+1) != '\0'))eptr++;
			eptr++;
		}
		if (*eptr == '\0') eptr--;	//because there's a ++ below
		*start=eptr;
		return(FALSE);
	}
	else if (*eptr != *pptr) return(FALSE);
	break;

	case ' ':
	case '	':
	case '\n':
	case '\r':
		if ((MatchType==TOK_SPACE) || (MatchType==TOK_CODE))
		{
			while (isspace(*eptr)) eptr++;
			eptr--;
			MatchType=0;
		}
		else if (*eptr != *pptr) return(FALSE);
	break;

	case '(':
	case ')':
	case '=':
	case '!':
	case '<':
	case '>':
		if (MatchType==TOK_CODE) MatchType=0; 
		else if (*eptr != *pptr) return(FALSE);
	break;

	default:
		if (MatchType != 0) return(FALSE);
		if (*eptr != *pptr) return(FALSE);
	break;
}

pptr++;
eptr++;
}

return(FALSE);
}





/*
int GetTokenFindSeparator(const char *Pattern, const char *String, const char **SepStart, const char **SepEnd, int Flags)
{
const char *start_ptr=NULL, *end_ptr=NULL, *ptr;
#ifdef __MMX__
__m64 src, cmp1, cmp2;
#endif


start_ptr=String;

while (*start_ptr != '\0')
{

	if ((*start_ptr=='\\') && (! Flags & GETTOKEN_BACKSLASH))
	{
		start_ptr++;
		start_ptr++;
	}

	ptr=Pattern;
	while (ptr)
	{
	#ifdef __MMX__
	if (*ptr != '\\')
	{
	cmp1=_mm_set1_pi8(*ptr);
	src=*(__m64 *) start_ptr;
	cmp2=_mm_cmpeq_pi8(src,cmp1);
	if ((uint64_t) cmp2==0)
	{
	cmp1=_mm_cmpeq_pi8(src,cmp2);
	if ((uint64_t) cmp1 != 0) 
	{
		while (*start_ptr !='\0') start_ptr++;
		*SepStart=start_ptr;
		return(FALSE);
	}
	start_ptr+=8;
	continue;
	}
	}
	#endif

		if (GetTokenSepMatch(ptr,&start_ptr, &end_ptr, Flags))
		{
			*SepStart=start_ptr;
			*SepEnd=end_ptr;
			return(TRUE);
		}

		if (Flags & GETTOKEN_MULTI_SEPARATORS) 
		{
			ptr=strchr(ptr,'|');
			if (ptr) ptr++;
		}
		else ptr=NULL;
	}

if ((*start_ptr) !='\0') start_ptr++;
}

//We found nothing, set sep start to equal end of string
*SepStart=start_ptr;

return(FALSE);
}
*/

int GetTokenFindSeparator(const char *Pattern, const char *String, const char **SepStart, const char **SepEnd, int Flags)
{
const char *start_ptr=NULL, *end_ptr=NULL, *ptr;
#ifdef __MMX__
__m64 src, cmp1, cmp2;
#endif


ptr=Pattern;
while (ptr)
{
	start_ptr=String;
	while (*start_ptr != '\0')
	{
		if ((*start_ptr=='\\') && (! (Flags & GETTOKEN_BACKSLASH)))
		{
			start_ptr++;
			start_ptr++;
			continue;
		}
	
		#ifdef __MMX__
		if (*ptr != '\\')
		{
		cmp1=_mm_set1_pi8(*ptr);
		src=*(__m64 *) start_ptr;
		cmp2=_mm_cmpeq_pi8(src,cmp1);
		if ((uint64_t) cmp2==0)
		{
		cmp1=_mm_cmpeq_pi8(src,cmp2);
		if ((uint64_t) cmp1 != 0) 
		{
			while (*start_ptr !='\0') start_ptr++;
			*SepStart=start_ptr;
			return(FALSE);
		}
		start_ptr+=8;
		continue;
		}

/*
		if (((uint64_t) cmp2 & 0xFFFFFFFF)==0)
		{
		cmp1=_mm_cmpeq_pi8(src,cmp2);
		if (((uint64_t) cmp1 & 0xFFFFFFFF)==0)
		{
			while (*start_ptr !='\0') start_ptr++;
			*SepStart=start_ptr;
			return(FALSE);
		}
		start_ptr+=4;
		continue;
		}
*/

		}
		#endif
	
		if (GetTokenSepMatch(ptr,&start_ptr, &end_ptr, Flags))
		{
			*SepStart=start_ptr;
			*SepEnd=end_ptr;
			return(TRUE);
		}
		if ((*start_ptr) !='\0') start_ptr++;
	}

	if (Flags & GETTOKEN_MULTI_SEPARATORS) 
	{
		ptr=strchr(ptr,'|');
		if (ptr) ptr++;
	}
	else ptr=NULL;
}

//We found nothing, set sep start to equal end of string
*SepStart=start_ptr;

return(FALSE);
}


char *GetToken(const char *SearchStr, const char *Separator, char **Token, int Flags)
{
const char *SepStart=NULL, *SepEnd=NULL;
const char *sptr, *eptr;


/* this is a safety measure so that there is always something in Token*/
if (Token) *Token=CopyStr(*Token,"");

if ((! Token) || StrEnd(SearchStr))
{
	#ifdef __MMX__
		if (! (Flags & GETTOKEN_NOEMMS)) _mm_empty();
	#endif
	return(NULL);
}

GetTokenFindSeparator(Separator, SearchStr, &SepStart, &SepEnd, Flags);

sptr=SearchStr;

if (Flags & GETTOKEN_INCLUDE_SEP)
{
	if (SepStart==SearchStr) eptr=SepEnd;
	else 
	{
		eptr=SepStart;
		SepEnd=SepStart;
	}
}
else if (Flags & GETTOKEN_APPEND_SEP) eptr=SepEnd;
else eptr=SepStart;

if (Flags & GETTOKEN_STRIP_QUOTES)
{
	if ((*sptr=='"') || (*sptr=='\'')) 
	{
		//is character before the sep a quote? If so, we copy one less char, and also start one character later
		//else we copy the characters as well
		eptr--;
		if (*sptr==*eptr) sptr++;
		else eptr++;
	}
}

*Token=CopyStrLen(*Token,sptr,eptr-sptr);

if (Flags & GETTOKEN_STRIP_SPACE)
{
StripTrailingWhitespace(*Token);
StripLeadingWhitespace(*Token);
}

//return empty string, but not null
if ((! SepEnd) || (*SepEnd=='\0')) 
{
  SepEnd=SearchStr+StrLen((char *) SearchStr);
}

#ifdef __MMX__
if (! (Flags & GETTOKEN_NOEMMS)) _mm_empty();
#endif
return((char *) SepEnd);
}




char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value)
{
char *ptr, *ptr2;
char *Token=NULL;

*Name=CopyStr(*Name,"");
*Value=CopyStr(*Value,"");
ptr=GetToken(Input,PairDelim,&Token,GETTOKEN_HONOR_QUOTES);
if (StrValid(Token))
{
ptr2=GetToken(Token,NameValueDelim,Name,GETTOKEN_HONOR_QUOTES);
ptr2=GetToken(ptr2,PairDelim,Value,GETTOKEN_HONOR_QUOTES);
StripQuotes(*Name);
StripQuotes(*Value);
}

DestroyString(Token);
return(ptr);
}

