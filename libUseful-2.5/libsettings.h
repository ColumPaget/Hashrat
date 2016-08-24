#ifndef LIBUSEFUL_SETTINGS
#define LIBUSEFUL_SETTINGS

#include "Vars.h"

/* These functions provide an interface for setting variables that */
/* are used by libUseful itself */

#ifdef __cplusplus
extern "C" {
#endif

ListNode *LibUsefulValuesGetHead();
void LibUsefulSetValue(char *Name, char *Value);
char *LibUsefulGetValue(char *Name);

#ifdef __cplusplus
};
#endif


#endif
