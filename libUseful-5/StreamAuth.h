/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_STREAM_AUTH_H
#define LIBUSEFUL_STREAM_AUTH_H

#include "Stream.h"

/* Functions supporting some types of authentication against streams.

For http streams password-file authentication is supported. For other network streams
IP address based and ssl certificate based authenticaiton is supported.

The only function you'd normally use here is the 'STREAMIsAuth(STREAM *S)' macro, to check
if a stream has been authenticated using any of these methods. 'STREAMAuth' is usually called
automatically from within STREAMServerAccept, and doesn't need to be re-called.

Both STREAMIsAuth and STREAMAuth return TRUE or FALSE to indicate whether the stream has been authenticated

Example usage:

Serv=STREAMServerNew("http:192.168.0.1", "auth='password-file:/etc/myserver.pw;ip:192.168.0.2,192.168.0.3'");
S=STREAMServerAccept(Serv);
if (StreamIsAuth(S))
{
...
}

This sets up authentication using either a password file (see PasswordFile.h) or by IP address. If the connection is coming from one of the specified IP addresses, or theres a user/password match in the password file, then the connection is authenticated (obviously password-file is only usable because HTTP has a password authentication scheme).

Note usage of semi-colon to seperate authentication types. comma is used to seperate multiple options within an authentication type

Available authentication schemes are:

ip:<address list>                               - authenticate by matching source IP with one of the IPs in a comma-seperated list
password-file:<path>                            - authenticate by using a username/password found in password file at <path>
cert:<subject>                                  - authenticate with a client-certificate that verifies and has the specified subject (usually username)
certificate:<subject>                           - authenticate with a client-certificate that verifies and has the specified subject (usually username)
issuer:<issuer>                                 - authenticate with a client-certificate that verifies and is issued by the specified issuer
basic:<base64 encoded username+ password>       - authenticate using a username and password combo expressed as an HTTP style base64 encoded string
cookie:<cookie value>                           - authenticate using an HTTP cookie that matches <cookie value>

For the 'certificate' and 'issuer' authentication types, the verifiying certificates (CA certificates) can be defined using the 'SSL:VerifyFile' and 'SSL:VerifyDir' options to STREAMServerNew (without these libUseful will default to using the openssl default file, usually stored somewhere like '/etc/ssl/cacert.pem', and which provides CA certificates for well-known certificate authorities.

Example SSL setup:

Serv=STREAMServerNew("tls:192.168.0.1", "auth='cert:bill.blogs;issuer:myCA' ssl:CertFile=/etc/ssl/myhost.crt ssl:KeyFile=/etc/ssl/myhost.key ssl:VerifyFile=/etc/ssl/myCA.ca");
S=STREAMServerAccept(Serv);
if (StreamIsAuth(S))
{
...
}

In this setup we can authenticate using a certificate for 'bill.blogs' that is verified by any CA certificate (in this case that would have to be one in /etc/ssl/myCA.ca) or with any certificate issued by 'myCA' (which again, must be verified against a certificate in /etc/ssl/myCA.ca)


*/




#ifdef __cplusplus
extern "C" {
#endif

#define STREAMIsAuth(S) ((S)->Flags & LU_SS_AUTH)
int STREAMAuth(STREAM *S);

#ifdef __cplusplus
}
#endif


#endif
