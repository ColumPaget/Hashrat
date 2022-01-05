/*
 *
 * You probably have no interest in this module, it provides shared functionality used by TerminalMenu and TerminalChoice
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
char *MenuAttribs;
char *MenuCursorLeft;
char *MenuCursorRight;
char *MenuPadLeft;
char *MenuPadRight;
int Flags;
char *Text;
} TERMWIDGET;

TERMWIDGET *TerminalWidgetCreate(STREAM *Term, const char *Options);
void TerminalWidgetDestroy(void *p_Widget);


#ifdef __cplusplus
}
#endif



#endif
