/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

//These functions relate to adding a 'Data processor' to the stream that
//will decode chunked HTTP transfers. This is an automatic component of
//HTTP Streams, and thus the user normally doesn't need to use or know about
//this module


#ifndef LIBUSEFUL_HTTP_CHUNKED_H
#define LIBUSEFUL_HTTP_CHUNKED_H

#include "includes.h"
#include "defines.h"
#include "Stream.h"



#ifdef __cplusplus
extern "C" {
#endif

void HTTPAddChunkedProcessor(STREAM *S);


#ifdef __cplusplus
}
#endif


#endif

