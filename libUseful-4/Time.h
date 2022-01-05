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

//this sets a SIGALRM timer, causing a signal to be sent to our process after 'timeout' seconds.
//You can either pass a signal handler function, or pass NULL to use the default libUseful internal
//signal handler (SIGNAL_HANDLER_FUNC is of the form 'void MyHandler(int sig)' )
void SetTimeout(int timeout, SIGNAL_HANDLER_FUNC);

long TimezoneOffset(const char *TimeZone);

void MillisecsToTV(int millisecs, struct timeval *tv);

#ifdef __cplusplus
}
#endif



#endif
