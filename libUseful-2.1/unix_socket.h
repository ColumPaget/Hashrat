#ifndef LIBUSEFUL_UNIXSOCK_H
#define LIBUSEFUL_UNIXSOCK_H

#include "file.h"
#include "defines.h"
#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif


int OpenUnixSocket(const char *Path, int SockType);
int STREAMConnectUnixSocket(STREAM *S, const char *Path, int SockType);

#ifdef __cplusplus
}
#endif


#endif
