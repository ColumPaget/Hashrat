/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_TERMINAL_KEYS_H
#define LIBUSEFUL_TERMINAL_KEYS_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *TerminalTranslateKeyCode(int key);
int TerminalTranslateKeyStrWithMod(const char *str, int *mod);
int TerminalTranslateKeyStr(const char *str);
int TerminalReadCSISeq(STREAM *S);
int TerminalReadOSCSeq(STREAM *S);
int TerminalReadSSCSeq(STREAM *S);

#ifdef __cplusplus
}
#endif



#endif


