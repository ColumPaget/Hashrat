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


#ifdef __cplusplus
extern "C" {
#endif

STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command);


#ifdef __cplusplus
}
#endif

#endif
