/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SOCK
#define LIBUSEFUL_SOCK

#include "includes.h"

//These flags are mostly used internally and you'll rarely be concerned with them
#define CONNECT_NONBLOCK 1
#define CONNECT_SSL 2
#define CONNECT_HTTP_PROXY 4
#define CONNECT_SOCKS_PROXY 8
#define SOCK_NOKEEPALIVE 16 //by default sockets have TCP keepalive set. This option turns it off
#define SOCK_BROADCAST 32   //used internally to create broadcast sockets
#define SOCK_DONTROUTE 64
#define SOCK_REUSEPORT 128
#define SOCK_PEERCREDS 512  //used with UNIX sockets to get remote user ID
#define CONNECT_ERROR 1024  //report errors even if Error:IgnoreIP is set

#define SOCK_CONNECTED 1
#define SOCK_CONNECTING -1

//this is used only with IPServerInit to create a tcp transparent proxy
#define SOCK_TPROXY -1

#define BIND_LISTEN  1
#define BIND_CLOEXEC 2
#define BIND_RAW 4

//This is not defined on all systems, so add it here
#ifndef HOST_NAME_MAX
#if defined(__APPLE__)
#define HOST_NAME_MAX 255
#else
#define HOST_NAME_MAX 64
#endif /* __APPLE__ */
#endif /* HOST_NAME_MAX */


#ifdef __cplusplus
extern "C" {
#endif

//these functions return TRUE if a string appears to be an IP4 or IP6 address
int IsIP4Address(const char *Str);
int IsIP6Address(const char *Str);

//returns TRUE for either IP6 or IP4
int IsIPAddress(const char *Str);

//return the primary IP bound to an interface (e.g. 127.0.0.1 for lo). Currently only returns IP4 addresses
const char *GetInterfaceIP(const char *Interface);

//interface details as a list of name=value pairs. (Pass in a string for them to be copied into, like 'copy str'
char *GetInterfaceDetails(char *RetStr, const char *Interface);

//look in the kernel's arp cache for the MAC address of an IP. Also can return the network device (eth0 etc) that
//this IP has been seen on. Some communication with the device must have recently happened for this to work.
int GetHostARP(const char *IP, char **Device, char **MAC);

//send an ICMP packet. for 'ping request' type is '8' and code is '0'. for 'ping reply type is '0' and code is '0'.
//Data is contents of packet, for ping this can be anything as ping doesn't care about packet data
int ICMPSend(int sock, const char *Host, int Type, int Code, int TTL, char *Data, int len);

//open a UDP connection. Addr and Port here are the LOCAL addresss and port. Address can be blank to bind to all
//local interfaces, but pass an interface name or IP if you want to bind to only one interface.
int UDPOpen(const char *Addr, int Port,int NonBlock);

//send data to a host on a udp socket
int UDPSend(int sock, const char *Host, int Port, char *Data, int len);

//recv data from a host on a udp socket. Host and Port are filled with details of who the data came from
int UDPRecv(int sock,  char *Buffer, int len, char **Host, int *Port);


// Create a server socket 'Type' can be SOCK_STREAM, SOCK_DGRAM, or SOCK_TPROXY. SOCK_STREAM creates a tcp socket, SOCK_DGRAM a udp
// socket and SOCK_TPROXY a tcp transparent proxy socket. Transparent proxy sockets can have any tcp connection redirected to them
// using iptables.
// 'Address' is a local device name or IP address to bind to, or blank to bind to all local addresses/devices.
// Return value is socket file descriptor
int IPServerInit(int Type, const char *Address, int Port);

//Accept a connection on a ServerSocket previously created by IPServerInit. 'Addr' returns the IP of the remote
//host that is connecting, you can pass NULL if you don't want that. Return value is a new file descriptor for
//the accepted connection
int IPServerAccept(int ServerSock,char **Addr);

STREAM *STREAMServerInit(const char *URL);
STREAM *STREAMServerAccept(STREAM *Serv);

//get endpoint details of a connection. Any of LocalAddress, LocalPort, RemoteAddress and RemotePort can be NULL if
//you don't want that particular info
int GetSockDetails(int fd, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort);

//if using SOCK_TPROXY 'tcp transparent proxying' then we will receive connections that are really intended for elsewhere.
//This function allows looking up what the original intended destination was
int GetSockDestination(int sock, char **Host, int *Port);

//returns TRUE if a socket is connected, FALSE otherwise
int IsSockConnected(int sock);
int STREAMIsConnected(STREAM *S);

// get IP address of peer at other end of a connected socket
const char *GetRemoteIP(int sock);

//lookup the primary IP address of a hostname
const char *LookupHostIP(const char *Host);

//lookup a list of addresses for a hostname
ListNode *LookupHostIPList(const char *Host);


const char *IPStrToHostName(const char *);

//convert an integer representation of IP4 address to a string
const char *IPtoStr(unsigned long);

//convert a string representation of IP4 address to an integer representation
unsigned long StrtoIP(const char *);


//Connect to a host and port. Flags can be a bitmask of CONNECT_NONBLOCK, CONNECT_ERROR, SOCK_DONTROUTE and SOCK_NOKEEPALIVE
int TCPConnect(const char *Host, int Port, int Flags);
int STREAMTCPConnect(STREAM *S, const char *Host, int Port, int TTL, int ToS, int Flags);
int STREAMConnect(STREAM *S, const char *URL, const char *Config);

//these are internal functions that you won't usually be concerned with
int STREAMProtocolConnect(STREAM *S, const char *Proto, const char *Host, unsigned int Port, int Flags);
int STREAMDirectConnect(STREAM *S, const char *URL, int Flags);
int DoPostConnect(STREAM *S, int Flags);


#ifdef __cplusplus
}
#endif


#endif
