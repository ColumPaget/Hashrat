#include "includes.h"

#ifndef va_copy
#define va_copy(dest, src) (dest) = (src) 
#endif 

/*
int StrLen(char *Str)
{
if (! Str) return(0);
return(strlen(Str));
}
*/


char *DestroyString(char *string)
{
if (string) free(string);
// we return a null to be put into the string ptr 
return(0);
}



int CompareStr(const char *S1, const char *S2)
{
int len1, len2;
len1=StrLen(S1);
len2=StrLen(S2);
if ((len1==0) && (len2==0)) return(0);

if (len1==0) return(-1);
if (len2==0) return(1);

return(strcmp(S1,S2));
}



char *CopyStrLen(char *Dest, const char *Src,int len)
{
char *ptr;
ptr=CopyStr(Dest,Src);
if (StrLen(ptr) >len) ptr[len]=0;
return(ptr);
}

char *CatStrLen(char *Dest, const char *Src,int len)
{
char *ptr;
int catlen=0;

catlen=StrLen(Dest);
ptr=CatStr(Dest,Src);
catlen+=len;
if (StrLen(ptr) > catlen) ptr[catlen]=0;
return(ptr);
}


char *CopyStr(char *Dest, const char *Src)
{
int len;
char *ptr;

if (Dest) *Dest=0;
return(CatStr(Dest,Src));
}



char *VCatStr(char *Dest, const char *Str1,  va_list args)
{
//initialize these to keep valgrind happy
int len=0;
char *ptr=NULL;
const char *sptr=NULL;

if (Dest !=NULL) 
{
len=StrLen(Dest);
ptr=Dest;
}
else
{
 len=10;
 ptr=(char *) calloc(10,1);
}

if (! Str1) return(ptr);
for (sptr=Str1; sptr !=NULL; sptr=va_arg(args,const char *))
{
len+=StrLen(sptr)+1;
len=len*2;


ptr=(char *) realloc(ptr,len);
if (ptr && sptr) strcat(ptr,sptr);
}

return(ptr);
}

char *MCatStr(char *Dest, const char *Str1,  ...)
{
char *ptr=NULL;
va_list args;

va_start(args,Str1);
ptr=VCatStr(Dest,Str1,args);
va_end(args);

return(ptr);
}


char *MCopyStr(char *Dest, const char *Str1,  ...)
{
char *ptr=NULL;
va_list args;

ptr=Dest;
if (ptr) *ptr='\0';
va_start(args,Str1);
ptr=VCatStr(ptr,Str1,args);
va_end(args);

return(ptr);
}

char *CatStr(char *Dest, const char *Src)
{
int len;
char *ptr;

if (Dest !=NULL) 
{
len=StrLen(Dest);
ptr=Dest;
}
else
{
 len=10;
 ptr=(char *) calloc(10,1);
}

if (StrLen(Src)==0) return(ptr);
len+=StrLen(Src);
len++;

ptr=(char *)realloc(ptr,len);
if (ptr && Src) strcat(ptr,Src);
return(ptr);
}



//Clone string is really intended to be passed into the 'ListClone' function
//in the manner of a copy constructor, for most uses it's best to stick to
//CopyStr

char *CloneStr(const char *Str)
{
return(CopyStr(NULL,Str));
}


char *VFormatStr(char *InBuff, const char *InputFmtStr, va_list args)
{
int inc=100, count=1, result=0;
char *Tempstr=NULL, *FmtStr=NULL;
va_list argscopy;

Tempstr=InBuff;

//Take a copy of the supplied Format string and change it.
//Do not allow '%n', it's useable for exploits
FmtStr=CopyStr(FmtStr,InputFmtStr);
EraseString(FmtStr, "%n");


inc=4 * StrLen(FmtStr); //this should be a good average
for (count=1; count < 100; count++)
{
	result=inc * count +1;
  Tempstr=SetStrLen(Tempstr, result);

		//the vsnprintf function DESTROYS the arg list that is passed to it.
		//This is just plain WRONG, it's a long-standing bug. The solution is to
		//us va_copy to make a new one every time and pass that in.
		va_copy(argscopy,args);
     result=vsnprintf(Tempstr,result,FmtStr,argscopy);
		va_end(argscopy);

  /* old style returns -1 to say couldn't fit data into buffer.. so we */
  /* have to guess again */
  if (result==-1) continue;

  /* new style returns how long buffer should have been.. so we resize it */
  if (result > (inc * count))
  {
    Tempstr=SetStrLen(Tempstr, result+10);
    result=vsnprintf(Tempstr,result+10,FmtStr,args);
  }

   break;
}

DestroyString(FmtStr);


return(Tempstr);
}



char *FormatStr(char *InBuff, const char *FmtStr, ...)
{
int inc=100, count=1, result;
char *tempstr=NULL;
va_list args;

va_start(args,FmtStr);
tempstr=VFormatStr(InBuff,FmtStr,args);
va_end(args);
return(tempstr);
}


char *AddCharToStr(char *Dest,char Src)
{
char temp[2];
char *ptr=NULL;

temp[0]=Src;
temp[1]=0;
ptr=CatStr(Dest,temp);
return(ptr);
}


inline char *AddCharToBuffer(char *Dest, int DestLen, char Char)
{
char *actb_ptr;

//if (Dest==NULL || ((DestLen % 100)==0)) 
actb_ptr=(char *) realloc((void *) Dest,DestLen +110);
//else actb_ptr=Dest;

actb_ptr[DestLen]=Char;
actb_ptr[DestLen+1]='\0';

return(actb_ptr);
}


inline char *AddBytesToBuffer(char *Dest, int DestLen, char *Bytes, int NoOfBytes)
{
char *actb_ptr=NULL;

//if (Dest==NULL || ((DestLen % 100)==0)) 
actb_ptr=(char *) realloc((void *) Dest,DestLen + NoOfBytes +10);
//else actb_ptr=Dest;

memcpy(actb_ptr+DestLen,Bytes,NoOfBytes);

return(actb_ptr);
}



char *SetStrLen(char *Str,int len)
{
/* Note len+1 to allow for terminating NULL */
if (Str==NULL) return((char *) calloc(1,len+1));
else return( (char *) realloc(Str,len+1));
}


char *strlwr(char *str)
{
int count, len;
len=StrLen(str);
for (count=0; count < len; count++) str[count]=tolower(str[count]);
return(str);
}

char *strupr(char *str)
{
int count, len;
len=StrLen(str);
for (count=0; count < len; count++) str[count]=toupper(str[count]);
return(str);
}

char *strrep(char *str, char oldchar, char newchar)
{
int count, len;
len=StrLen(str);
for (count=0; count < len; count++) 
{
if (str[count]==oldchar) str[count]=newchar;
}
return(str);
}

char *strmrep(char *str, char *oldchars, char newchar)
{
int count, len;
len=StrLen(str);
for (count=0; count < len; count++) 
{
if (strchr(oldchars,str[count])) str[count]=newchar;
}
return(str);
}

void StripTrailingWhitespace(char *String)
{
int count,len;

len=StrLen(String);
if (len==0) return;
for(count = len-1; (count >-1) && isspace(String[count]); count--) String[count]=0;
}


void StripLeadingWhitespace(char *String)
{
int count,len;

len=StrLen(String);
if (len==0) return;
for(count = 0; (count <len -1) && isspace(String[count]); count++);
memmove(String,String+count,len-count);
String[len-count]=0;
}



void StripCRLF(char *Line)
{
int len, count;

len=StrLen(Line);
if (len < 1) return;

for (count=len-1; (count > -1); count--)
{
   if ((Line[count]=='\r') || (Line[count]=='\n')) Line[count]=0;
   else break;
}

}


void StripQuotes(char *Str)
{
int len;
char *ptr, StartQuote='\0';

ptr=Str;
while (isspace(*ptr)) ptr++;
len=StrLen(ptr);

if ((*ptr=='"') || (*ptr=='\'')) StartQuote=*ptr;

if ((len > 0) && (StartQuote != '\0'))
{
if (ptr[len-1]==StartQuote) ptr[len-1]='\0';
memmove(Str,ptr+1,len);
}

}


char *EnquoteStr(char *Out, const char *In)
{
const char *ptr;
char *ReturnStr=NULL;
int len=0;

ptr=In;
ReturnStr=CopyStr(Out,"");

while (ptr && (*ptr != '\0'))
{
        switch (*ptr)
        {
		case '\\':
                case '"':
                case '\'':
                  ReturnStr=AddCharToBuffer(ReturnStr,len,'\\');
		  len++;
                  ReturnStr=AddCharToBuffer(ReturnStr,len, *ptr);
		  len++;
                  break;


                case '\r':
                  ReturnStr=AddCharToBuffer(ReturnStr,len,'\\');
		  len++;
                  ReturnStr=AddCharToBuffer(ReturnStr,len, 'r');
		  len++;
                break;


                case '\n':
                  ReturnStr=AddCharToBuffer(ReturnStr,len,'\\');
		  len++;
                  ReturnStr=AddCharToBuffer(ReturnStr,len, 'n');
		  len++;
                break;

                default:
                  ReturnStr=AddCharToBuffer(ReturnStr,len, *ptr);
		  len++;
                break;
        }
        ptr++;

}
return(ReturnStr);
}



int MatchTokenFromList(const char *Token,char **List, int Flags)
{
int count, len;

for (count=0; List[count] !=NULL; count++)
{
  len=StrLen(List[count]);
  if (len==0) continue;
  if (StrLen(Token) < len) continue;

  if (Flags & MATCH_TOKEN_PART)
  {
    if (Flags & MATCH_TOKEN_CASE)
    {
	    if (strncmp(Token,List[count],len)==0) return(count);
    }
    else if (strncasecmp(Token,List[count],len)==0) return(count);
  }
  else 
  {
  	if (StrLen(Token) != len) continue;
	if (Flags & MATCH_TOKEN_CASE)
	{
		if(strcmp(Token,List[count])==0)  return(count);
	}
  	else if (strcasecmp(Token,List[count])==0) return(count);
  }
}
return(-1);
}



int MatchLineStartFromList(const char *Token,char **List)
{
int count, len;

for (count=0; List[count] !=NULL; count++)
{
  len=StrLen(List[count]);

  if (
       (len >0) && 
       (strncasecmp(Token,List[count],len)==0) 
     ) return(count);
}
return(-1);
}

#define SEP_CHAR_SPACE -1

//Handle Quotes. If we start with a quote, sweep through to find the end of the quotes
char *HandleQuotes(char *String)
{
char *ptr;
char StartQuote='\0';

ptr=String;
if ((*ptr)=='\"') StartQuote=*ptr;
if ((*ptr)=='\'') StartQuote=*ptr;
if (StartQuote)
{
  ptr++;
  while ((*ptr) != StartQuote) 
  {
	  if ((*ptr)=='\0') return(ptr);
	  if ((*ptr)=='\\') ptr++;
	  ptr++;
  }
  ptr++;
}   
return(ptr);
}


//This function searches for the separator
int GetTokenSepMatch(char *Pattern, char **start, char **end, int Flags)
{
char *pptr, *eptr;
int MatchChar, InQuotes=FALSE;


pptr=Pattern;


if (Flags & GETTOKEN_QUOTES) *start=HandleQuotes(*start);
if (*start==*end) return(FALSE);

eptr=*start;
while (1)
{
//if we run out of pattern, then we got a match
if (*pptr=='\0') 
{
	*end=eptr;
	return(TRUE);
}

//if ((*eptr=='"') && (Flags & GETTOKEN_QUOTES)) *eptr=HandleQuotes(*eptr);

//if we run out of string, then we got a part match, but its not
//a match, so we return fail
if ((*eptr)=='\0')
{
	return(FALSE);
}

if (*eptr=='\\' )
{
	//if we got a quoted character we can't have found
	//the separator, so return false
	eptr++;
	return(FALSE);
}


if (*pptr=='\\')
{
  pptr++;
  if (*pptr=='S') MatchChar=SEP_CHAR_SPACE;
  else MatchChar=*pptr;
}
else MatchChar=*pptr;


if (MatchChar==SEP_CHAR_SPACE)  
{
   if (! isspace(*eptr)) return(FALSE);
   while (isspace(*(eptr+1))) eptr++;
}
else if (*eptr != *pptr) return(FALSE);

pptr++;
eptr++;
}
return(FALSE);
}



char *GetNextSeparator(char *Pattern, char **Sep, int Flags)
{
char *ptr;
int len=0;

ptr=Pattern;
if (*ptr=='\0') return(NULL);
*Sep=CopyStr(*Sep,"");

ptr=Pattern;
while ((*ptr != '\0') && (*ptr !='|'))
{
	*Sep=AddCharToBuffer(*Sep,len,*ptr);
	len++;
	ptr++;
}

if (*ptr == '|') ptr++;

return(ptr);
}


int GetTokenFindSeparator(char *Pattern, char *String, const char **SepStart, const char **SepEnd, int Flags)
{
char *start_ptr=NULL, *end_ptr=NULL;
char *Sep=NULL, *SepPtr, *ptr;

start_ptr=String;

while (*start_ptr != '\0')
{
if (*start_ptr=='\\')
{
  start_ptr++;
  start_ptr++;
}

if (Flags & GETTOKEN_MULTI_SEPARATORS)
{
ptr=GetNextSeparator(Pattern,&Sep,Flags);
SepPtr=Sep;
}
else 
{
	SepPtr=Pattern;
	ptr=Pattern+StrLen(Pattern);
}

while (ptr)
{
if (GetTokenSepMatch(SepPtr,&start_ptr, &end_ptr, Flags))
{
	*SepStart=start_ptr;
	*SepEnd=end_ptr;
	DestroyString(Sep);
	return(TRUE);
}

if (Flags & GETTOKEN_MULTI_SEPARATORS)
{
ptr=GetNextSeparator(ptr,&Sep,Flags);
SepPtr=Sep;
}
else ptr=NULL;

}

if ((*start_ptr) !='\0') start_ptr++;
}

DestroyString(Sep);
return(FALSE);
}


char *GetToken(const char *SearchStr, const char *Separator, char **Token, int Flags)
{
char *Tempstr=NULL;
const char *SepStart=NULL, *SepEnd=NULL;
const char *ptr, *ssptr;
int len=0;


/* this is a safety measure so that there is always something in Token*/
*Token=CopyStr(*Token,"");

len=StrLen(SearchStr);
if (len < 1) return(NULL);

if (! GetTokenFindSeparator((char *) Separator,(char *) SearchStr,&SepStart,&SepEnd, Flags))
{
   SepStart=SearchStr+len;
}


//strip any leading quotes
ptr=SearchStr;
*Token=CopyStrLen(*Token,ptr,SepStart-ptr);

if (Flags & GETTOKEN_QUOTES)
{
	StripQuotes(*Token);
}


//return empty string, but not null
if (StrLen(SepEnd) < 1) 
{
  SepEnd=SearchStr+StrLen((char *) SearchStr);
}

return((char *) SepEnd);
}


#define ESC 0x1B

char *DeQuoteStr(char *Buffer, const char *Line)
{
char *out, *in;
int olen=0;

if (Line==NULL) return(NULL);
out=CopyStr(Buffer,"");
in=(char *) Line;

while(in && (*in != '\0') )
{
	if (*in=='\\')
	{
		in++;
		switch (*in)
		{
		  case 'e': 
			out=AddCharToBuffer(out,olen,ESC);
			olen++;
			break;


		   case 'n': 
			out=AddCharToBuffer(out,olen,'\n');
			olen++;
			break;

		  case 'r': 
			out=AddCharToBuffer(out,olen,'\r');
			olen++;
			break;

		   case 't': 
			out=AddCharToBuffer(out,olen,'\t');
			olen++;
			break;


		   case '\\': 
		   default:
			out=AddCharToBuffer(out,olen,*in);
			olen++;
			break;

		}
	}
	else 
	{
		out=AddCharToBuffer(out,olen,*in);
		olen++;
	}
	in++;
}

return(out);
}



char *QuoteCharsInStr(char *Buffer, const char *String, const char *QuoteChars)
{
char *Tempstr=NULL;
int si, ci, slen, clen, olen=0;

slen=StrLen(String);
clen=StrLen(QuoteChars);

Tempstr=CopyStr(Buffer,"");

for (si=0; si < slen; si++)
{
	for (ci=0; ci < clen; ci++)
	{
  		if (String[si]==QuoteChars[ci])
		{
			Tempstr=AddCharToBuffer(Tempstr,olen, '\\');
			olen++;
			break;
		}
	}
	Tempstr=AddCharToBuffer(Tempstr,olen,String[si]);
	olen++;
}

return(Tempstr);
}


