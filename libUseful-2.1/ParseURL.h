#ifndef LIBUSEFUL_PARSEURL
#define LIBUSEFUL_PARSEURL

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

void ParseURL(const char *URL, char **Proto, char **Host, char **Port, char **User, char **Password, char **Path, char **Args);
void ParseConnectDetails(const char *Str, char **Type, char **Host, char **Port, char **User, char **Pass, char **InitDir);
void ParseConnectHop(const char *Line, int *Type,  char **Host, char **User, char **Password, char **KeyFile, int *Port);

#ifdef __cplusplus
}
#endif

#endif
