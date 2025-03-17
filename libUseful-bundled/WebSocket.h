/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_WEBSOCKET_H
#define LIBUSEFUL_WEBSOCKET_H

#include "Stream.h"


#ifdef __cplusplus
extern "C" {
#endif


int WebSocketSendBytes(STREAM *S, const char *Data, int Len);
int WebSocketReadBytes(STREAM *S, char *Data, int Len);
STREAM *WebSocketOpen(const char *URL, const char *Config);
int WebSocketAccept(STREAM *S);


#ifdef __cplusplus
}
#endif


#endif
