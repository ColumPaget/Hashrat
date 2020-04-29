
/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_OPENSSL_H
#define LIBUSEFUL_OPENSSL_H
#include "includes.h"

/*
You should be able to activate SSL on any libUseful stream, except pipes.
For client connections this should be as simple as:

S=STREAMOpen("tcp://myhost.com:8080","");
DoSSLClientNegotiation(S, 0);

actually, you can go even simpler:

S=STREAMOpen("tls://myhost.com:8080","");

For server connections you must supply a certificate/key pair, like this:


fd=IPServerAccept(ListenSocket, &PeerAddress);
if (fd > -1)
{
S=STREAMFromFD(fd);
STREAMSetValue(S,"SSL:CertFile","/etc/ssl/myhost.crt");
STREAMSetValue(S,"SSL:KeyFile","/etc/ssl/myhost.key");
DoSSLServerNegotiation(S, 0);
}


if you want to do certificate verification on a certificate sent by a peer, then you must
supply a link to either a directory containing certs, or a single certificate file that
is a concatanation of certificates:

S=STREAMOpen("tcp://myhost.com:8080","");
STREAMSetValue(S,"SSL:VerifyCertFile","/etc/ssl/cacert.certs");
DoSSLClientNegotiation(S, 0);


S=STREAMOpen("tcp://myhost.com:8080","");
STREAMSetValue(S,"SSL:VerifyCertDir","/etc/ssl/rootcerts/");
DoSSLClientNegotiation(S, 0);


All the values related to SSL can be set globally to be used with all SSL sockets


LibUsefulSetValue("SSL:CertFile" "/etc/ssl/myhost.crt");
LibUsefulSetValue("SSL:KeyFile" "/etc/ssl/myhost.key");
LibUsefulSetValue("SSL:VerifyCertDir", "/etc/ssl/certs/");

fd=IPServerAccept(ListenSocket, &PeerAddress);
if (fd > -1)
{
S=STREAMFromFD(fd);
DoSSLServerNegotiation(S, 0);
}

*/



//Pass these flags to DoSSLServerNegotiation
#define LU_SSL_PFS 1            //use PerfectForwardSecrecy alogorithims
#define LU_SSL_VERIFY_PEER 2    //verify the certificate offered by the peer


#ifdef __cplusplus
extern "C" {
#endif

//Check if SSL Is compiled in/ available for use. returns TRUE or FALSE
int SSLAvailable();

//reseed random number generator. You wouldn't normally call this to be honest
void OpenSSLReseedRandom();

//is peer authenticated. Clients  can use certificate authentication and this function checks if they
//did and if the certificate passed checks
int OpenSSLIsPeerAuth(STREAM *S);

void OpenSSLGenerateDHParams();

//if you connect a stream to something as a client then you can call this function to activate SSL/TLS on the connection
//currently 'Flags' does nothing, but is included for possible future uses
int DoSSLClientNegotiation(STREAM *S, int Flags);

//if you receive a connection, say with IPServerAccept, then apply this function to it to activate SSL/TLS on the connection
//'Flags' can be any of the LU_SSL_ flags listed above
int DoSSLServerNegotiation(STREAM *S, int Flags);

//This is called automatically by STREAMClose. You won't generally explicitly call this.
void OpenSSLClose(STREAM *S);

//call this before doing anything else with a STREAM that's been 'accept'-ed from a server socket. If the stream is encrypted
//with SSL/TLS  this will return TRUE, FALSE otherwise
int OpenSSLAutoDetect(STREAM *S);

#ifdef __cplusplus
}
#endif


#endif
