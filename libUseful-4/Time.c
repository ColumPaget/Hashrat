#ifndef _XOPEN_SOURCE
//need this defined to get strptime
#define _XOPEN_SOURCE
#endif

#include "Time.h"
#include <time.h>


//we cache seconds because we expect most questions about
//time to be in seconds, and this avoids multiplying millisecs up
static time_t LU_CachedTime=0;
//This is cached millisecs since 1970
static uint64_t LU_CachedMillisecs=0;
static struct tm LU_CachedTM;


uint64_t GetTime(int Flags)
{
    struct timeval tv;

    if ((! (Flags & TIME_CACHED)) || (LU_CachedTime==0) )
    {
        gettimeofday(&tv, NULL);
        LU_CachedTime=tv.tv_sec;
        LU_CachedMillisecs=(tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    }

    if (Flags & TIME_MILLISECS) return(LU_CachedMillisecs);
    else if (Flags & TIME_CENTISECS) return(LU_CachedMillisecs / 10);

    return((uint64_t) LU_CachedTime);
}


char *GetDateStrFromSecs(const char *DateFormat, time_t Secs, const char *TimeZone)
{
    time_t val;
    struct tm *TMS;
    static char *Buffer=NULL;
    char *Tempstr=NULL;
#define DATE_BUFF_LEN 255


    if (StrValid(TimeZone))
    {
        if (getenv("TZ")) Tempstr=CopyStr(Tempstr,getenv("TZ"));
        setenv("TZ",TimeZone,TRUE);
        tzset();
    }

    /*
    if (Secs==LU_CachedTime) TMS=&LU_CachedTM;
    else
    {
    	val=Secs;
    	TMS=localtime(&val);
    	memcpy(&LU_CachedTM, TMS, sizeof(struct tm));
    }
    */

    val=Secs;
    TMS=localtime(&val);

    val=StrLen(DateFormat)+ DATE_BUFF_LEN;
    Buffer=SetStrLen(Buffer,val);
    strftime(Buffer,val,DateFormat,TMS);

    if (StrValid(TimeZone))
    {
        if (! Tempstr) unsetenv("TZ");
        else setenv("TZ",Tempstr,TRUE);
        tzset();
    }

    DestroyString(Tempstr);
    return(Buffer);
}



char *GetDateStr(const char *DateFormat, const char *TimeZone)
{
    return(GetDateStrFromSecs(DateFormat, (time_t) GetTime(0), TimeZone));
}


time_t DateStrToSecs(const char *DateFormat, const char *Str, const char *TimeZone)
{
    time_t Secs=0;
    struct tm TMS;
    char *Tempstr=NULL;

    if (StrEnd(DateFormat)) return(0);
    if (StrEnd(Str)) return(0);

    if (StrValid(TimeZone))
    {
        if (getenv("TZ")) Tempstr=CopyStr(Tempstr,getenv("TZ"));
        setenv("TZ",TimeZone,TRUE);
        tzset();
    }

    memset(&TMS,0,sizeof(struct tm));
    strptime(Str,DateFormat,&TMS);
    TMS.tm_isdst=-1;
    Secs=mktime(&TMS);

    if (StrValid(TimeZone))
    {
        if (! Tempstr) unsetenv("TZ");
        else setenv("TZ",Tempstr,TRUE);
        tzset();
    }
    return(Secs);
}


long TimezoneOffset(const char *TimeZone)
{
    long Secs=0;
    char *Tempstr=NULL;

    if (StrValid(TimeZone))
    {
        if (getenv("TZ")) Tempstr=CopyStr(Tempstr,getenv("TZ"));
        setenv("TZ",TimeZone,TRUE);
        tzset();
    }

//TO DO: portable offset calculation
#ifdef linux
    Secs=timezone;
#endif

    if (StrValid(TimeZone))
    {
        if (! Tempstr) unsetenv("TZ");
        else setenv("TZ",Tempstr,TRUE);
        tzset();
    }

    return(Secs);
}


/* A general 'Set Timer' function, Useful for timing out */
/* socket connections etc                                */
void SetTimeout(int timeout, SIGNAL_HANDLER_FUNC Handler)
{
    struct sigaction SigAct;

    if (Handler) SigAct.sa_handler=Handler;
    else SigAct.sa_handler=&LU_DefaultSignalHandler;

    //SA_RESETHAND is the 'official' flag for the property of firing once
    //but older systems likely use SA_ONESHOT
    #ifdef SA_RESETHAND
    SigAct.sa_flags=SA_RESETHAND;
    #else
      #ifdef SA_ONESHOT
        SigAct.sa_flags=SA_ONESHOT;
      #endif
    #endif
//SigAct.sa_restorer=NULL;

    sigaction(SIGALRM,&SigAct,NULL);
    alarm(timeout);
}

