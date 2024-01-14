/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_TERMINAL_THEME_H
#define LIBUSEFUL_TERMINAL_THEME_H


#ifdef __cplusplus
extern "C" {
#endif


#include "includes.h"
#include "List.h"
#include "TerminalWidget.h"

ListNode *TerminalThemeInit();
const char *TerminalThemeGet(const char *Name);
void TerminalThemeSet(const char *Name, const char *Value);
void TerminalThemeApply(TERMWIDGET *TW, const char *Type);


#ifdef __cplusplus
}
#endif



#endif


