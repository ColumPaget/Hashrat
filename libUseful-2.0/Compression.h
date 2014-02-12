#ifndef LIBUSEFUL_COMPRESSION_H
#define LIBUSEFUL_COMPRESSION_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

int CompressBytes(char **Out, char *Alg, char *In, int Len, int Level);

#ifdef __cplusplus
}
#endif



#endif
