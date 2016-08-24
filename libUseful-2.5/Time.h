#ifndef LIBUSEFUL_TIME_H
#define LIBUSEFUL_TIME_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif


#define TIME_MILLISECS 1
#define TIME_CENTISECS 2
#define TIME_CACHED 1024

uint64_t GetTime(int Flags);
char *GetDateStrFromSecs(const char *DateFormat, time_t Secs, const char *TimeZone);
char *GetDateStr(const char *DateFormat, const char *TimeZone);
time_t DateStrToSecs(const char *DateFormat, const char *Str, const char *TimeZone);
void SetTimeout(int timeout);

#ifdef __cplusplus
}
#endif



#endif
