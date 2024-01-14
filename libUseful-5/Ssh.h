/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SSH
#define LIBUSEFUL_SSH

#include "Stream.h"

/*
Connect to an ssh server and run a command. This requires the ssh command-line program to be available.

Note, such SSH connections CANNOT BE USED WITH CONNECTION CHAINS / PROXIES. They are always direct connections.

if ssh's config system is being used to set up known connections (via ~/.ssh/config) then User, Pass and Port
are not needed, and 'Host' is simply the connection-config name as configured in ~/.ssh/config.

The returned stream can be used with the usual STREAM functions to read/write to and from the Command on the
ssh host. see Stream.h for available functions.
*/

#define SSH_CANON_PTY 1  //communicate to ssh program over a canonical pty (honor ctrl-d, ctrl-c etc)
#define SSH_NO_ESCAPE 2  //disable the 'escape character'
#define SSH_COMPRESS  4  //use -C to compress ssh traffic

#ifdef __cplusplus
extern "C" {
#endif

STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command, int Flags);
STREAM *SSHOpen(const char *Host, int Port, const char *User, const char *Pass, const char *Path, int Flags);
void SSHClose(STREAM *S);

#ifdef __cplusplus
}
#endif

#endif
