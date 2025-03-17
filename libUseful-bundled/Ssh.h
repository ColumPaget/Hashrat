/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SSH_H
#define LIBUSEFUL_SSH_H

#include "Stream.h"

/*
Connect to an ssh server and run a command. This requires the ssh command-line program to be available.

You will normally not use these functions, instead using something like 'STREAMOpen("ssh://myhost.com:2222", "r bind=192.168.6.1");'

The 'config' argument to SSHConnect and SSHOpen is the same as other STREAMOpen style commands, consisting of a set of fopen-style 'open flags' 
followed by name-value pairs for other settings

Currently recognized 'open flags' are:
'r' -> SF_RDONLY (read a file from remote server)
'w' -> SF_WRONLY (write a file to remote server)
'w' -> SF_APPEND (append to a file to remote server)
'l' -> list files on remote server
'x' or anything else runs a command on the remote server

Currently recognized name-value settings are:

bind=<address>   bind to a local address so that our connection seems to be coming from that address
config=<path>    use ssh config file <path>

Note, such SSH connections CANNOT BE USED WITH CONNECTION CHAINS / PROXIES. They are always direct connections.

if ssh's config system is being used to set up known connections (via ~/.ssh/config) then User, Pass and Port
are not needed, and 'Host' is simply the connection-config name as configured in ~/.ssh/config.

The returned stream can be used with the usual STREAM functions to read/write to and from the Command on the
ssh host. see Stream.h for available functions.
*/

#ifdef __cplusplus
extern "C" {
#endif

STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command, const char *Config);
STREAM *SSHOpen(const char *Host, int Port, const char *User, const char *Pass, const char *Path, const char *Config);
void SSHClose(STREAM *S);

#ifdef __cplusplus
}
#endif

#endif
