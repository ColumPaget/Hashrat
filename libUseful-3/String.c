#include "includes.h"
#include <ctype.h>
#include "Unicode.h"

#ifndef va_copy
#define va_copy(dest, src) (dest) = (src)
#endif


int strtobool(const char *str)
{
    if (! StrValid(str)) return(FALSE);
    switch (*str)
    {
    case 'n':
    case 'N':
        return(FALSE);
        break;

    case 'y':
    case 'Y':
        return(TRUE);
        break;

    default:
        if (strcasecmp(str,"true")==0) return(TRUE);
        return(atoi(str));
        break;
    }

    return(FALSE);
}



int CompareStr(const char *S1, const char *S2)
{
    if (
        ((!S1) || (*S1=='\0')) &&
        ((!S2) || (*S2=='\0'))
    ) return(0);

    if ((!S1) || (*S1=='\0')) return(-1);
    if ((!S2) || (*S2=='\0')) return(1);

    return(strcmp(S1,S2));
}


char *CopyStrLen(char *Dest, const char *Src, size_t len)
{
    const char *src, *end;
    char *dst;

    Dest=(char *)realloc(Dest,len+1);
    dst=Dest;
    src=Src;
    if (src)
    {
        end=src+len;
        while ((src < end) && (*src != '\0'))
        {
            *dst=*src;
            dst++;
            src++;
        }
    }
    *dst='\0';

    return(Dest);
}


char *CatStrLen(char *Dest, const char *Src, size_t len)
{
    const char *src, *end;
    char *dst;
    int dstlen;

    dstlen=StrLen(Dest);
    Dest=(char *)realloc(Dest,dstlen+len+1);
    dst=Dest+dstlen;
    src=Src;
    end=src+len;
    while ((src < end) && (*src != '\0'))
    {
        *dst=*src;
        dst++;
        src++;
    }
    *dst='\0';

    return(Dest);
}




char *CopyStr(char *Dest, const char *Src)
{
    if (Dest) *Dest=0;
    return(CatStr(Dest,Src));
}



char *VCatStr(char *Dest, const char *Str1,  va_list args)
{
//initialize these to keep valgrind happy
    size_t len=0, ilen;
    char *ptr=NULL;
    const char *sptr=NULL;
    char *eptr;


    if (Dest !=NULL)
    {
        len=StrLen(Dest);
        ptr=Dest;
    }
    else ptr=(char *) calloc(10,1);

    if (! Str1) return(ptr);

    for (sptr=Str1; sptr !=NULL; sptr=va_arg(args,const char *))
    {
        ilen=StrLen(sptr);
        if (ilen > 0)
        {
            ptr=(char *) realloc(ptr,len+ilen+10);
            if (ptr)
            {
                memcpy(ptr+len, sptr, ilen);
                len+=ilen;
            }
        }
    }
    ptr[len]='\0';

    return(ptr);
}

char *InternalMCatStr(char *Dest, const char *Str1,  ...)
{
    char *ptr=NULL;
    va_list args;

    va_start(args,Str1);
    ptr=VCatStr(Dest,Str1,args);
    va_end(args);

    return(ptr);
}


char *InternalMCopyStr(char *Dest, const char *Str1,  ...)
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



char *PadStr(char*Dest, char Pad, int PadLen)
{
    char *NewStr=NULL;
    int i, len, abslen;

    if (PadLen==0) return(Dest);
    if (PadLen < 0) abslen=0-PadLen;
    else abslen=PadLen;

    for (i=0; i < abslen; i++) NewStr=AddCharToBuffer(NewStr,i,Pad);

    if (PadLen > 0)
    {
        Dest=(CatStr(Dest, NewStr));
        DestroyString(NewStr);
    }
//Negative values mean pad in front of text
    else
    {
        NewStr=CatStr(NewStr,Dest);
        //NewStr is the replacement for Dest, so we destroy Dest
        DestroyString(Dest);
        Dest=NewStr;
    }

    return(Dest);
}


char *PadStrTo(char*Dest, char Pad, int PadLen)
{
    char *NewStr=NULL;
    int val, len;

    if (PadLen==0) return(Dest);

    len=StrLen(Dest);
    if (PadLen > 0)
    {
        val=PadLen-len;
        if (val > 0) return(PadStr(Dest,Pad,val));
    }
    else //must be negative, meaning PrePad rather than PostPad
    {
        val = (0 - PadLen) - len;
        if (val > 0) return(PadStr(Dest,Pad,0-val));
    }

    return(Dest);
}

char *CopyPadStr(char*Dest, const char *Src, char Pad, int PadLen)
{
    Dest=CopyStr(Dest,Src);
    Dest=PadStr(Dest,Pad,PadLen);
    return(Dest);
}

char *CopyPadStrTo(char*Dest, const char *Src, char Pad, int PadLen)
{
    Dest=CopyStr(Dest,Src);
    Dest=PadStrTo(Dest,Pad,PadLen);
    return(Dest);
}


char *VFormatStr(char *InBuff, const char *InputFmtStr, va_list args)
{
    int inc=100, count=1, result=0, FmtLen;
    char *Tempstr=NULL, *FmtStr=NULL, *ptr;
    va_list argscopy;

    Tempstr=InBuff;

//Take a copy of the supplied Format string and change it.
//Do not allow '%n', it's useable for exploits

    FmtLen=StrLen(InputFmtStr);
    FmtStr=CopyStr(FmtStr,InputFmtStr);

//Deny %n. Replace it with %x which prints out the value of any supplied argument
    ptr=strstr(FmtStr,"%n");
    while (ptr)
    {
        memcpy(ptr,"%x",2);
        ptr++;
        ptr=strstr(ptr,"%n");
    }


    inc=4 * FmtLen; //this should be a good average
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
            va_copy(argscopy,args);
            result=vsnprintf(Tempstr,result+10,FmtStr,argscopy);
            va_end(argscopy);
        }

        break;
    }

    DestroyString(FmtStr);

    return(Tempstr);
}



char *FormatStr(char *InBuff, const char *FmtStr, ...)
{
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

    temp[0]=Src;
    temp[1]=0;
    return(CatStrLen(Dest,temp,1));
}


inline char *AddCharToBuffer(char *Dest, size_t DestLen, char Char)
{
    char *actb_ptr;

//if (Dest==NULL || ((DestLen % 100)==0))
    actb_ptr=(char *) realloc((void *) Dest,DestLen +110);
//else actb_ptr=Dest;

    actb_ptr[DestLen]=Char;
    actb_ptr[DestLen+1]='\0';

    return(actb_ptr);
}


inline char *AddBytesToBuffer(char *Dest, size_t DestLen, char *Bytes, size_t NoOfBytes)
{
    char *actb_ptr=NULL;

//if (Dest==NULL || ((DestLen % 100)==0))
    actb_ptr=(char *) realloc((void *) Dest,DestLen + NoOfBytes +10);
//else actb_ptr=Dest;

    memcpy(actb_ptr+DestLen,Bytes,NoOfBytes);

    return(actb_ptr);
}



char *SetStrLen(char *Str,size_t len)
{
    /* Note len+1 to allow for terminating NULL */
    if (Str==NULL) return((char *) calloc(1,len+1));
    else return( (char *) realloc(Str,len+1));
}


char *strlwr(char *str)
{
    char *ptr;

    if (! str) return(NULL);
    for (ptr=str; *ptr !='\0'; ptr++) *ptr=tolower(*ptr);
    return(str);
}


char *strupr(char *str)
{
    char *ptr;
    if (! str) return(NULL);
    for (ptr=str; *ptr !='\0'; ptr++) *ptr=toupper(*ptr);
    return(str);
}


char *strrep(char *str, char oldchar, char newchar)
{
    char *ptr;

    if (! str) return(NULL);
    for (ptr=str; *ptr !='\0'; ptr++)
    {
        if (*ptr==oldchar) *ptr=newchar;
    }
    return(str);
}

char *strmrep(char *str, char *oldchars, char newchar)
{
    char *ptr;

    if (! str) return(NULL);
    for (ptr=str; *ptr !='\0'; ptr++)
    {
        if (strchr(oldchars,*ptr)) *ptr=newchar;
    }
    return(str);
}


//These functions return str to allow easy use in languages like lua where the string object
//is opaque and we must return a new one like this: StripTrailingWhitespace(CopyStr(NULL,Str));

char *StripTrailingWhitespace(char *str)
{
    size_t len;
    char *ptr;

    len=StrLen(str);
    if (len > 0)
    {
        for(ptr=str+len-1; (ptr >= str) && isspace(*ptr); ptr--) *ptr='\0';
    }

    return(str);
}


char *StripLeadingWhitespace(char *str)
{
    char *ptr, *start=NULL;

    if (str)
    {
        for(ptr=str; *ptr !='\0'; ptr++)
        {
            if ((! start) && (! isspace(*ptr))) start=ptr;
        }

        if (!start) start=ptr;
        memmove(str,start,ptr+1-start);
    }
    return(str);
}



char *StripCRLF(char *Line)
{
    size_t len;
    char *ptr;

    len=StrLen(Line);
    if (len > 0)
    {
        for (ptr=Line+len-1; ptr >= Line; ptr--)
        {
            if (strchr("\n\r",*ptr)) *ptr='\0';
            else break;
        }
    }

    return(Line);
}


char *StripQuotes(char *Str)
{
    int len;
    char *ptr, StartQuote='\0';

    ptr=Str;
    while (isspace(*ptr)) ptr++;

    if ((*ptr=='"') || (*ptr=='\''))
    {
        StartQuote=*ptr;
        len=StrLen(ptr);
        if ((len > 0) && (StartQuote != '\0') && (ptr[len-1]==StartQuote))
        {
            if (ptr[len-1]==StartQuote) ptr[len-1]='\0';
            memmove(Str,ptr+1,len);
        }
    }
    return(Str);
}



char *QuoteCharsInStr(char *Buffer, const char *String, const char *QuoteChars)
{
    char *RetStr=NULL;
    const char *sptr, *cptr;
    size_t olen=0;

    RetStr=CopyStr(Buffer,"");
    if (! String) return(RetStr);

    for (sptr=String; *sptr !='\0'; sptr++)
    {
        if (strchr(QuoteChars, *sptr))
        {
            RetStr=AddCharToBuffer(RetStr,olen, '\\');
            olen++;
            switch (*sptr)
            {
            case '\n':
                RetStr=AddCharToBuffer(RetStr,olen,'n');
                break;

            case '\r':
                RetStr=AddCharToBuffer(RetStr,olen,'r');
                break;

            default:
                RetStr=AddCharToBuffer(RetStr,olen,*sptr);
                break;
            }
        }
        //if we didn't find a matching char in QuoteChars then just add to string
        else RetStr=AddCharToBuffer(RetStr,olen,*sptr);
        olen++;
    }

    return(RetStr);
}





int MatchTokenFromList(const char *Token,const char **List, int Flags)
{
    int count;
    size_t tlen, ilen;
    char Up1stChar, Lwr1stChar;
    const char *Item;

    if ((! Token) || (*Token=='\0')) return(-1);
    Up1stChar=toupper(*Token);
    Lwr1stChar=tolower(*Token);
    tlen=StrLen(Token);
    for (count=0; List[count] !=NULL; count++)
    {
        Item=List[count];
        if ((*Item==Lwr1stChar) || (*Item==Up1stChar))
        {
            ilen=StrLen(Item);

            if (Flags & MATCH_TOKEN_PART)
            {
                if (tlen < ilen) continue;
                if (Flags & MATCH_TOKEN_CASE)
                {
                    if (strncmp(Token,Item,ilen)==0) return(count);
                }
                else if (strncasecmp(Token,Item,ilen)==0) return(count);
            }
            else
            {
                if (tlen != ilen) continue;
                if (Flags & MATCH_TOKEN_CASE)
                {
                    if(strcmp(Token,List[count])==0)  return(count);
                }
                else if (strcasecmp(Token,List[count])==0) return(count);
            }
        }
    }
    return(-1);
}






#define ESC 0x1B

char *UnQuoteStr(char *Buffer, const char *Line)
{
    char *out, *in;
    size_t olen=0;
    char hex[10];

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

            //hexadecimal
            case 'x':
                ptr_incr((const char **) &in, 1);
                hex[0]=*in;
                ptr_incr((const char **) &in, 1);
                hex[1]=*in;
                hex[2]='\0';
                out=AddCharToBuffer(out,olen,strtol(hex,NULL,16) & 0xFF);
                olen++;
                break;

            //unicode
            case 'u':
                ptr_incr((const char **) &in, 1);
                strncpy(hex, in, 4);
                hex[4]='\0';
                ptr_incr((const char **) &in, 3);
                out=StrAddUnicodeChar(out, strtol(hex,NULL,16));
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





int strntol(const char **ptr, int len, int radix, long *value)
{
    const char *end;
    char *Tempstr=NULL;

    end=*ptr;
    if (! ptr_incr(&end, len))
    {
        *ptr=end;
        return(FALSE);
    }
    Tempstr=CopyStrLen(Tempstr,*ptr,len);
    strlwr(Tempstr);
    *value=strtol(Tempstr,NULL,radix);

    DestroyString(Tempstr);
    *ptr=end;
    return(TRUE);
}


int istext(const char *Str)
{
    const char *ptr;

    for (ptr=Str; *ptr != '\0'; ptr++)
    {
        if (! isalpha(*ptr)) return(FALSE);
    }
    return(TRUE);
}

int isnum(const char *Str)
{
    const char *ptr;

    for (ptr=Str; *ptr != '\0'; ptr++)
    {
        if (! isdigit(*ptr)) return(FALSE);
    }
    return(TRUE);
}
