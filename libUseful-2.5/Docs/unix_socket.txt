#ifndef LIBUSEFUL_UNIXSOCK_H
#define LIBUSEFUL_UNIXSOCK_H

#include "file.h"
#include "defines.h"

int InitUnixServerSocket(const char *Path, int SockType);
int OpenUnixSocket(const char *Path, int SockType);
int STREAMConnectUnixSocket(STREAM *S, const char *Path, int SockType);

#endif
