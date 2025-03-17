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




int IsToday(int Day, int Month, int Year)
{
    time_t when;
    struct tm *Today;

    //must get 'now' late, or it can be changed by other users of 'localtime'
    when=time(NULL);
    Today=localtime(&when);


    if (
        (Day == Today->tm_mday) &&
        (Month == (Today->tm_mon + 1)) &&
        (Year == Today->tm_year + 1900)
    )
    {
        return(TRUE);
    }

    return(FALSE);
}



int IsLeapYear(unsigned int year)
{
    if ((year % 4) != 0) return(FALSE);

    if ((year % 400) == 0) return(TRUE);
    if ((year % 100) == 0) return(FALSE);

    return(TRUE);
}


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

    val=Secs;
    TMS=localtime(&val);

    val=StrLen(DateFormat)+ DATE_BUFF_LEN;
    Buffer=realloc(Buffer,val +1);
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



const char *FormatDuration(const char *Fmt, time_t Duration)
{
    char *Tempstr=NULL;
    static char *RetStr=NULL;
    unsigned long weeks, days, hours, mins, secs;
    const char *ptr;
    int len=0;


    if (strstr(Fmt, "%w"))
    {
        weeks=Duration / (DAYSECS * 7);
        if (weeks > 0) Duration -= (weeks * DAYSECS * 7);
    }

    if (strstr(Fmt, "%d"))
    {
        days=Duration / DAYSECS;
        if (days > 0) Duration -= (days * DAYSECS);
    }

    if (strstr(Fmt, "%h"))
    {
        hours=Duration / 3600;
        if (hours > 0) Duration -= (hours * 3600);
    }

    if (strstr(Fmt, "%m"))
    {
        mins=Duration / 60;
        if (mins > 0) Duration -= (mins * 60);
    }

    secs=Duration;

    for (ptr=Fmt; *ptr !='\0'; ptr++)
    {
        if (*ptr == '%')
        {
            ptr++;

            //handle problematic case where % is the last char in the string
            if (*ptr == '\0')
            {
                RetStr=AddCharToBuffer(RetStr, len, '%');
                len++;
                break;
            }

            switch (*ptr)
            {
            case 'w':
                Tempstr=FormatStr(Tempstr, "%lu", weeks);
                RetStr=CatStr(RetStr, Tempstr);
                len+=StrLen(Tempstr);
                break;

            case 'd':
                Tempstr=FormatStr(Tempstr, "%lu", days);
                RetStr=CatStr(RetStr, Tempstr);
                len+=StrLen(Tempstr);
                break;

            case 'h':
                Tempstr=FormatStr(Tempstr, "%lu", hours);
                RetStr=CatStr(RetStr, Tempstr);
                len+=StrLen(Tempstr);
                break;

            case 'm':
                Tempstr=FormatStr(Tempstr, "%lu", mins);
                RetStr=CatStr(RetStr, Tempstr);
                len+=StrLen(Tempstr);
                break;

            case 's':
                Tempstr=FormatStr(Tempstr, "%lu", secs);
                RetStr=CatStr(RetStr, Tempstr);
                len+=StrLen(Tempstr);
                break;

            case '%':
                RetStr=AddCharToBuffer(RetStr, len, '%');
                len++;
                break;

            default:
                RetStr=AddCharToBuffer(RetStr, len, '%');
                len++;
                RetStr=AddCharToBuffer(RetStr, len, *ptr);
                len++;
                break;
            }
        }
        else
        {
            RetStr=AddCharToBuffer(RetStr, len, *ptr);
            len++;
        }
    }

    Destroy(Tempstr);

    return(RetStr);
}



long TimezoneOffset(const char *TimeZone)
{
    long Secs=0;
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


int GetDaysInMonth(int Month, int Year)
{
    int DaysInMonth[]= {31,28,31,30,31,30,31,31,30,31,30,31};
    int maxday;


    if (Month < 1)
    {
        Month=12 - Month;
        Year--;
    }

    maxday=DaysInMonth[Month -1];
    //if it's a leap year, then add an extra day to feburary
    if ((Month == 2) && IsLeapYear(Year)) maxday++;

    return(maxday);
}



char *CalendarFormatCSV(char *RetStr, unsigned int Month, unsigned int Year)
{
    struct tm InTM, *OutTM;
    time_t when;
    int i, wday, day, maxday;
    char *Tempstr=NULL;


    RetStr=CopyStr(RetStr, "");
    InTM.tm_sec=0;
    InTM.tm_min=0;
    InTM.tm_hour=0;
    InTM.tm_mday=1;
    InTM.tm_mon=Month-1;
    InTM.tm_year=Year - 1900;
    InTM.tm_isdst=-1;

    when=mktime(&InTM);
    if (when > -1)
    {
        OutTM=localtime(&when);

        //if the calendar starts with days from the previous
        //month, then add those
        maxday=GetDaysInMonth(Month-1, Year);
        for (wday=0; wday < OutTM->tm_wday; wday++)
        {
            //+1 because OutTM->tm_wday - wday will never  be zero within this loop
            Tempstr=FormatStr(Tempstr, "-%2d,", maxday - (OutTM->tm_wday - wday) +1);
            RetStr=CatStr(RetStr, Tempstr);
        }

        //now add actual days of current month
        maxday=GetDaysInMonth(Month, Year);
        for (day=1; day <= maxday; day++)
        {
            Tempstr=FormatStr(Tempstr, "%2d,", day);
            RetStr=CatStr(RetStr, Tempstr);
            wday++;
            if (wday == 7)
            {
                RetStr=CatStr(RetStr, "\n");
                wday=0;
            }
        }

        //add next months days to end
        day=1;
        while (wday < 7)
        {
            Tempstr=FormatStr(Tempstr, "+%2d,", day);
            RetStr=CatStr(RetStr, Tempstr);
            day++;
            wday++;
        }
    }

    Destroy(Tempstr);
    return(RetStr);
}
