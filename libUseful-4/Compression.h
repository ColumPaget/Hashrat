/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_COMPRESSION_H
#define LIBUSEFUL_COMPRESSION_H

/*
For most users 'zlibProcessorInit' is not important, as it's an
internal function used by 'DataProcessing.h.


The 'Alg' argument of CompressBytes and DeCompressBytes specifies the compression algorithm.
Currently supported are "deflate", "zlib", "gzip", "gz", "bzip2", "bz2" and "xz".

"zlib" is a shorthand for "deflate", "gz" for "gzip", and "bz2" for "bzip2".

bzip2 and xz are currently implemented by spawning a copy of the bzip2 or xz command-line programs.
Only deflate/gzip is implemented at the library level. Thus bz2 and xz won't work if the appropriate
program is not available.

If libUseful has been compiled without zlib, then gzip also falls back to using the gzip program.

CompressBytes and DeCompressBytes both expect 'Out' to be a pointer to a libUseful style string (a block of memory
allocated on the heap using malloc, calloc or realloc. DO NOT USE SIMPLE char ARRAYS ALLOCATED ON THE STACK.

If there is not enough space in this string for the output these functions will use 'realloc' to make space. This
means you can even pass them a NULL string like this:

char *Compressed=NULL;

len=CompressBytes(&Compressed, "gzip", "will it shrink? will it shrink?",31, 1);


CompressBytes will allocate memory as needed. This memory must then be freed with 'free' or 'DestroyString'.

Input data is provided via the 'In' pointer. This can be binary data, so the number of bytes is supplied in 'Len'.

CompressBytes takes a 'Level' value which specifies the tradeoff between speed and output size. For gzip '1' is
fastest, and '9' tells gzip to try it's hardest to produce the smallest output.

The return value of Both CompressBytes and DecompressBytes is the length of data placed in 'Out'.
*/


#include "includes.h"


#ifdef __cplusplus
extern "C" {
#endif

int zlibProcessorInit(TProcessingModule *ProcMod, const char *Args);
int CompressBytes(char **Out, const char *Alg, const char *In, unsigned long Len, int Level);
int DeCompressBytes(char **Out, const char *Alg, const char *In, unsigned long Len);

#ifdef __cplusplus
}
#endif



#endif
