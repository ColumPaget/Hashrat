/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

/*
 *
 * You probably have no interest in this module, it provides shared functionality used by TerminalMenu, TerminalChoice and TerminalProgress
 */


#ifndef LIBUSEFUL_TERMINAL_WIDGET_H
#define LIBUSEFUL_TERMINAL_WIDGET_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TERMWIDGET_POSITIONED 1

typedef struct
{
int x;
int y;
int wid;
int high;
STREAM *Term;
ListNode *Options;
char *Attribs;
char *CursorAttribs;
char *SelectedAttribs;
char *CursorLeft;
char *CursorRight;
int Flags;
char *Text;
} TERMWIDGET;

TERMWIDGET *TerminalWidgetNew(STREAM *Term, int x, int y, int width, int high, const char *Options);
TERMWIDGET *TerminalWidgetCreate(STREAM *Term, const char *Options);
void TerminalWidgetParseConfig(TERMWIDGET *TW, const char *Config);
void TerminalWidgetDestroy(void *p_Widget);

int TerminalWidgetPutLine(TERMWIDGET *TW, int line, const char *Text);
int TerminalWidgetStartLine(TERMWIDGET *TW, int line);


#ifdef __cplusplus
}
#endif



#endif
