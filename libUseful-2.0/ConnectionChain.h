#ifndef LIBUSEFUL_CONNECTCHAIN
#define LIBUSEFUL_CONNECTCHAIN

#define PMATCH_SUBSTR 1
#define PMATCH_NOWILDCARDS 2
#define PMATCH_NOCASE 4
#define PMATCH_NOEXTRACT 8
#define PMATCH_NEWLINEEND 16
#define PMATCH_NOTSTART 32
#define PMATCH_NOTEND 64

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

int STREAMProcessConnectHop(STREAM *S, char *Value, int LastHop);
int STREAMAddConnectionHop(STREAM *S, char *Value);

#ifdef __cplusplus
}
#endif

#endif



