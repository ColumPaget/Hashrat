#include "defines.h"
#include "includes.h"
#include "Vars.h"
#include "libUseful.h"

/* These functions provide an interface for setting variables that */
/* are used by libUseful itself */

int LibUsefulFlags=0;
static ListNode *LibUsefulSettings=NULL;

ListNode *LibUsefulValuesGetHead()
{
    return(LibUsefulSettings);
}

void LibUsefulInitSettings()
{
    char *Tempstr=NULL;

    LibUsefulSettings=ListCreate();
    SetVar(LibUsefulSettings,"LibUseful:Version",__LIBUSEFUL_VERSION__);
    Tempstr=MCopyStr(Tempstr,__LIBUSEFUL_BUILD_DATE__," ",__LIBUSEFUL_BUILD_TIME__,NULL);
    SetVar(LibUsefulSettings,"LibUseful:BuildTime",Tempstr);
    DestroyString(Tempstr);
}

void LibUsefulSetHTTPFlag(int Flag, const char *Value)
{
    int Flags;

    if (strtobool(Value)) Flags=HTTPGetFlags() | Flag;
    else Flags=HTTPGetFlags() & ~Flag;
    HTTPSetFlags(Flags);
}



void LibUsefulSetValue(const char *Name, const char *Value)
{
    if (! LibUsefulSettings) LibUsefulInitSettings();
    if (strcasecmp(Name,"HTTP:Debug")==0) LibUsefulSetHTTPFlag(HTTP_DEBUG, Value);
    if (strcasecmp(Name,"HTTP:NoCookies")==0) LibUsefulSetHTTPFlag(HTTP_NOCOOKIES, Value);
    if (strcasecmp(Name,"HTTP:NoCompress")==0) LibUsefulSetHTTPFlag(HTTP_NOCOMPRESS, Value);
    if (strcasecmp(Name,"HTTP:NoCompression")==0) LibUsefulSetHTTPFlag(HTTP_NOCOMPRESS, Value);
    if (strcasecmp(Name,"HTTP:NoRedirect")==0) LibUsefulSetHTTPFlag(HTTP_NOREDIRECT, Value);
    if (strcasecmp(Name,"HTTP:NoCache")==0) LibUsefulSetHTTPFlag(HTTP_NOCACHE, Value);
    SetVar(LibUsefulSettings,Name,Value);
}

const char *LibUsefulGetValue(const char *Name)
{
    if (! LibUsefulSettings) LibUsefulInitSettings();

    if (!StrLen(Name)) return("");
    return(GetVar(LibUsefulSettings,Name));
}

int LibUsefulGetBool(const char *Name)
{
    return(strtobool(LibUsefulGetValue(Name)));
}


int LibUsefulGetInteger(const char *Name)
{
const char *ptr;

ptr=LibUsefulGetValue(Name);
if (StrValid(ptr)) return(atoi(ptr));
return(0);
}


void LibUsefulAtExit()
{
    if (LibUsefulFlags & LU_CONTAINER) FileSystemUnMount("/","lazy");
}
