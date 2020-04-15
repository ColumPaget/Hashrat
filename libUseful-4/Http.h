/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

/*

For basic http usage (getting webpages) you don't need to use most of this stuff. You can just pass the URL to 'STREAMOpen' like so:

STREAM *S;
char *Tempstr=NULL;

S=STREAMOpen("https://www.google.com/","r");
if (S)
{
printf("RESPONSE: %s\n",STREAMGetValue("HTTP:ResponseCode"));
Tempstr=STREAMReadLine(Tempstr, S);
printf("1st line: %s\n",Tmepstr);
}


This will return the webpage, handling the HTTP protocol invisibly.

The returned STREAM object will have some values set, which can be obtained with 'STREAMGetValue'. These are:

HTTP:ResponseCode            Response code as in '404' or '200' etc
HTTP:ResponseReason          Response reason (i.e. text that follows response code)
HTTP:<header>                Every header sent by the server is stored like this. So e.g. 'HTTP:Content-Type', 'HTTP:Content-Length'


The URL passed to STREAMOpen can contain a username and password like so:

S=STREAMOpen("https://user:password@myhost.com/","r");

Or if using oauth


S=STREAMOpen("https://myhost.com/","r oauth='myhost-oauth'");

where 'myhost-oauth' is the name of an oauth session previously set up using the functions in 'OAuth.h'


The second argument for 'STREAMOpen' is a config argument. For http/https urls this has the form of an initiaol method indicator, followed by a number of name-value pairs.


The 'method' indicator uses a format similar to 'fopen', which character flags (only one will be honored) specifiying the transaction method

r    GET method (default if no method specified)
w    POST method
W    PUT method
D    DELETE method
P    PATCH method
H    HEAD  method

after this initial argument come name-value pairs with the following values

oauth=<oauth config to use>
content-type=<content type>
content-length=<content length>
user=<username>
useragent=<user agent>
user-agent=<user agent>
hostauth

Note, 'hostauth' is not a name/value pair, just a config flag that enables sending authentication without waiting for a 401 Response from the server. This means that we can't know the authentication realm for the server, and so internally use the hostname as the realm for looking up logon credentials. This is mostly useful for the github api.

*/



#ifndef LIBUSEFUL_HTTP_H
#define LIBUSEFUL_HTTP_H

#include "includes.h"
#include "defines.h"
#include "Stream.h"


//These values can be set either by 'HTTPSetFlags' for all connections or in the 'Flags' member
//of an HTTPInfoStruct for a particular conection


#define HTTP_VER1_0 1  // use HTTP/1.0 rather than HTTP/1.1
#define HTTP_DEBUG 2   // print HTTP headers to stderr
#define HTTP_CHUNKED 4 // this is a flag that indicates chunked transfers are in use, it's not set by the user
#define HTTP_NOCACHE 8 // tell the webserver not to give us cached copies of anything
#define HTTP_NOCOMPRESS 16  // don't use compression
#define HTTP_NOREDIRECT 32  // don't automatically follow redirects
#define HTTP_TRY_HTTPS 64   // try to connect with https even if connection says http
#define HTTP_REQ_HTTPS 128  // fail if connection isn't HTTPS
#define HTTP_KEEPALIVE 256  // use HTTP keepalive 
#define HTTP_POSTARGS  512  
#define HTTP_SSL 1024          //flag indicates SSL is in use, not set by user
#define HTTP_SSL_REWRITE 2048
#define HTTP_PROXY 4096
#define HTTP_TUNNEL 8192
#define HTTP_NODECODE 32768   // don't decode any transfer encoding
#define HTTP_NOCOOKIES 65536  // don't accept cookies
#define HTTP_GZIP 1048576     // not set by user
#define HTTP_DEFLATE 2097152  // not set by user
#define HTTP_BZIP2 4194304    // not set by user
#define HTTP_XZ 8388608       // not set by user



// none of the below flags are set by the user
#define HTTP_AUTH_BASIC  1
#define HTTP_AUTH_DIGEST 2
#define HTTP_AUTH_TOKEN 4
#define HTTP_AUTH_OAUTH 8
#define HTTP_AUTH_HOST 16
#define HTTP_AUTH_PROXY 64
#define HTTP_AUTH_SENT 128
#define HTTP_AUTH_RETURN 256



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
    char *Credentials;
    char *Authorization;
    char *ProxyAuthorization;
    char *ConnectionChain;
		char *UserAgent;
    STREAM *S;
} HTTPInfoStruct;

#ifdef __cplusplus
extern "C" {
#endif

//apply http quoting (e.g. space is '%20') to a string, quoting any character in 'CharList'
// e.g.  Quoted=HTTPQuoteChars(Quoted, "http://user:pass@somewhere.com",":@");
char *HTTPQuoteChars(char *RetBuff, const char *Str, const char *CharList);

//apply http quoting to a string, quoting standard problematic characters
// e.g.  Quoted=HTTPQuote(Quoted, "http://user:pass@somewhere.com");
char *HTTPQuote(char *RetBuff, const char *Str);

//unquote a string that contains HTTP style quoting
// e.g.  Str=HTTPUnQuote(Str, Quoted);
char *HTTPUnQuote(char *RetBuff, const char *Str);

//Create an HTTPInfoStruct from components
HTTPInfoStruct *HTTPInfoCreate(const char *Protocol, const char *Host, int Port, const char *Logon, const char *Password, const char *Method, const char *Doc, const char *ContentType, int ContentLength);

//Create an HTTPInfoStruct from a URL (the URL can include username and password)
HTTPInfoStruct *HTTPInfoFromURL(const char *Method, const char *URL);

void HTTPInfoSetURL(HTTPInfoStruct *Info, const char *Method, const char *URL);
//configure an existing HTTPInfoStruct from a URL (the URL can include username and password)
void HTTPInfoSetValues(HTTPInfoStruct *Info, const char *Host, int Port, const char *Logon, const char *Password, const char *Method, const char *Doc, const char *ContentType, int ContentLength);

//free up all memory associated with an HTTPInfoStruct. The structure is passed as a 'void' argument so that this function can be
//used with 'ListDestroy' as lists store objects as 'void *'
void HTTPInfoDestroy(void *p_Info);


//Called internally by 'STREAMOpen', you should probably use 'STREAMOpen' instead
STREAM *HTTPGet(const char *URL);

//Supports other HTTP Methods than GET. If you're PUT/POST-ing data then the last three arguments provide the data, its content type and its length
STREAM *HTTPMethod(const char *Method, const char *URL, const char *ContentType, const char *ContentData, int ContentLength);

//Does a POST. Expectes textual data and calculates the ContentLength internally
STREAM *HTTPPost(const char *URL, const char *ContentType, const char *Content);

//this is called by STREAMOpen when passed an http or https url, and you should really use that
STREAM *HTTPWithConfig(const char *URL, const char *Config);

STREAM *HTTPConnect(HTTPInfoStruct *Info);
STREAM *HTTPTransact(HTTPInfoStruct *Info);
void HTTPSetFlags(int Flags);
int HTTPGetFlags();


char *HTTPReadDocument(char *RetStr, STREAM *S);
int HTTPCopyToSTREAM(STREAM *Con, STREAM *S);

//Copy the document at URL to the STREAM 'S' that is open for writing
int HTTPDownload(char *URL, STREAM *S);


//libUseful internally caches HTTP cookies. Call this function to clear out that cache.
void HTTPClearCookies();


//Generate an http Disgest authentication string from components. You would almost never use this as this process is done internally
char *HTTPDigest(char *RetStr, const char *Method, const char *Logon, const char *Password, const char *Realm, const char *Doc, const char *Nonce);


void HTTPInfoSetAuth(HTTPInfoStruct *Auth, const char *Logon, const char *Password, int Type);

#ifdef __cplusplus
}
#endif


#endif
