
#ifndef LIBUSEFUL_OPENSSL_H
#define LIBUSEFUL_OPENSSL_H
#include "includes.h"

#define LU_SSL_PFS 1
#define LU_SSL_VERIFY_PEER 2

void OpenSSLGenerateDHParams();
void HandleSSLError();
int SSLAvailable();
int DoSSLClientNegotiation(STREAM *S, int Flags);
int DoSSLServerNegotiation(STREAM *S, int Flags);
int STREAMIsPeerAuth(STREAM *S);

#endif
