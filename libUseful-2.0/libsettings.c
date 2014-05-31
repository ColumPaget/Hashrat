#include "defines.h"
#include "includes.h"
#include "Vars.h"
#include "libUseful.h"

/* These functions provide an interface for setting variables that */
/* are used by libUseful itself */

ListNode *LibUsefulSettings=NULL;

ListNode *LibUsefulValuesGetHead()
{
return(LibUsefulSettings);
}

void LibUsefulInitSettings()
{
char *Tempstr=NULL;

		LibUsefulSettings=ListCreate();
		SetVar(LibUsefulSettings,"LibUsefulVersion",__LIBUSEFUL_VERSION__);
		Tempstr=MCopyStr(Tempstr,__LIBUSEFUL_BUILD_DATE__," ",__LIBUSEFUL_BUILD_TIME__,NULL);
		SetVar(LibUsefulSettings,"LibUsefulBuildTime",Tempstr);
		DestroyString(Tempstr);
}


void LibUsefulSetValue(char *Name, char *Value)
{
	if (! LibUsefulSettings) LibUsefulInitSettings();
	SetVar(LibUsefulSettings,Name,Value);
}

char *LibUsefulGetValue(char *Name)
{
if (! LibUsefulSettings) LibUsefulInitSettings();

if (!StrLen(Name)) return("");
return(GetVar(LibUsefulSettings,Name));
}

