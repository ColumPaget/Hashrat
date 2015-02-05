#include "includes.h"
#include "Tokenizer.h"
#include "string.h"

#define TOK_SPACE 1

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
	eptr++;
	*start=eptr;
	return(FALSE);
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
		if (MatchType==TOK_SPACE) 
		{
			while (isspace(*eptr)) eptr++;
			eptr--;
			MatchType=0;
		}
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





int GetTokenFindSeparator(const char *Pattern, const char *String, const char **SepStart, const char **SepEnd, int Flags)
{
const char *start_ptr=NULL, *end_ptr=NULL, *ptr;

start_ptr=String;

while (*start_ptr != '\0')
{
if (*start_ptr=='\\')
{
  start_ptr++;
  start_ptr++;
}


ptr=Pattern;
while (ptr)
{
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



char *GetToken(const char *SearchStr, const char *Separator, char **Token, int Flags)
{
const char *SepStart=NULL, *SepEnd=NULL;
const char *sptr, *eptr;


/* this is a safety measure so that there is always something in Token*/
*Token=CopyStr(*Token,"");

if (! SearchStr) return(NULL);
if (*SearchStr=='\0') return(NULL);

GetTokenFindSeparator(Separator, SearchStr,&SepStart,&SepEnd, Flags);

sptr=SearchStr;
eptr=SepStart;
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


//return empty string, but not null
if ((! SepEnd) || (*SepEnd=='\0')) 
{
  SepEnd=SearchStr+StrLen((char *) SearchStr);
}

return((char *) SepEnd);
}




char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value)
{
char *ptr, *ptr2;
char *Token=NULL;

ptr=GetToken(Input,PairDelim,&Token,GETTOKEN_HONOR_QUOTES);
if (StrLen(Token))
{
ptr2=GetToken(Token,NameValueDelim,Name,GETTOKEN_HONOR_QUOTES);
ptr2=GetToken(ptr2,PairDelim,Value,GETTOKEN_HONOR_QUOTES);
StripQuotes(*Name);
StripQuotes(*Value);
}

DestroyString(Token);
return(ptr);
}

