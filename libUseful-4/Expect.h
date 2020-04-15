/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_EXPECT_H
#define LIBUSEFUL_EXPECT_H

/*

These functions handle 'chat' style dialogues on a STREAM connection.

Read Stream.h to understand the STREAM object. STREAMS can be pipes to other processes, network connections, etc.

*/


#include "Stream.h"

#define DIALOG_END 1
#define DIALOG_FAIL 2
#define DIALOG_OPTIONAL 4
#define DIALOG_TIMEOUT 8

typedef struct
{
    int Flags;
    int Match;
    char *Expect;
    char *Reply;
} TExpectDialog;


#ifdef __cplusplus
extern "C" {
#endif

//Consume data from a stream, waiting for it to be silent for 'wait' seconds, then return;
int STREAMExpectSilence(STREAM *S, int wait);

/*
consume data from a stream, until the string pointed to be 'Expect' is seen. Then send the string
pointed to by 'Reply' and return
*/
int STREAMExpectAndReply(STREAM *S, const char *Expect, const char *Reply);

/*
Add a Expect/Reply pair to a list (See List.h for details of libUseful lists.) A number of these can be
chained together in the list which is then passed to 'STREAMExpectDialog'. 'STREAMExpectDialog' will
work through the list, expecting to match one Expect/Reply pair after another in a conversation of
'expect' matches and sent replies.

The 'Flags' option can have the following values:

DIALOG_END
   If the 'Expect' value is seen, then the dialog has successfully completed. STREAMExpectDialog
	 will immediately return TRUE, even if there are other items yet to be processed

DIALOG_FAIL
   If the 'Expect' value is seen, then the dialog has failed. STREAMExpectDialog will immedialtely
   return FALSE, even if there are other items yet to be processed

DIALOG_OPTIONAL
   If this item isn't seen, but one after it is seen, then skip over this item.

the list used by STREAMExpectDialog should be destroyed like this:

ListDestroy(DialogList, ExpectDialogDestroy);
*/

void ExpectDialogAdd(ListNode *Dialogs, char *Expect, char *Reply, int Flags);
int STREAMExpectDialog(STREAM *S, ListNode *Dialogs, int Timeout);

void ExpectDialogDestroy(void *Item);

#ifdef __cplusplus
}
#endif


#endif
