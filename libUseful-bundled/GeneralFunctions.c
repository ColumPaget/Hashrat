#include "includes.h"
#include "Encodings.h"
#include "Hash.h"
#include "Time.h"
#include "StrLenCache.h"




inline void Destroy(void *Obj)
{
    if (Obj)
    {
        //If the object is a cached string, the remove it from cache
        StrLenCacheDel(Obj);
        free(Obj);
    }
}


uint32_t reverse_uint32(uint32_t Input)
{
    uint32_t Output;

    Output = (Input & 0xFF) << 24;
    Output |= (Input & 0xFF00) << 8;
    Output |= (Input & 0xFF0000) >> 8;
    Output |= (Input & 0xFF000000) >> 24;

    return(Output);
}


//parse up to eight bits expressed as '1' and '0'
uint8_t parse_bcd_byte(const char *In)
{
    uint8_t val=0;
    int bit, len, i;
    const char *ptr;

    len=StrLen(In);
    if (len < 1) return(0);
    if (len > 8) len=8;
    bit=1 << (len -1);

    ptr=In;
    for (i=0; i < len; i++)
    {
        if (*ptr=='1') val |= bit;
        ptr++;
        bit=bit >> 1;
    }

    return(val);
}



char *encode_bcd_bytes(char *RetStr, unsigned const char *Bytes, int Len)
{
    unsigned const char *ptr, *end;
    unsigned char *optr;
    int bit, i, ilen, olen;

    end=Bytes+Len;

    //BEWARE: We cannot 'point' optr into RetStr until we call 'SetStrLen', as
    //RetStr might be NULL before then, or SetStrLen might realloc and change its address
    ilen = StrLen(RetStr);
    olen = ilen + (Len * 8);
    RetStr=SetStrLen(RetStr, olen);
    optr=RetStr + ilen;

    for (ptr=Bytes; ptr < end; ptr++)
    {
        bit=128;
        for (i=0; i < 8; i++)
        {
            if (*ptr & bit) *optr='1';
            else *optr='0';
            bit = bit >> 1;
            optr++;
        }
    }
    *optr='\0';

    return(RetStr);
}


//xmemset uses a 'volatile' pointer so that it won't be optimized out
void xmemset(volatile char *Str, char fill, off_t size)
{
    volatile char *p;

    for (p=Str; p < (Str+size); p++) *p=fill;
}

int xsetenv(const char *Name, const char *Value)
{
    if (setenv(Name, Value,TRUE)==0) return(TRUE);
    return(FALSE);
}


//increment ptr but don't go past '\0'
//we expect this function to be frequently called, so we inline it
inline int ptr_incr(const char **ptr, int count)
{
    const char *end;

    end=(*ptr) + count;
    for (; (*ptr) < end; (*ptr)++)
    {
        if ((**ptr) == '\0') return(FALSE);
    }
    return(TRUE);
}


//increment ptr until either the terminator is encountered, or we hit '\0'
//we expect this function to be frequently called, so we inline it
inline const char *traverse_until(const char *ptr, char terminator)
{
    while ((*ptr != terminator) && (*ptr != '\0'))
    {
        //handle quoted chars
        if ((*ptr=='\\') && (*(ptr+1) != '\0')) ptr++;
        ptr++;
    }
    return(ptr);
}

//given that the first character is some kind of quote,
//safely traverse the string until we encounter another
//we expect this function to be frequently called, so we inline it
inline const char *traverse_quoted(const char *ptr)
{
    char Quote;

    Quote=*ptr;
    ptr++;
    return(traverse_until(ptr, Quote));
}


#define FNV_INIT_VAL 2166136261
unsigned int fnv_hash(unsigned const char *key, int NoOfItems)
{
    unsigned const char *p;
    unsigned int h = FNV_INIT_VAL;

    for (p=key; *p !='\0' ; p++ ) h = ( h * 16777619 ) ^ *p;

    return(h % NoOfItems);
}



char *CommaList(char *RetStr, const char *AddStr)
{
    if (StrValid(RetStr)) RetStr=MCatStr(RetStr,", ",AddStr,NULL);
    else RetStr=CopyStr(RetStr,AddStr);
    return(RetStr);
}





//remap one fd to another, usually used to change stdin, stdout or stderr
int fd_remap(int fd, int newfd)
{
    int result;

    close(fd);
    result=dup(newfd);
    return(result);
}

//open a file and *only* remap it to the given fd if it opens successfully
int fd_remap_path(int fd, const char *Path, int Flags)
{
    int newfd;
    int result;

    newfd=open(Path, Flags);
    if (newfd==-1) return(FALSE);
    result=fd_remap(fd, newfd);
    close(newfd);

    return(result);
}




//This Function eliminates characters from a string that can be used to trivially achieve code-exec via the shell
char *MakeShellSafeString(char *RetStr, const char *String, int SafeLevel)
{
    char *Tempstr=NULL;
    char *BadChars=";|&`$";
    int ErrFlags=0;

    if (SafeLevel==SHELLSAFE_BLANK)
    {
        Tempstr=CopyStr(RetStr,String);
        strmrep(Tempstr,BadChars,' ');
    }
    else Tempstr=QuoteCharsInStr(RetStr,String,BadChars);


    if ( (SafeLevel & (SHELLSAFE_REPORT | SHELLSAFE_ABORT)) && (CompareStr(Tempstr,String) !=0) )
    {
        ErrFlags=ERRFLAG_SYSLOG;
        if (SafeLevel & SHELLSAFE_ABORT) ErrFlags |= ERRFLAG_ABORT;
        RaiseError(ErrFlags, "MakeShellSafeString", "unsafe chars found in %s", String);
    }
    return(Tempstr);
}



