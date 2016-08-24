#ifndef LIBUSEFUL_HTTP_H
#define LIBUSEFUL_HTTP_H

#include "includes.h"
#include "defines.h"
#include "file.h"

#define HTTP_AUTH_BASIC  1
#define HTTP_AUTH_DIGEST 2
#define HTTP_AUTH_TOKEN 4
#define HTTP_AUTH_OAUTH 8
#define HTTP_AUTH_PROXY 64
#define HTTP_AUTH_SENT 128
#define HTTP_AUTH_RETURN 256


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
#define HTTP_NOCACHE 8
#define HTTP_NOCOMPRESS 16
#define HTTP_NOREDIRECT 32
#define HTTP_TRY_HTTPS 64
#define HTTP_REQ_HTTPS 128
#define HTTP_KEEPALIVE 256
#define HTTP_POSTARGS  512
#define HTTP_SSL 1024
#define HTTP_SSL_REWRITE 2048
#define HTTP_PROXY 4096
#define HTTP_TUNNEL 8192
#define HTTP_NODECODE 32768
#define HTTP_NOCOOKIES 65536
#define HTTP_GZIP 1048576
#define HTTP_DEFLATE 2097152
#define HTTP_BZIP2 4194304
#define HTTP_XZ 8388608


#define HTTP_HEADERS_SENT 1
#define HTTP_CLIENTDATA_SENT 2
#define HTTP_HEADERS_READ 4

extern const char *HTTP_AUTH_BY_TOKEN;



typedef struct
{
char *Protocol;
char *Host;
int Port;
char *Method;
char *Doc;
char *Destination;
char *ResponseCode;
int Flags;
int AuthFlags;
int State;
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
ListNode *ServerHeaders;
ListNode *CustomSendHeaders;
char *UserName;
char *Authorization;
char *ProxyAuthorization;
STREAM *S;
} HTTPInfoStruct;

#ifdef __cplusplus
extern "C" {
#endif

char *HTTPQuote(char *RetBuff, const char *Str);
char *HTTPQuoteChars(char *RetBuff, const char *Str, const char *CharList);
char *HTTPUnQuote(char *RetBuff, const char *Str);

void HTTPSetVar(const char *Name, const char *Var);


void HTTPInfoDestroy(void *p_Info);
void HTTPInfoSetValues(HTTPInfoStruct *Info, const char *Host, int Port, const char *Logon, const char *Password, const char *Method, const char *Doc, const char *ContentType, int ContentLength);
void HTTPInfoSetAuth(HTTPInfoStruct *Auth, const char *Logon, const char *Password, int Type);
char *HTTPDigest(char *RetStr, const char *Method, const char *Logon, const char *Password, const char *Realm, const char *Doc, const char *Nonce);

HTTPInfoStruct *HTTPInfoCreate(const char *Protocol, const char *Host, int Port, const char *Logon, const char *Password, const char *Method, const char *Doc, const char *ContentType, int ContentLength);


STREAM *HTTPConnect(HTTPInfoStruct *Info);
STREAM *HTTPTransact(HTTPInfoStruct *Info);
void HTTPInfoSetURL(HTTPInfoStruct *Info, const char *Method, const char *URL);
HTTPInfoStruct *HTTPInfoFromURL(const char *Method, const char *URL);
STREAM *HTTPMethod(const char *Method, const char *URL, const char *Logon, const char *Password, const char *ContentType, const char *ContentData, int ContentLength);
STREAM *HTTPGet(const char *URL, const char *Logon, const char *Password);
STREAM *HTTPPost(const char *URL, const char *Logon, const char *Password, const char *ContentType, const char *Content);
void HTTPSetUserAgent(char *AgentName);
void HTTPSetProxy(char *Proxy);
void HTTPSetFlags(int Flags);
int HTTPGetFlags();

char *HTTPReadDocument(char *RetStr, STREAM *S);
void HTTPCopyToSTREAM(STREAM *Con, STREAM *S);
int HTTPDownload(char *URL, char *Login, char *Password, STREAM *S);

#ifdef __cplusplus
}
#endif


#endif
