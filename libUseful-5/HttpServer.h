/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_HTTP_SERVER_H
#define LIBUSEFUL_HTTP_SERVER_H

#include "Stream.h"
#include "Vars.h"

/* 
Support for very basic in-app http servers.
Usage is:

Serv=STREAMServerNew("http:127.0.0.1:8080");
S=STREAMServerAccept(Serv);
if (S)
{
if (strcmp(STREAMGetValue("HTTP:URL"), "/test")==0) 
{
  HTTPSendHeaders(S, 200, "OKAY", "X-Test-Header=test");
  STREAMWriteLine("testing 123", S);
  STREAMClose(S);
}
}


STREAMServerAccept handles the HTTP handshake and sets the following variables

HTTP:Method          HTTP Method used (GET, PUT, POST etc)
HTTP:URL             Requested url
Cookie               list of supplied cookies in format name=value;name=value
Auth:Basic           base64 encoded username:password details
Auth:Bearer          oauth 'access token' or other unique secret
WEBSOCKET:KEY        'Key' Used in the weboscket protocol, the programmer does not normally need to know about this
WEBSOCKET:PROTOCOL   Requested websockets protocol, can be used to support multiple websocket subprotocols at the same url
HTTP:Upgrade         Will be set to 'websocket' if client asked to switch to websockets
HTTP:<header>   all other headers in request

These can then be processed by the main program. HTTPSendHeaders is used to send back reply
headers, and then other data can be written to the stream.

Two convienience functions, HTTPSendDocument and HTTPSendFile allow the user to send headers and a document
or the contents of a file at a given path. These functions wrap HTTPSendHeaders, so that doesn't need to 
be used with them. Both have an optional 'content-type' argument, but if this is blank or null then
HTTPSendFile will try to guess the content-type of a file using the file extension.

Authentication can be handled by one of the following methods  

ip:<address>     - IP address of remote host matches one in the list
cookie:<name>    - HTTP cookie is set to value

Serv=STREAMServerNew("http://127.0.0.1:4040", "rw auth='password-file:/tmp/test.pw'");
Connections=ListCreate();
ListAddItem(Connections, Serv);

while (1)
{
STREAMSelect(Connections, NULL);
S=STREAMServerAccept(Serv);
if (! STREAMAuth(S)) HTTPServerSendHeaders(S, 401, "Authentication Required", "WWW-Authenticate=Basic");
else
{

*/




#ifdef __cplusplus
extern "C" {
#endif


void HTTPServerParseClientCookies(ListNode *Vars, const char *Str);
void HTTPServerParseAuthorization(ListNode *Vars, const char *Str);
void HTTPServerParseClientHeaders(STREAM *S);
void HTTPServerSendHeaders(STREAM *S, int ResponseCode, const char *ResponseText, const char *Headers);
void HTTPServerAccept(STREAM *S);

int HTTPServerSendDocument(STREAM *S, const char *Bytes, int Length, const char *ContentType, const char *Headers);
int HTTPServerSendFile(STREAM *S, const char *Path, const char *ContentType, const char *Headers);

#ifdef __cplusplus
}
#endif


#endif
