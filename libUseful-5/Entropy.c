#include "Entropy.h"
#include "Encodings.h"
#include "Hash.h"
#include <sys/utsname.h>


#ifdef HAVE_GETENTROPY

static int GetEntropyFromGetEntropy(char *RandomBytes, int ReqLen)
{
    int len=0, chunk, result;

    while (len < ReqLen)
    {
        chunk=ReqLen-len;
        if (chunk > 256) chunk=256;
//get entropy does not return length, instead it returns 0 on success
//and success means it provided the requested number of bytes
        result=getentropy(RandomBytes+len, chunk);
        if (result != 0) break;
        len+=chunk;
    }

    return(len);
}

#endif


static int GetEntropyFromFile(const char *Path, char *RandomBytes, int ReqLen)
{
    int len=0, result;
    int fd;

    fd=open(Path,O_RDONLY);
    if (fd > -1)
    {
        while(len < ReqLen)
        {
            result=read(fd,RandomBytes+len,ReqLen-len);
            if (result < 0) break;
            len+=result;
        }
        close(fd);
    }
    return(len);
}


//desperately try and generate some random bytes if all better methods fail
static int GetEntropyEmergencyFallback(char **RandomBytes, int ReqLen)
{
    clock_t ClocksStart, ClocksEnd;
    char *Tempstr=NULL;
    struct utsname uts;
    int len=0, i;

    ClocksStart=clock();
    //how many clock cycles used here will depend on overall
    //machine activity/performance/number of running processes
    for (i=0; i < 100; i++) sleep(0);
    uname(&uts);
    ClocksEnd=clock();
    srand(time(NULL) + ClocksEnd);

    Tempstr=FormatStr(Tempstr,"%lu:%lu:%lu:%lu:%lu:%llu\n",getpid(),getuid(),rand(),ClocksStart,ClocksEnd,GetTime(TIME_MILLISECS));
    //This stuff should be unique to a machine
    Tempstr=CatStr(Tempstr, uts.sysname);
    Tempstr=CatStr(Tempstr, uts.nodename);
    Tempstr=CatStr(Tempstr, uts.machine);
    Tempstr=CatStr(Tempstr, uts.release);
    Tempstr=CatStr(Tempstr, uts.version);


    len=HashBytes(RandomBytes, "sha512", Tempstr, StrLen(Tempstr), 0);
    if (len > ReqLen) len=ReqLen;

    Destroy(Tempstr);

    return(len);
}


int GenerateRandomBytes(char **RetBuff, int ReqLen, int Encoding)
{
    int len=0;
    char *RandomBytes=NULL;

    RandomBytes=SetStrLen(RandomBytes,ReqLen);
#ifdef HAVE_GETENTROPY
    len=GetEntropyFromGetEntropy(RandomBytes, ReqLen);
#endif
    if (len==0) len=GetEntropyFromFile("/dev/urandom", RandomBytes, ReqLen);
    if (len==0) len=GetEntropyEmergencyFallback(&RandomBytes, ReqLen);

    if (Encoding==0)
    {
        //don't use CopyStrLen, as 'RandomBytes' can include '\0'
        *RetBuff=SetStrLen(*RetBuff, len);
        memcpy(*RetBuff, RandomBytes, len);
    }
    else *RetBuff=EncodeBytes(*RetBuff, RandomBytes, len, Encoding);

    Destroy(RandomBytes);

    return(len);
}




char *GetRandomData(char *RetBuff, int ReqLen, char *AllowedChars)
{
    char *Tempstr=NULL, *RetStr=NULL;
    int i, len;
    uint8_t val, max_val;

    max_val=StrLen(AllowedChars);
    RetStr=CopyStr(RetBuff,"");
    len=GenerateRandomBytes(&Tempstr, ReqLen, 0);

    for (i=0; i < len ; i++)
    {
        val=Tempstr[i];
        RetStr=AddCharToStr(RetStr,AllowedChars[val % max_val]);
    }


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

