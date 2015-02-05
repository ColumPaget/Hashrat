#ifndef LIBUSEFUL_INET_H
#define LIBUSEFUL_INET_H

#include "includes.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

char *ExtractFromWebpage(char *RetStr, char *URL, char *ExtractStr, int MinLength);
char *GetExternalIP(char *RetStr);
int IPGeoLocate(char *IP, ListNode *Vars);

#ifdef __cplusplus
}
#endif


#endif
