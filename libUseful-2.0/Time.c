#include "Time.h"

char *GetDateStrFromSecs(char *DateFormat, time_t Secs, char *TimeZone)
{
time_t val;
struct tm *TMS;
static char *Buffer=NULL;
char *Tempstr=NULL;
#define DATE_BUFF_LEN 255

val=Secs;

if (StrLen(TimeZone))
{
if (getenv("TZ")) Tempstr=CopyStr(Tempstr,getenv("TZ"));
setenv("TZ",TimeZone,TRUE);
tzset();
}
TMS=localtime(&val);
if (StrLen(TimeZone))
{
if (! Tempstr) unsetenv("TZ");
else setenv("TZ",Tempstr,TRUE);
tzset();
}

val=StrLen(DateFormat)+ DATE_BUFF_LEN;
Buffer=SetStrLen(Buffer,val);
strftime(Buffer,val,DateFormat,TMS);

DestroyString(Tempstr);
return(Buffer);
}



char *GetDateStr(char *DateFormat, char *TimeZone)
{
time_t Now;

time(&Now);
return(GetDateStrFromSecs(DateFormat, Now, TimeZone));
}


time_t DateStrToSecs(char *DateFormat, char *Str, char *TimeZone)
{
time_t Secs=0;
struct tm TMS;
char *Tempstr=NULL;
int val;

if (StrLen(DateFormat)==0) return(0);
if (StrLen(Str)==0) return(0);

if (StrLen(TimeZone))
{
	if (getenv("TZ")) Tempstr=CopyStr(Tempstr,getenv("TZ"));
	setenv("TZ",TimeZone,TRUE);
	tzset();
}

strptime(Str,DateFormat,&TMS);
TMS.tm_isdst=-1;
Secs=mktime(&TMS);

if (StrLen(TimeZone))
{
	if (! Tempstr) unsetenv("TZ");
	else setenv("TZ",Tempstr,TRUE);
	tzset();
}
return(Secs);
}
/* A general 'Set Timer' function, Useful for timing out */
/* socket connections etc                                */

void SetTimeout(int timeout)
{
struct sigaction SigAct;

SigAct.sa_handler=&ColLibDefaultSignalHandler;
SigAct.sa_flags=SA_RESETHAND;
//SigAct.sa_restorer=NULL;

sigaction(SIGALRM,&SigAct,NULL);
alarm(timeout);
}

