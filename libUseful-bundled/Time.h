/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_TIME_H
#define LIBUSEFUL_TIME_H

#include "includes.h"
#include "Process.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HOURSECS 3600
#define DAYSECS (24 * 3600)
#define LU_STD_DATE_FMT "%Y/%m/%d %H:%M:%S"

//Flags passed to get time
#define TIME_MILLISECS 1  //return number of milliseconds
#define TIME_CENTISECS 2  //return number of centiseconds
#define TIME_CACHED 1024  //used cached time

// Get current time. By default returns seconds since 1970. Passing the above flags can change that to milliseconds or centiseconds.
// Passing TIME_CACHED causes it to return a cached value for the time (you'd use this in time-critical applications where you're
// looking the time up a lot
uint64_t GetTime(int Flags);

//return a string describing the current date/time, in a format described by 'DateFormat'. 'DateFormat' is an strftime/strptime style format string.
char *GetDateStr(const char *DateFormat, const char *TimeZone);

//in the next two functions 'epoch' is usually Jan 1 1970, but might be some other date on different systems. 
//It's the date that all time is measured from in the operating system or hardware.

//as GetDateStr except the 'Secs' argument (which is number of seconds since epoch, as returned by time(NULL)) specifies the time to format
char *GetDateStrFromSecs(const char *DateFormat, time_t Secs, const char *TimeZone);

//Convert a date/time string 'Str' to number of seconds since epoch
time_t DateStrToSecs(const char *DateFormat, const char *Str, const char *TimeZone);

//Convert a string describing a duration to seconds. String in the form "1d 5h 30m" where m=minutes h=hours d=days w=weeks y=year Y=year (year always 365 days)
time_t ParseDuration(const char *Dur);

//format a string using the substitutions %w %d %h %m %s for weeks, days, hours, minutes and seconds
//if any of the substututions are missing, then their value is carried over to the next highest substitution
//so if there's no '%d', then the 'days' part will be counted in '%h' (hours) 
//or if that is missing in '%m' (mins) or '%s' (seconds) 
const char *FormatDuration(const char *Fmt, time_t Duration);

//convert Time from timezone 'SrcZone' to 'DstZone'
//time must be inf ormat "%Y/%m/%d %H:%M:%S"
char *TimeZoneConvert(char *RetStr, const char *Time, const char *SrcZone, const char *DstZone);

//this sets a SIGALRM timer, causing a signal to be sent to our process after 'timeout' seconds.
//You can either pass a signal handler function, or pass NULL to use the default libUseful internal
//signal handler (SIGNAL_HANDLER_FUNC is of the form 'void MyHandler(int sig)' )
void SetTimeout(int timeout, SIGNAL_HANDLER_FUNC);

//return the offset in seconds of "TimeZone"
long TimezoneOffset(const char *TimeZone);

//convert a milliseconds value to a timeval
void MillisecsToTV(int millisecs, struct timeval *tv);


//given day, month and year, is it the current day? return TRUE if so, FALSE otherwise
int IsToday(int Day, int Month, int Year);


//return TRUE (1) if year is a leap year, FALSE (0) otherwise
int IsLeapYear(unsigned int year);

//return number of days in month (month range 1-12, months < 1 are in previous years and >12 are in subsequent years)
int GetDaysInMonth(int Month, int Year);

//produce a csv of days in month. each line is a week. Any dates starting
//with '-' are in the previous month to the one requested, and any 
//started with '+' are in the next month
char *CalendarFormatCSV(char *RetStr, unsigned int Month, unsigned int Year);


#ifdef __cplusplus
}
#endif



#endif
