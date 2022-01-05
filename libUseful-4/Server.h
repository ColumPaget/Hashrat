/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SERVER
#define LIBUSEFUL_SERVER

#include "Socket.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a server socket 'Type' can be SOCK_STREAM, SOCK_DGRAM, or SOCK_TPROXY. SOCK_STREAM creates a tcp socket, SOCK_DGRAM a udp
// socket and SOCK_TPROXY a tcp transparent proxy socket. Transparent proxy sockets can have any tcp connection redirected to them
// using iptables.
// 'Address' is a local device name or IP address to bind to, or blank to bind to all local addresses/devices.
// Return value is socket file descriptor
int IPServerInit(int Type, const char *Address, int Port);
int IPServerNew(int Type, const char *Address, int Port, int Flags);

//Accept a connection on a ServerSocket previously created by IPServerInit. 'Addr' returns the IP of the remote
//host that is connecting, you can pass NULL if you don't want that. Return value is a new file descriptor for
//the accepted connection
int IPServerAccept(int ServerSock,char **Addr);

//STREAMServerInit and STREAMServerNew create server sockets for tcp:// udp:// unix:// unixdgram:// and tproxy:// protocols
STREAM *STREAMServerInit(const char *URL);


//STREAMServerNew takes a Config argument that can contain a flags string, and/or a set of name=value settings

// the 'flags string' is a string containing characters as follows:
//  k - disable tcp keepalives
//  A - Autodetect SSL
//  B - BROADCAST  set udp socket to be a broadcast socket
//  F - Tcp FASTOPEN
//  N - Tcp NODELAY - disable Nagel's algorithm and send data straight away 
//  R - Don't route. All addresses are treated as local 
//  P - REUSE_PORT allows multiple processes to listen on the same port

//Supported name=value pairs are
//listen=<val> allow <val> number of connections waiting to be accepted on a listening server socket
//mode=<perms> set permissions for a unix server socket. '<perms>' can be an octal value (e.g. 666) or a 'rwx' string (e.g. rw-rw-rw-)
//perms=<perms> set permissions for a unix server socket. '<perms>' can be an octal value (e.g. 666) or a 'rwx' string (e.g. rw-rw-rw-)
//permissions=<perms> set permissions for a unix server socket. '<perms>' can be an octal value (e.g. 666) or a 'rwx' string (e.g. rw-rw-rw-)
STREAM *STREAMServerNew(const char *URL, const char *Config);


//Accept a connection on a tcp:// unix:// or tproxy:// socket
STREAM *STREAMServerAccept(STREAM *Serv);

#ifdef __cplusplus
}
#endif


#endif
