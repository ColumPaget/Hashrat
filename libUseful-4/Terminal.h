/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

/* 
This module provides various functions related to terminal input and output, particularly terminal colors.

The functions 'TerminalFormatStr' and  'TerminalPutStr' accept a string with backslash escape sequences
and the following 'tilde command' formatting values 


~~        output the tilde character '~'
~r        switch color to red
~R        switch background to red
~g        switch color to green
~G        switch background to green
~b        switch color to blue
~B        switch background to blue
~n        switch color to black ('night' or 'noir')
~N        switch background to black ('night' or 'noir')
~y        switch color to yellow
~Y        switch background to yellow
~m        switch color to magenta
~M        switch background to magenta
~c        switch color to cyan
~C        switch background to cyan
~+R       switch background to bright red
~+g       switch color to bright green
~+G       switch background to bright green
~+b       switch color to bright blue
~+B       switch background to bright blue
~+n       switch color to bright black ('night' or 'noir')
~+N       switch background to bright black ('night' or 'noir')
~+y       switch color to bright yellow
~+Y       switch background to bright yellow
~+m       switch color to bright magenta
~+M       switch background to bright magenta
~+c       switch color to bright cyan
~+C       switch background to bright cyan
~e        switch to bold text
~i        switch to inverse text
~u        switch to underlined text
~<        clear to start of line
~>        clear to end of line
~0        reset all attributes (return to normal text)
~Uxxxx    output a unicode character (xxxx is the 4-digit unicode name)
~:name:   output a unicode character using a name from the unicode names file 

so, for example:

	TerminalPutStr("~y~RYellow text on a red background~0", StdOut);

Escape sequences a-la the 'echo' command are also accepted:

  \\      backslash
  \a      alert (BEL)
  \b      backspace
  \c      produce no further output
  \e      escape
  \f      form feed
  \n      new line
  \r      carriage return
  \t      horizontal tab
  \v      vertical tab
  \0NNN   byte with octal value NNN (1 to 3 digits)
  \xHH    byte with hexadecimal value HH (1 to 2 digits)

This of course means that the final displayed string might be a different length from the formatted string that created it. So the function:

	int TerminalStrLen(const char *Str);

Can be used to get the length of the string that will actually be displayed (with all formatting characters removed)

Unicode characters can be referred to by name like so:

TerminalPutStr("duration: 80~:micro:s", StdOut);

provided that the name 'micro' has been mapped to a name in the unicode names file. This file contains entries in the format <name> <hex code> like so:


micro 00b5
quarter 00bc
half 00bd
3quarter 00be
moon 263E
quarternote 2669
eighthnote 266A
music 266B
beamed8note 266B
beamed16note 266C


The default path for this file is /etc/unicode-names.conf, but it can be overriden either by setting the environment variable 'UNICODE_NAMES_FILE' or by setting the libuseful variable 'Unicode:NamesFile'
*/




#ifndef LIBUSEFUL_TERMINAL_H
#define LIBUSEFUL_TERMINAL_H

#include "includes.h"
#include "Unicode.h"
#include "KeyCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

//These values are passed as Color and BgColor to ANSICode to produce escape sequences with those colors
typedef enum {ANSI_NONE, ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_WHITE, ANSI_RESET, ANSI_RESET2, ANSI_DARKGREY,  ANSI_LIGHTRED, ANSI_LIGHTGREEN, ANSI_LIGHTYELLOW, ANSI_LIGHTBLUE, ANSI_LIGHTMAGENTA, ANSI_LIGHTCYAN, ANSI_LIGHTWHITE} T_ANSI_COLORS;



//These flags are mostly used internally
#define TERM_HIDETEXT  1   //hide text (default is show it)
#define TERM_SHOWSTARS 2   //show stars instead of text (for passwords)
#define TERM_SHOWTEXTSTARS 4 //show stars+last character typed
#define TERM_HIDECURSOR 32   //hide the cursor
#define TERM_RAWKEYS 64      //switch a terminal into 'raw' mode rather than canonical (usually you want this)
#define TERMBAR_UPPER 128
#define TERMBAR_LOWER 256
#define TERM_SAVEATTRIBS 512
#define TERM_SAVE_ATTRIBS 512
#define TERM_MOUSE        1024 //send xterm mouse events for buttons 1 2 and 3
#define TERM_WHEELMOUSE   2048 //send xterm mouse events for buttons 1 2 and 3, and wheel buttons (4 and 5)
#define TERM_ALIGN_CENTER 4096
#define TERM_ALIGN_RIGHT  8192

//These flags can be passed in the Flags argument of ANSICode
#define ANSI_HIDE			65536
#define ANSI_BOLD			131072
#define ANSI_FAINT		262144
#define ANSI_UNDER		524288
#define ANSI_BLINK		1048576
#define ANSI_INVERSE  2097152
#define ANSI_NORM "\x1b[0m"
#define ANSI_BACKSPACE 0x08



#define TerminalSetUTF8(level) (UnicodeSetUTF8(level))

//Commands available via the 'terminal command' function. Normally you wouldn't use these, but use the equivalent macros below
typedef enum {TERM_NORM, TERM_TEXT, TERM_COLOR, TERM_CLEAR_SCREEN, TERM_CLEAR_ENDLINE, TERM_CLEAR_STARTLINE, TERM_CLEAR_LINE, TERM_CURSOR_HOME, TERM_CURSOR_MOVE, TERM_CURSOR_SAVE, TERM_CURSOR_UNSAVE, TERM_CURSOR_HIDE, TERM_CURSOR_SHOW, TERM_SCROLL, TERM_SCROLL_REGION, TERM_UNICODE, TERM_UNICODE_NAME} ETerminalCommands;


typedef struct
{
int flags;
int button;
int x;
int y;
} TMouseEvent;

// pass in ANSI_ flags as listed above and get out an ANSI escape sequence 
char *ANSICode(int Color, int BgColor, int Flags);

//parse a color name ('red', 'yellow' etc) and return the equivalent ANSI_ flag
int ANSIParseColor(const char *Str);

// initialize STREAM to be a terminal. This captures terminal width and height (rows and columns) and sets up the scrolling area.
// Flags can include TERM_HIDECURSOR, to start with cursor hidden, TERM_RAWKEYS to disable 'canonical' mode and get raw keystrokes
// and TERM_BOTTOMBAR to create a region at the bottom of the screen to hold an information or input bar
int TerminalInit(STREAM *S, int Flags);

// Specify if system supports utf8. This is global for all terminals. 'level' can be
//   0 - not supported
//   1 - unicode values below 0x8000 supported
//   2 - unicode values below 0x10000 supported
void TerminalSetUTF8(int level);


//reset terminal values to what they were before 'TerminalInit'. You should call this before exit if you don't want a messed up console.
void TerminalReset(STREAM *S);

//clear screen
#define TerminalClear(S) ( TerminalCommand(TERM_CLEAR_SCREEN, 0, 0, S))

//erase to the end of the current line
#define TerminalEraseLine(S) ( TerminalCommand(TERM_CLEAR_ENDLINE, 0, 0, S))

//hide the cursor
#define TerminalCursorHide(S) ( TerminalCommand(TERM_CURSOR_HIDE, 0, 0, S))

//show the cursor
#define TerminalCursorShow(S) ( TerminalCommand(TERM_CURSOR_SHOW, 0, 0, S))

//move the cursor to column x, row y
#define TerminalCursorMove(S, x, y) ( TerminalCommand(TERM_CURSOR_MOVE, x, y, S))

//save the cursors position. You can then perform operations that move the cursor, and then reset it back to it's saved position
#define TerminalCursorSave(S) ( TerminalCommand(TERM_CURSOR_SAVE, 0, 0, S))

//restore the cursor to the position save with 'TerminalCursorSave'
#define TerminalCursorRestore(S) ( TerminalCommand(TERM_CURSOR_UNSAVE, 0, 0, S))

//scroll the screen by x columns and y rows
#define TerminalScroll(S, x, y) ( TerminalCommand(TERM_SCROLL, y, x, S))

//scroll one line up
#define TerminalScrollUp(S) ( TerminalCommand(TERM_SCROLL, 1, 0, S))

//scroll one line down
#define TerminalScrollDown(S) ( TerminalCommand(TERM_SCROLL, -1, 0, S))

//scroll the screen by x columns and y rows
#define TerminalSetScrollRegion(S, x, y) ( TerminalCommand(TERM_SCROLL_REGION, y, x, S))


//some macros to control xterm windows
#define XtermSetIconNameAndTitle(S, title) ( XtermStringCommand("\x1b]0;", (title), "\007", (S)) )
#define XtermSetIconName(S, title) ( XtermStringCommand("\x1b]1;", (title), "\007", (S)) )
#define XtermSetTitle(S, title) ( XtermStringCommand("\x1b]2;", (title), "\007", (S)) )
#define XtermIconify(S) ((void) STREAMWriteLine("\x1b[2t", (S)))
#define XtermUnIconify(S) ((void) STREAMWriteLine("\x1b[1t", (S)))
#define XtermRaise(S) ((void) STREAMWriteLine("\x1b[5t", (S)))
#define XtermLower(S) ((void) STREAMWriteLine("\x1b[6t", (S)))
#define XtermFullscreen(S) ((void) STREAMWriteLine("\x1[10;1t", (S)))
#define XtermUnFullscreen(S) ((void) STREAMWriteLine("\x1[10;0t", (S)))


void XtermStringCommand(const char *Prefix, const char *Str, const char *Postfix, STREAM *S);

//put a character. Char can be a value outside the ANSI range which will result in an xterm unicode character string being output
void TerminalPutChar(int Char, STREAM *S);

//'Str' is a format string with with 'tilde commands' in it. The ANSI coded result is copied into RetStr, which is resized as needed and returned
char *TerminalFormatStr(char *RetStr, const char *Str, STREAM *Term);

//this function is used internally but is deprecated
const char *TerminalFormatSubStr(const char *Str, char **RetStr, STREAM *Term);

//'Str' is a format string with 'tilde commands' in it. The ANSI coded result is output to stream S
void TerminalPutStr(const char *Str, STREAM *S);


//step past a single character. Understands tilde-strings and (some) unicode, consuming them as one character
int TerminalConsumeCharacter(const char **ptr);

//calculate length of string *after ANSI formating*, so ANSI escape sequences don't count as characters added
//to the length 
int TerminalStrLen(const char *Str);

//truncate a terminal string to a length *handling ANSI formatting*, so ANSI escape sequences don't count to 
//the length to be truncated. This means if you ask for 5 characters, you get five text characters, plus any 
//ansi strings that preceed them
char *TerminalStrTrunc(char *Str, int MaxLen);


//Perform various commands on the terminal (normally you'd just use the macros listed above)
void TerminalCommand(int Cmd, int Arg1, int Arg2, STREAM *S);

//read a single character from the terminal. In addition to the normal ANSI characters this returns the TKEY_ values listed above
//provided the terminal has been initialized with TERM_RAWKEYS. This allows reading keystrokes for use in games etc
int TerminalReadChar(STREAM *S);


/* 
converts a key to a string. For non-printable key values these strings are the same as the #defined keys above, except without
the leading 'TKEY_'. So 'ESC', 'F1' 'SHIFT_F1' 'UP' 'DOWN' etc etc */
const char *TerminalTranslateKeyCode(int key);

int TerminalTranslateKeyStrWithMod(const char *str, int *mod);

int TerminalTranslateKeyStr(const char *str);


//translates a config string into the flags TERM_HIDETEXT, TERM_SHOWSTARS, TERM_SHOWTEXTSTARS
//You will probably use those flags directly, this function is intended for bindings to programming languages
//without good support for bitflags (e.g. lua)
int TerminalTextConfig(const char *Config);

//read a line of text from the terminal. This command reads keystrokes and optionally echoes them. It supports backspace to delete typed
//characters. When enter/newline is pressed the whole string of typed characters is returned. 
//N.B. this requires the termainl to have been initialized with the TERM_RAWKEYS flag set, otherwise the terminal driver will handle this
//job in 'canonical' mode
//Supported Flags are:
//	 TERM_HIDETEXT which causes typed characters to be hidden
//   TERM_SHOWSTARS which echoes '*' instead of the typed character (intended for use with password entry)
//   TERM_SHOWTEXTSTARS which echoes a character, but overwrites it with '*' when the next character is typed, so that only one character of
//       a password is visible at any time
char *TerminalReadText(char *RetStr, int Flags, STREAM *S);

//as TerminalReadText but with a prompt 
char *TerminalReadPrompt(char *RetStr, const char *Prompt, int Flags, STREAM *S);

//get width and height/length of a terminal
void TerminalGeometry(STREAM *S, int *wid, int *len);

//prototype for a callback function that recieves a key press
typedef int (*TKEY_CALLBACK_FUNC)(STREAM *Term, int Key);

//set a callback function that will be passed all key presses
void TerminalSetKeyCallback(STREAM *Term, TKEY_CALLBACK_FUNC Func);

//returns a structure describing the last mouse event. Call this after recieving a MOUSE_BTN keypress event
TMouseEvent *TerminalGetMouse(STREAM *Term);

#ifdef __cplusplus
}
#endif



#endif
