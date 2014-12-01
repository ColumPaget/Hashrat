#ifndef LIBUSEFUL_SOCK
#define LIBUSEFUL_SOCK

#include "includes.h"

#define CONNECT_NONBLOCK 1
#define CONNECT_SSL 2
#define CONNECT_HTTP_PROXY 4
#define CONNECT_SOCKS_PROXY 8

#define SOCK_CONNECTED 1
#define SOCK_CONNECTING -1

#ifdef __cplusplus
extern "C" {
#endif

int IsIP4Address(const char *Str);
int IsIP6Address(const char *Str);
int IsIPAddress(const char *);

const char *GetInterfaceIP(const char *Interface);


/* Server Socket Funcs*/
int InitServerSock(const char *Address, int Port);
int InitUnixServerSock(const char *Path);
int TCPServerSockAccept(int ServerSock,char **Addr);
int UnixServerSockAccept(int ServerSock);

int GetSockDetails(int fd, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort);

/* Client Socket Funcs*/
int IsSockConnected(int sock);
int ReconnectSock(int sock, const char *Host, int Port, int Flags);
int ConnectToHost(const char *Host, int Port, int Flags);
/* int CheckForData(int sock); */
int SendText(int sock, char *Text);
int ReadText(int sock, char *Buffer, int MaxLen);
int ReadToCR(int fd, char *Buffer, int MaxLen);


STREAM *STREAMOpenUDP(int Port,int NonBlock);
int STREAMSendDgram(STREAM *S, const char *Host, int Port, char *Bytes, int len);
int STREAMConnectToHost(STREAM *S, const char *Host, int Port, int Flags);
int STREAMIsConnected(STREAM *S);
int DoPostConnect(STREAM *S, int Flags);

/* Stuff relating to standard inet download procedure (until \r\n.\r\n) */
typedef struct
{
STREAM *Input;
STREAM *Output;
char *TermStr;
int TermPos;
} DownloadContext;

int ProcessIncommingBytes(DownloadContext *);
//int DownloadToDot(int sock, FILE *SaveFile);
int DownloadToDot(STREAM *Connection, STREAM *SaveFile);
int DownloadToTermStr(STREAM *Connection, STREAM *SaveFile, char *TermStr);


/* IP Address and host lookup functions */
char *GetRemoteIP(int sock);
char *LookupHostIP(const char *Host);
char *IPStrToHostName(const char *);
char *IPtoStr(unsigned long);
unsigned long StrtoIP(const char *);


#ifdef __cplusplus
}
#endif


#endif
