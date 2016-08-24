#ifndef LIBUSEFUL_SOCK
#define LIBUSEFUL_SOCK

#include <stdio.h>
#include "file.h"

#define CONNECT_NONBLOCK 1
#define CONNECT_SSL 2
#define CONNECT_HTTP_PROXY 4
#define CONNECT_SOCKS_PROXY 8

#define CONNECT_HOP_TCP 1
#define CONNECT_HOP_HTTP_PROXY 2
#define CONNECT_HOP_SSH 3
#define CONNECT_HOP_SHELL_CMD 4
#define CONNECT_HOP_TELNET 5

#define SOCK_CONNECTED 1
#define SOCK_CONNECTING -1

#ifdef __cplusplus
extern "C" {
#endif



/* Server Socket Funcs*/
int InitServerSock(char *Address, int Port);
int InitUnixServerSock(char *Path);
int TCPServerSockAccept(int ServerSock,int *Addr);
int UnixServerSockAccept(int ServerSock);

int GetSockDetails(int fd, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort);

/* Client Socket Funcs*/
int IsSockConnected(int sock);
int ReconnectSock(int sock, char *Host, int Port, int Flags);
int ConnectToHost(char *Host, int Port, int Flags);
/* int CheckForData(int sock); */
int SendText(int sock, char *Text);
int ReadText(int sock, char *Buffer, int MaxLen);
int ReadToCR(int fd, char *Buffer, int MaxLen);


STREAM *STREAMOpenUDP(int Port,int NonBlock);
int STREAMSendDgram(STREAM *S, char *Host, int Port, char *Bytes, int len);
int STREAMConnectToHost(STREAM *S, char *Host, int Port, int Flags);
int STREAMIsConnected(STREAM *S);
int DoPostConnect(STREAM *S, int Flags);
int DoSSLClientNegotiation(STREAM *S, int Flags);
int DoSSLServerNegotiation(STREAM *S, int Flags);
void STREAMSetValue(STREAM *S, char *Name, char *Value);
char *STREAMGetValue(STREAM *S, char *Name);
const char *STREAMQuerySSLCipher(STREAM *S);
int STREAMIsPeerAuth(STREAM *S);

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
void ParseConnectDetails(char *Str, char **Type, char **Host, int *Port, char **User, char **Pass, char **InitDir);


/* IP Address and host lookup functions */
char *LookupHostIP(char *Host);
char *GetRemoteIP(int sock);
char *IPStrToHostName(char *);
char *IPtoStr(unsigned long);
unsigned long StrtoIP(char *);
int IsIPAddress(char *);


int STREAMAddConnectionHop(STREAM *S, char *Value);
void STREAMAddConnectionHopList(STREAM *S, char *HopList);


#ifdef __cplusplus
}
#endif


#endif
