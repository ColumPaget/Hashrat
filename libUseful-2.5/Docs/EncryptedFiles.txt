#ifndef LIBUSEFUL_ENCRYPTEDFILES_H
#define LIBUSEFUL_ENCRYPTEDFILES_H

#include "DataProcessing.h"
#include "file.h"

#define FLAG_ENCRYPT   1
#define FLAG_DECRYPT   2
#define FLAG_HEXKEY    4
#define FLAG_HEXIV     8
#define FLAG_HEXSALT   16
#define FLAG_VERBOSE   32
#define FLAG_SPEED     64
#define FLAG_NOPAD_DATA 128



#ifdef __cplusplus
extern "C" {
#endif

char *FormatEncryptArgs(char *RetBuff,int Flags, const char *Cipher, const char *Key, const char *InitVector, const char *Salt );
int AddEncryptionHeader(STREAM *S, int Flags, const char *Cipher, const char *Key, const char *InitVector, const char *Salt);
void HandleDecryptionHeader(STREAM *S, const char *Header, const char *Key);



#ifdef __cplusplus
}
#endif


#endif
