#include "includes.h"
#include "Encodings.h"
#include "Hash.h"
#include "Time.h"
#include "StrLenCache.h"




void Destroy(void *Obj)
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
int bit, i;

end=Bytes+Len;

for (ptr=Bytes; ptr < end; ptr++)
{
bit=128;
for (i=0; i < 8; i++)
{
	if (*ptr & bit) RetStr=CatStr(RetStr, "1");
	else RetStr=CatStr(RetStr, "0");
	bit = bit >> 1;
}
}

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

int ptr_incr(const char **ptr, int count)
{
    const char *end;

    end=(*ptr) + count;
    for (; (*ptr) < end; (*ptr)++)
    {
        if ((**ptr) == '\0') return(FALSE);
    }
    return(TRUE);
}

const char *traverse_until(const char *ptr, char terminator)
{
    while ((*ptr != terminator) && (*ptr != '\0'))
    {
        //handle quoted chars
        if ((*ptr=='\\') && (*(ptr+1) != '\0')) ptr++;
        ptr++;
    }
    return(ptr);
}


const char *traverse_quoted(const char *ptr)
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
    char *BadChars=";|&`";

    if (SafeLevel==SHELLSAFE_BLANK)
    {
        Tempstr=CopyStr(RetStr,String);
        strmrep(Tempstr,BadChars,' ');
    }
    else Tempstr=QuoteCharsInStr(RetStr,String,BadChars);

    if (CompareStr(Tempstr,String) !=0)
    {
        //if (EventCallback) EventCallback(String);
    }
    return(Tempstr);
}



