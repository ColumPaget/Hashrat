/*
Copyright (c) 2025 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_LINEEDIT_H
#define LIBUSEFUL_LINEEDIT_H

/* 
This module supplies a line-editor with a history. 

Example code using this can be found in Terminal.h in 'TerminalReadText' or in examples/EditLine.c

The 'LineEditCreate' command can be passed the flags:

LINE_EDIT_HISTORY    - provide an recallable history of entered lines
LINE_EDIT_NOMOVE     - don't allow use of left/right to move back into the text
                       this is used for password entry text that is invisible and
                       thus we cannot allow the user to use full editing, as they
                       can become 'lost'.

The 'EditLineHandleChar' function does most of the work, it expects to receive
the following characters (defined in KeyCodes.h)


TKEY_LEFT      -    move cursor left within text
TKEY_RIGHT     -    move cursor right within text
TKEY_UP        -    if history mode is active, recall previous entry
TKEY_DOWN      -    if moving through history goto next entry
TKEY_HOME      -    move cursor to start of text
TKEY_END       -    move cursor to end of text
TKEY_BACKSPACE -    backspace delete previous character
TKEY_ERASE     -    erase curr character
TKEY_ENTER     -    return/enter line
TKEY_ESCAPE    -    cancel line editing

The 'EditLineHandleChar' function returns the cursor position within the edited line, or if 'enter' is pressed it will return the value LINE_EDIT_ENTER, if 'escape' is pressed then it will return LINE_EDIT_CANCEL.

When LINE_EDIT_ENTER is called 'LineEditDone' should be used to collect the typed line, as it will reset the LineEdit for a new line to be typed in, and if history mode is active, it will add the typed line to the history.

The 'history' is a List (as in List.c) of named items where the line is added as the 'Tag' for each named node. The 'Item' member of each node is expected to be NULL, which has impacts on 'LineEditDestroy' as mentioned below. The size of the history will be limited (default 30 items) but this limit can be changed with 'LineEditSetMaxHistory'. Setting the maximum history to '0' disables history limits, allowing the history to grow indefinitely.

If the LineEditCreate was called with the flag LINE_EDIT_HISTORY, then 'LineEditDestroy' will attempt to destroy/free the history. However, it will not free any items included as (ListNode *)->Item values. It will only free the list structures themselves, including the 'Tags' that are the history lines. Thus if you add or update history nodes to have their ->Item members point to something, you will have to free that something yourself.

You can swap in externally managed histories with 'LineEditSwapHistory', which returns the old history that's been swapped out. In this situation it's best to create the LineEdit without the LINE_EDIT_HISTORY flag, so that LineEditDestroy won't try to free the history. The LineEdit system will use the history, adding to it and recalling from it. However, in this situation, where LINE_EDIT_HISTORY was not passed to LineEditCreate, the line edit system will not apply limits to the size of the history. If you want it to apply limits to external histories that are being swapped in and out, you can use LineEditSetMaxHistory to force this feature.
*/


#include "includes.h"
#include "List.h"


#define LINE_EDIT_ENTER  -1
#define LINE_EDIT_CANCEL -2

#define LINE_EDIT_HISTORY    1
#define LINE_EDIT_NOMOVE     2

typedef struct
{
char *Line;
int Flags;
int Len;
int MaxHistory;
int Cursor;
ListNode *History;
} TLineEdit;


#define LineEditSetMaxHistory(LE, Max) ((LE)->MaxHistory = (Max))
#define LineEditGetHistory(LE) ((LE)->History)

TLineEdit *LineEditCreate(int Flags);
void LineEditSetText(TLineEdit *LE, const char *Text);
void LineEditDestroy(TLineEdit *LE);
int LineEditHandleChar(TLineEdit *LE, int Char);
char *LineEditDone(char *RetStr, TLineEdit *LE);
void LineEditClearHistory(TLineEdit *LE);
void LineEditAddToHistory(TLineEdit *LE, const char *Text);

//swap history list for another, return the old one
ListNode *LineEditSwapHistory(TLineEdit *LE, ListNode *NewHist);

#endif
