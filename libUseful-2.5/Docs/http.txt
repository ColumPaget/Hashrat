#ifndef LIBUSEFUL_HTTP_H
#define LIBUSEFUL_HTTP_H

#include "includes.h"
#include "defines.h"
#include "file.h"

#define HTTP_AUTH_BASIC  1
#define HTTP_AUTH_DIGEST 2
#define HTTP_SENT_AUTH 4
#define HTTP_PROXY_AUTH 8


#define HTTP_OKAY 0
#define HTTP_NOCONNECT 1
#define HTTP_NOTFOUND 2
#define HTTP_REDIRECT 3
#define HTTP_ERROR 4
#define HTTP_CIRCULAR_REDIRECTS 5
#define HTTP_NOTMODIFIED 6

#define HTTP_VER1_0 1
#define HTTP_DEBUG 2
#define HTTP_CHUNKED 4
#define HTTP_HEADERS_SENT 8
#define HTTP_NOCOMPRESS 16
#define HTTP_NOREDIRECT 32
#define HTTP_KEEPALIVE 128
#define HTTP_PROXY 512
#define HTTP_SSL 1024
#define HTTP_SSL_REWRITE 2048
#define HTTP_GZIP 8192
#define HTTP_DEFLATE 16384


typedef struct
{
int Flags;
char *AuthRealm;
char *AuthQOP;
char *AuthNonce;
char *AuthOpaque;
char *Logon;
char *Password;
} HTTPAuthStruct;


typedef struct
{
char *Host;
int Port;
char *Method;
char *Doc;
char *Destination;
char *ResponseCode;
int Flags;
char *RedirectPath;
char *PreviousRedirect;
char *ContentType;
char *Timestamp;
int ContentLength;
int Depth;
char *PostData;
char *PostContentType;
int PostContentLength;
char *Proxy;
time_t IfModifiedSince;
ListNode *CustomSendHeaders;
HTTPAuthStruct *Authorization;
HTTPAuthStruct *ProxyAuthorization;
STREAM *S;
} HTTPInfoStruct;



char *HTTPQuote(char *, char*);
char *HTTPQuoteChars(char *RetBuff, char *Str, char *CharList);
char *HTTPUnQuote(char *, char*);


void HTTPInfoDestroy(void *p_Info);
void HTTPInfoSetValues(HTTPInfoStruct *Info, char *Host, int Port, char *Logon, char *Password, char *Method, char *Doc, char *ContentType, int ContentLength);
HTTPInfoStruct *HTTPInfoCreate(char *Host, int Port, char *Logon, char *Password, char *Method, char *Doc, char *ContentType, int ContentLength);
STREAM *HTTPConnect(HTTPInfoStruct *Info);
STREAM *HTTPTransact(HTTPInfoStruct *Info);
HTTPInfoStruct *HTTPInfoFromURL(char *Method, char *URL);
STREAM *HTTPMethod(char *Method, char *URL, char *Logon, char *Password);
STREAM *HTTPGet(char *URL, char *Logon, char *Password);
STREAM *HTTPPost(char *URL, char *Logon, char *Password, char *ContentType, char *Content);
int HTTPReadBytes(STREAM *Con, char **Buffer);
void HTTPCopyToSTREAM(STREAM *Con, STREAM *S);
int HTTPDownload(char *URL, char *Login, char *Password, STREAM *S);
void HTTPSetUserAgent(char *AgentName);
void HTTPSetProxy(char *Proxy);
void HTTPSetFlags(int Flags);
char *HTTPParseURL(char *URL, char **Proto, char **Host, int *Port, char **Login, char **Password);

#endif
