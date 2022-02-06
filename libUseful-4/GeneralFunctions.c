#include "includes.h"
#include "Encodings.h"
#include "Hash.h"
#include "Time.h"

#include <sys/utsname.h>


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


void *ArrayGetItem(void *array[], int pos)
{
    int i;
    for (i=0; i <= pos; i++)
    {
        if (array[i]==NULL) return(NULL);
        if (i==pos) return(array[i]);
    }
    return(NULL);
}



//remap one fd to another, usually used to change stdin, stdout or stderr
int fd_remap(int fd, int newfd)
{
    close(fd);
    dup(newfd);
    return(TRUE);
}

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



double ToPower(double val, double power)
{
    double result=0;
    int i;

    result=val;
    for (i=1; i < power; i++)
    {
        result=result * val;
    }

    return(result);
}


double FromSIUnit(const char *Data, int Base)
{
    double val;
    char *ptr=NULL;

    val=strtod(Data,&ptr);
    while (isspace(*ptr)) ptr++;
    switch (*ptr)
    {
    case 'k':
        val=val * Base;
        break;
    case 'M':
        val=val * ToPower(Base,2);
        break;
    case 'G':
        val=val * ToPower(Base,3);
        break;
    case 'T':
        val=val * ToPower(Base,4);
        break;
    case 'P':
        val=val * ToPower(Base,5);
        break;
    case 'E':
        val=val * ToPower(Base,6);
        break;
    case 'Z':
        val=val * ToPower(Base,7);
        break;
    case 'Y':
        val=val * ToPower(Base,8);
        break;
    }

    return(val);
}



const char *ToSIUnit(double Value, int Base, int Precision)
{
    static char *Str=NULL;
    char *Fmt=NULL;
    double next;
//Set to 0 to keep valgrind happy
    int i=0;
    char suffix=' ', *sufflist=" kMGTPEZY";


    for (i=0; sufflist[i] !='\0'; i++)
    {
        next=ToPower(Base, i+1);
        if (next > Value) break;
    }

    if ((i > 0) && (sufflist[i] !='\0'))
    {
        Value=Value / ToPower(Base, i);
        suffix=sufflist[i];
        Fmt=FormatStr(Fmt, "%%0.%df%%c", Precision);
        Str=FormatStr(Str,Fmt,(float) Value,suffix);
    }
    else
    {
        //here 'next' is the remainder, by casting 'Value' to a long we remove the
        //decimal component, then subtract from Value. This leaves us with *only*
        //the decimal places
        next=Value - (long) Value;
        if (Precision==0) Str=FormatStr(Str,"%ld",(long) Value);
        else
        {
            Fmt=FormatStr(Fmt, "%%0.%df", Precision);
            Str=FormatStr(Str,Fmt,(float) Value);
        }
    }


    DestroyString(Fmt);
    return(Str);
}



int LookupUID(const char *User)
{
    struct passwd *pwent;

    if (! StrValid(User)) return(-1);
    pwent=getpwnam(User);
    if (! pwent) return(-1);
    return(pwent->pw_uid);
}


int LookupGID(const char *Group)
{
    struct group *grent;

    if (! StrValid(Group)) return(-1);
    grent=getgrnam(Group);
    if (! grent) return(-1);
    return(grent->gr_gid);
}


const char *LookupUserName(uid_t uid)
{
    struct passwd *pwent;

    pwent=getpwuid(uid);
    if (! pwent) return("");
    return(pwent->pw_name);
}


const char *LookupGroupName(gid_t gid)
{
    struct group *grent;

    grent=getgrgid(gid);
    if (! grent) return("");
    return(grent->gr_name);
}



int GenerateRandomBytes(char **RetBuff, int ReqLen, int Encoding)
{
    struct utsname uts;
    int i, len;
    clock_t ClocksStart, ClocksEnd;
    char *Tempstr=NULL, *RandomBytes=NULL;
    int fd;


    fd=open("/dev/urandom",O_RDONLY);
    if (fd > -1)
    {
        RandomBytes=SetStrLen(RandomBytes,ReqLen);
        len=read(fd,RandomBytes,ReqLen);
        close(fd);
    }
    else
    {
        ClocksStart=clock();
        //how many clock cycles used here will depend on overall
        //machine activity/performance/number of running processes
        for (i=0; i < 100; i++) sleep(0);
        uname(&uts);
        ClocksEnd=clock();


        Tempstr=FormatStr(Tempstr,"%lu:%lu:%lu:%lu:%llu\n",getpid(),getuid(),ClocksStart,ClocksEnd,GetTime(TIME_MILLISECS));
        //This stuff should be unique to a machine
        Tempstr=CatStr(Tempstr, uts.sysname);
        Tempstr=CatStr(Tempstr, uts.nodename);
        Tempstr=CatStr(Tempstr, uts.machine);
        Tempstr=CatStr(Tempstr, uts.release);
        Tempstr=CatStr(Tempstr, uts.version);


        len=HashBytes(&RandomBytes, "sha256", Tempstr, StrLen(Tempstr), 0);
        if (len > ReqLen) len=ReqLen;
    }


    *RetBuff=EncodeBytes(*RetBuff, RandomBytes, len, Encoding);

    DestroyString(Tempstr);
    DestroyString(RandomBytes);

    return(len);
}




char *GetRandomData(char *RetBuff, int len, char *AllowedChars)
{
    int fd;
    char *Tempstr=NULL, *RetStr=NULL;
    int i;
    uint8_t val, max_val;

    srand(time(NULL));
    max_val=StrLen(AllowedChars);

    RetStr=CopyStr(RetBuff,"");
    fd=open("/dev/urandom",O_RDONLY);
    for (i=0; i < len ; i++)
    {
        if (fd > -1) read(fd,&val,1);
        else val=rand();

        RetStr=AddCharToStr(RetStr,AllowedChars[val % max_val]);
    }

    if (fd) close(fd);

    DestroyString(Tempstr);
    return(RetStr);
}


char *GetRandomHexStr(char *RetBuff, int len)
{
    return(GetRandomData(RetBuff,len,HEX_CHARS));
}


char *GetRandomAlphabetStr(char *RetBuff, int len)
{
    return(GetRandomData(RetBuff,len,ALPHA_CHARS));
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

    if (strcmp(Tempstr,String) !=0)
    {
        //if (EventCallback) EventCallback(String);
    }
    return(Tempstr);
}



