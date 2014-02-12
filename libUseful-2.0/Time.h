#ifndef LIBUSEFUL_TIME_H
#define LIBUSEFUL_TIME_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif


char *GetDateStrFromSecs(char *DateFormat, time_t Secs, char *TimeZone);
char *GetDateStr(char *DateFormat, char *TimeZone);
time_t DateStrToSecs(char *DateFormat, char *Str, char *TimeZone);
void SetTimeout(int timeout);


#ifdef __cplusplus
}
#endif



#endif
