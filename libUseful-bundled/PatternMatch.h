/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_MATCHPATTERN
#define LIBUSEFUL_MATCHPATTERN


/*
pmatch functions provide an enhanced form of the fnmatch shell/glob style pattern matching.
This is because, personally, I hate POSIX regular expressions.

SUPPORTED NATCHES ARE:

?      any character
*      any list of characters
^      start-of-string marker
$      end-of-string marker
[..]   list of characters

\a     alert (bell)
\b     backspace
\d     delete
\e     escape
\n     newline
\r     carriage-return
\t     tab
\l     any lowercase character
\A     any alphabetic character
\B     any alphanumeric character
\C     any printable character
\D     any digit
\S     any space
\P     any punctuation
\U     any uppercase character

\+     turn match switch on
\-     turn match switch off

MATCH SWITCHES:

\+C \-C  turn on/off case sensitive matching
\+W \-W  turn on/off wildcards
\+X \-X  turn on/off extraction. When 'off' matching chars will not be added to returned strings
\+O \-O  turn on/off overlapping matches. Overlapping matches are on by default.




Please note that, without the PMATCH_SUBSTR flag being passed, the whole string must match (like glob).

*/


#define PMATCH_SUBSTR 1
#define PMATCH_NOWILDCARDS 2
#define PMATCH_NOCASE 4
#define PMATCH_NOEXTRACT 8
#define PMATCH_NEWLINEEND 16
#define PMATCH_NOTSTART 32
#define PMATCH_NOTEND   64
#define PMATCH_NO_OVERLAP 128
#define PMATCH_SHORT 256


#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char *Start;
    const char *End;
} TPMatch;


//find one match. Start and End can be NULL  if you don't care where the match is
int pmatch_one(const char *Pattern, const char *String, int len, const char **Start, const char **End, int Flags);


//find many matches and return them in the list
int pmatch(const char *Pattern, const char *String, int Len, ListNode *Matches, int Flags);


#ifdef __cplusplus
}
#endif

#endif
