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


void TimeZoneSet(const char *TimeZone)
{
    if (StrValid(TimeZone))
    {
        setenv("TZ",TimeZone,TRUE);
        tzset();
    }
    else unsetenv("TZ");
}


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
#define DATE_BUFF_LEN 255
    time_t val;
    struct tm *TMS;
    static char *Buffer=NULL;
    char *SavedTimeZone=NULL;

    SavedTimeZone=CopyStr(SavedTimeZone, getenv("TZ"));
    TimeZoneSet(TimeZone);

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

    TimeZoneSet(SavedTimeZone);

    DestroyString(SavedTimeZone);
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
    char *SavedTimeZone=NULL;


    if (StrEnd(DateFormat)) return(0);
    if (StrEnd(Str)) return(0);

    SavedTimeZone=CopyStr(SavedTimeZone, getenv("TZ"));
    TimeZoneSet(TimeZone);

    memset(&TMS,0,sizeof(struct tm));
    strptime(Str,DateFormat,&TMS);
    TMS.tm_isdst=-1;
    Secs=mktime(&TMS);

    TimeZoneSet(SavedTimeZone);

    Destroy(SavedTimeZone);

    return(Secs);
}


time_t ParseDuration(const char *Dur)
{
    time_t Result=0, val;
    char *p_next;
    const char *ptr;

    ptr=Dur;
    while (ptr)
    {
        val=strtoul(ptr, &p_next, 10);
        switch (*p_next)
        {
        case 'm':
            val *= 60;
            break;
        case 'h':
            val *= 3600;
            break;
        case 'd':
            val *= DAYSECS;
            break;
        case 'w':
            val *= DAYSECS * 7;
            break;
        case 'y':
            val *= DAYSECS * 365;
            break;
        case 'Y':
            val *= DAYSECS * 365;
            break;
        }
        Result += val;
        if (*p_next =='\0') break;
        p_next++;
        while (isspace(*p_next)) p_next++;
        ptr=p_next;
    }

    return(Result);
}


long TimezoneOffset(const char *TimeZone)
{
    long Secs=0;
    char *Tempstr=NULL;
    char *SavedTimeZone=NULL;

    SavedTimeZone=CopyStr(SavedTimeZone, getenv("TZ"));
    TimeZoneSet(TimeZone);

//TO DO: portable offset calculation
#ifdef linux
    Secs=timezone;
#endif

    TimeZoneSet(SavedTimeZone);
    Destroy(SavedTimeZone);

    return(Secs);
}


char *TimeZoneConvert(char *RetStr, const char *Time, const char *SrcZone, const char *DstZone)
{
    time_t secs;

    secs=DateStrToSecs(LU_STD_DATE_FMT, Time, SrcZone);
    RetStr=CopyStr(RetStr, GetDateStrFromSecs(LU_STD_DATE_FMT, secs, DstZone));

    return(RetStr);
}


void MillisecsToTV(int millisecs, struct timeval *tv)
{
    tv->tv_sec=millisecs / 1000;
    tv->tv_usec=(millisecs - (tv->tv_sec * 1000)) * 1000;
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



