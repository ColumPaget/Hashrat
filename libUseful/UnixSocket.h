/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_UNIXSOCK_H
#define LIBUSEFUL_UNIXSOCK_H

#include "Stream.h"
#include "defines.h"
#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

//create a unix (filesystem not network) socket. 'Type' can be 'SOCK_STREAM' or 'SOCK_DGRAM'.
//return value is file descriptor of socket, or -1 on error
int UnixServerInit(int Type, const char *Path);

//accept a new connection on a unix server sock. return value is file descriptor for connection
//or -1 on error
int UnixServerAccept(int ServerSock);

/*you shouldn't normally need to use these functions, the same thing can be achieved by

	S=STREAMOpen("unix:/tmp/mysock", "");

or

	S=STREAMOpen("unixdgram:/tmp/mysock", "");
*/

int OpenUnixSocket(const char *Path, int SockType);
int STREAMConnectUnixSocket(STREAM *S, const char *Path, int SockType);


#ifdef __cplusplus
}
#endif


#endif
