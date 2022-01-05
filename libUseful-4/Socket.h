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
#define SOCK_TLS_AUTO 2048
#define SOCK_TCP_NODELAY  4096
#define SOCK_TCP_FASTOPEN 8192

#define SOCK_CONNECTED 1
#define SOCK_CONNECTING -1

//this is used only with IPServerInit to create a tcp transparent proxy
#define SOCK_TPROXY -1

#define BIND_LISTEN  1
#define BIND_CLOEXEC 2
#define BIND_RAW 4
#define BIND_REUSEPORT 8

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


#ifndef HAVE_HTONLL
#if __BIG_ENDIAN__
# define htonll(x) (x)
#else
# define htonll(x) ( (uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32) )
#endif
#endif

#ifndef HAVE_NTOHLL
#if __BIG_ENDIAN__
# define ntohll(x) (x)
#else
# define ntohll(x) ( (uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32) )
#endif
#endif



typedef struct
{
    int Flags;
    int QueueLen;
    int Timeout;
    int Perms;
    int TTL;
    int ToS;
    int Mark;
} TSockSettings;


int SocketParseConfig(const char *Config, TSockSettings *Settings);

// Set a single on/off socket option
int SockSetOpt(int sock, int Opt, const char *Name, int OnOrOff);

// Change socket options. Multiple options can be passed at once in both
// SetFlags and UnsetFlags.
// N.B. Flags/options here are the SOCK_ values defined above, like
// SOCK_BROADCAST, SOCK_DONTROUTE etc, etc
void SockSetOptions(int sock, int SetFlags, int UnsetFlags);


//Mostly used internally. given a socket create a stream and set the Type, Peer, DestIP and DestPort values
STREAM *STREAMFromSock(int sock, int Type, const char *Peer, const char *DestIP, int DestPort);

//Bind a socket. Type will be 'SOCK_STREAM' or 'SOCK_DGRAM', Address and Port are the Address and
//port to bind to (not connect to, bind as in a server or the local end of an outgoing connection)
int BindSock(int Type, const char *Address, int Port, int Flags);

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


//Connect to a host and port. 'Config' is a string consisting of inital flags, followed by name-value pairs
//initial flag chars are:

// r  - 'read' mode (a non-op as all sockets are readable)
// w  - 'write' mode (a non-op as all sockets are writeable)
// n  - nonblocking socket
// E  - report socket connection errors
// k  - TURN OFF socket keep alives
// B  - broadcast socket
// F  - TCP Fastopen
// R  - Don't route (equivalent to applying SOCKOPT_DONTROUTE)
// N  - TCP no-delay (disable Nagle algo)

//Name-value pairs are:

//   ttl=<seconds>       set ttl of socket
//   tos=<value>         set tos of socket
//   mark=<value>        set SOCKOPT_MARK if supported
//   keepalive=<y/n>     turn on/off socket keepalives
//   timeout=<centisecs> connect/read timeout for socket
//
// Example:  TCPConnect("myhost.com", 80, "rF ttl=10 timeout=100 keepalive=n");
int TCPConnect(const char *Host, int Port, const char *Config);
int STREAMNetConnect(STREAM *S, const char *Proto, const char *Host, int Port, const char *Config);
int STREAMConnect(STREAM *S, const char *URL, const char *Config);

//these are internal functions that you won't usually be concerned with
int STREAMProtocolConnect(STREAM *S, const char *URL, const char *Config);
int STREAMDirectConnect(STREAM *S, const char *URL, int Flags);
int DoPostConnect(STREAM *S, int Flags);


#ifdef __cplusplus
}
#endif


#endif
