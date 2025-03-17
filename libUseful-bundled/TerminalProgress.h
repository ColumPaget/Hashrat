/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

/* This implements a simple terminal progress bar
 *
 * Example Usage:

STREAM *Term;
char *Tempstr=NULL;
TERMPROGRESS *TP;
int i;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS | TERM_SAVEATTRIBS);
TerminalClear(Term);

TP=TerminalProgressCreate(Term, "prompt='progress: ' left-contain=' -=' right-contain='=- ' progress=| width=20");
for (i=0; i <= 20; i++) 
{
Tempstr=FormatStr(Tempstr, "NOW AT: %d", i);
TerminalProgressUpdate(TP, i, 20, Tempstr);
sleep(1);
}

TerminalReset(Term);
}

 *
 *  The 'Config' parameter of TerminalProgressCreate takes the following entries:
 *
 *  prompt=            A text or prompt that preceeds the progress bar
 *  x=                 Position the progress-bar at terminal column 'x'. Without x and/or y the progress-bar will be on the current line
 *  y=                 Position the progress-bar at terminal row 'y'. Without x and/or y the progress-bar will be on the current line
 *  width=             Width in characters of progress-bar NOT INCLUDING prompt, left container, right container etc
 *  progress=          A character to use for the progress part of the bar, defaults to '*'
 *  remain=            A character to use for the remaining part of the bar, defaults to ' '
 *  remainder=         A character to use for the remaining part of the bar, defaults to ' '
 *  remaining=         A character to use for the remaining part of the bar, defaults to ' '
 *  attrib=            Set attributes of REMAINDER part of the bar
 *  progress-attrib=   Set attributes of PROGRESS part of the bar
 *  left-contain=      A string to be the left 'container' of the bar, defaults to ' ['
 *  right-contain=     A string to be the right 'container' of the bar, defaults to '] '
 *
 *  attrib and progress-attrib can be used to create bars based only around colored blocks, like so

TP=TerminalProgressCreate(Term, "prompt='progress: ' progress=' ' attribs=~B progress-attribs=~W left-contain='' right-contain=' ' width=20");
 *
 */


#ifndef LIBUSEFUL_TERMINAL_PROGRESS_H
#define LIBUSEFUL_TERMINAL_PROGRESS_H

#include "includes.h"
#include "Unicode.h"
#include "KeyCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "Terminal.h"
#include "TerminalMenu.h"

#define TERMPROGRESS TERMWIDGET

#define TerminalProgressDestroy TerminalWidgetDestroy
#define TerminalProgressSetOptions TerminalWidgetSetOptions

TERMPROGRESS *TerminalProgressCreate(STREAM *Term, const char *Config);
void TerminalProgressDraw(TERMPROGRESS *TP, float Perc, const char *Info);
char *TerminalProgressProcess(char *RetStr, TERMPROGRESS *TP);
float TerminalProgressUpdate(TERMPROGRESS *TP, int Value, int Max, const char *Info);


#ifdef __cplusplus
}
#endif

#endif
