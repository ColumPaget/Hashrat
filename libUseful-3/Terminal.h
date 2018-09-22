/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

/* 
This module provides various functions related to terminal input and output, particularly terminal colors.

The functions 'TerminalFormatStr' and  'TerminalPutStr' accept a string with the following 'tilde command' formatting values 


~~    output the tilde character '~'
~r    switch color to red
~R    switch background to red
~g    switch color to green
~G    switch background to green
~b    switch color to blue
~B    switch background to blue
~n    switch color to black ('night' or 'noir')
~N    switch background to black ('night' or 'noir')
~y    switch color to yellow
~Y    switch background to yellow
~m    switch color to magenta
~M    switch background to magenta
~c    switch color to cyan
~C    switch background to cyan
~e    switch to bold text
~i    switch to inverse text
~u    switch to underlined text
~<    clear to start of line
~>    clear to end of line
~0    reset all attributes (return to normal text)
~U    output a unicode character


so, for example

TerminalPutStr("~y~RYellow text on a red background~0", StdOut);

This of course means that the final displayed string might be a different length from the formatted string that created it. So the function

int TerminalStrLen(const char *Str);

Can be used to get the length of the string that will actually be displayed (with all formatting characters removed)

*/




#ifndef LIBUSEFUL_ANSI_H
#define LIBUSEFUL_ANSI_H

#include "includes.h"
#include "Unicode.h"

#ifdef __cplusplus
extern "C" {
#endif

//These values are passed as Color and BgColor to ANSICode to produce escape sequences with those colors
typedef enum {ANSI_NONE, ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_WHITE, ANSI_RESET, ANSI_RESET2, ANSI_DARKGREY, ANSI_LIGHTRED, ANSI_LIGHTGREEN, ANSI_LIGHTYELLOW, ANSI_LIGHTBLUE, ANSI_LIGHTMAGENTA, ANSI_LIGHTCYAN, ANSI_LIGHTWHITE} T_ANSI_COLORS;



//These flags are mostly used internally
#define TERM_HIDETEXT  1   //hide text (default is show it)
#define TERM_SHOWSTARS 2   //show stars instead of text (for passwords)
#define TERM_SHOWTEXTSTARS 4 //show stars+last character typed
#define TERM_HIDECURSOR 32   //hide the cursor
#define TERM_RAWKEYS 64      //switch a terminal into 'raw' mode rather than canonical (usually you want this)
#define TERMBAR_UPPER 128
#define TERMBAR_LOWER 256
#define TERM_SAVEATTRIBS 512

//These flags can be passed in the Flags argument of ANSICode
#define ANSI_HIDE			65536
#define ANSI_BOLD			131072
#define ANSI_FAINT		262144
#define ANSI_UNDER		524288
#define ANSI_BLINK		1048576
#define ANSI_INVERSE  2097152
#define ANSI_NORM "\x1b[0m"
#define ANSI_BACKSPACE 0x08



//Keycode definitions
#define ESCAPE 0x1b

#define KEY_F1 0x111
#define KEY_F2 0x112
#define KEY_F3 0x113
#define KEY_F4 0x114
#define KEY_F5 0x115
#define KEY_F6 0x116
#define KEY_F7 0x117
#define KEY_F8 0x118
#define KEY_F9 0x119
#define KEY_F10 0x11A
#define KEY_F11 0x11B
#define KEY_F12 0x11C
#define KEY_F13 0x11D
#define KEY_F14 0x11E

#define KEY_SHIFT_F1 0x121
#define KEY_SHIFT_F2 0x122
#define KEY_SHIFT_F3 0x123
#define KEY_SHIFT_F4 0x124
#define KEY_SHIFT_F5 0x125
#define KEY_SHIFT_F6 0x126
#define KEY_SHIFT_F7 0x127
#define KEY_SHIFT_F8 0x128
#define KEY_SHIFT_F9 0x129
#define KEY_SHIFT_F10 0x12A
#define KEY_SHIFT_F11 0x12B
#define KEY_SHIFT_F12 0x12C
#define KEY_SHIFT_F13 0x12D
#define KEY_SHIFT_F14 0x12E

#define KEY_CTRL_F1 0x131
#define KEY_CTRL_F2 0x132
#define KEY_CTRL_F3 0x133
#define KEY_CTRL_F4 0x134
#define KEY_CTRL_F5 0x135
#define KEY_CTRL_F6 0x136
#define KEY_CTRL_F7 0x137
#define KEY_CTRL_F8 0x138
#define KEY_CTRL_F9 0x139
#define KEY_CTRL_F10 0x13A
#define KEY_CTRL_F11 0x13B
#define KEY_CTRL_F12 0x13C
#define KEY_CTRL_F13 0x13D
#define KEY_CTRL_F14 0x13E

#define KEY_UP 0x141
#define KEY_DOWN 0x142
#define KEY_LEFT 0x143
#define KEY_RIGHT 0x144
#define KEY_HOME 0x145
#define KEY_END 0x146
#define KEY_PAUSE 0x147
#define KEY_FOCUS_IN 0x148
#define KEY_FOCUS_OUT 0x149
#define KEY_INSERT 0x14A
#define KEY_DELETE 0x14B
#define KEY_PGUP   0x14C
#define KEY_PGDN   0x14D
#define KEY_WIN    0x14E
#define KEY_MENU   0x14F

#define KEY_SHIFT_UP 0x151
#define KEY_SHIFT_DOWN 0x152
#define KEY_SHIFT_LEFT 0x153
#define KEY_SHIFT_RIGHT 0x154
#define KEY_SHIFT_HOME 0x155
#define KEY_SHIFT_END 0x156
#define KEY_SHIFT_PAUSE 0x157
#define KEY_SHIFT_FOCUS_IN 0x158
#define KEY_SHIFT_FOCUS_OUT 0x159
#define KEY_SHIFT_INSERT 0x15A
#define KEY_SHIFT_DELETE 0x15B
#define KEY_SHIFT_PGUP   0x15C
#define KEY_SHIFT_PGDN   0x15D
#define KEY_SHIFT_WIN    0x15E
#define KEY_SHIFT_MENU   0x15F

#define KEY_CTRL_UP 0x161
#define KEY_CTRL_DOWN 0x162
#define KEY_CTRL_LEFT 0x163
#define KEY_CTRL_RIGHT 0x164
#define KEY_CTRL_HOME 0x165
#define KEY_CTRL_END 0x166
#define KEY_CTRL_PAUSE 0x167
#define KEY_CTRL_FOCUS_IN 0x168
#define KEY_CTRL_FOCUS_OUT 0x169
#define KEY_CTRL_INSERT 0x16A
#define KEY_CTRL_DELETE 0x16B
#define KEY_CTRL_PGUP   0x16C
#define KEY_CTRL_PGDN   0x16D
#define KEY_CTRL_WIN    0x16E
#define KEY_CTRL_MENU   0x16F


typedef struct
{
    int Flags;
    int ForeColor;
    int BackColor;
    int TextLen;
    char *Text;
		char *MenuPadLeft;
		char *MenuPadRight;
		char *MenuCursorLeft;
		char *MenuCursorRight;
    STREAM *Term;
} TERMBAR;


#define TerminalSetUTF8(level) (UnicodeSetUTF8(level))

//Commands available via the 'terminal command' function. Normally you wouldn't use these, but use the equivalent macros below
typedef enum {TERM_NORM, TERM_TEXT, TERM_COLOR, TERM_CLEAR_SCREEN, TERM_CLEAR_ENDLINE, TERM_CLEAR_STARTLINE, TERM_CLEAR_LINE, TERM_CURSOR_HOME, TERM_CURSOR_MOVE, TERM_CURSOR_SAVE, TERM_CURSOR_UNSAVE, TERM_CURSOR_HIDE, TERM_CURSOR_SHOW, TERM_SCROLL, TERM_SCROLL_REGION, TERM_UNICODE} ETerminalCommands;



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

//put a character. Char can be a value outside the ANSI range which will result in an xterm unicode character string being output
void TerminalPutChar(int Char, STREAM *S);

//'Str' is a format string with with 'tilde commands' in it. The ANSI coded result is copied into RetStr, which is resized as needed and returned
char *TerminalFormatStr(char *RetStr, const char *Str);

//'Str' is a format string with 'tilde commands' in it. The ANSI coded result is output to stream S
void TerminalPutStr(const char *Str, STREAM *S);

//calculate length of string *after ANSI formating* 
int TerminalStrLen(const char *Str);


//Perform various commands on the terminal (normally you'd just use the macros listed above)
int TerminalCommand(int Cmd, int Arg1, int Arg2, STREAM *S);

//read a single character from the terminal. In addition to the normal ANSI characters this returns the KEY_ values listed above
//provided the terminal has been initialized with TERM_RAWKEYS. This allows reading keystrokes for use in games etc
int TerminalReadChar(STREAM *S);


/* 
converts a key to a string. For non-printable key values these strings are the same as the #defined keys above, except without
the leading 'KEY_'. So 'ESC', 'F1' 'SHIFT_F1' 'UP' 'DOWN' etc etc */
const char *TerminalTranslateKeyCode(int key);

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

// These functions create a terminal bar at the bottom of the screen. 
TERMBAR *TerminalBarCreate(STREAM *Term, const char *Config, const char *Text);
void TerminalBarDestroy(TERMBAR *TB);

// Change config of a terminal bar. Options key=value strings. Recognized keys are:
//    MenuPadLeft   //string that menu options should be left padded with
//    MenuPadRight  //string that menu options should be right padded with
//    MenuCursorLeft   //string that SELECTED menu options should be left padded with
//    MenuCursorRight  //string that SELECTED menu options should be right padded with
//    forecolor, backcolor, bold, inverse, underline, blink     //attributes
//    e.g.   forecolor=red backcolor=yellow 'MenuPadLeft=  ' 'MenuPadRight=  ' 'MenuCursorLeft= >' 'MenuCursorRight=< '
void TerminalBarSetConfig(TERMBAR *TB, const char *Config);


//display a terminal bar with 'Text' 
void TerminalBarUpdate(TERMBAR *TB, const char *Text);

//display a prompt in a terminal bar and let the user type in text. Flags are as for 'TerminalReadText'
char *TerminalBarReadText(char *RetStr, TERMBAR *TB, int Flags, const char *Prompt);

//display a menu of optoons in a terminal bar. Options are supplied as a comma-separated
//list in 'ItemStr'. The selected option is copied into 'RetStr' and returned
char *TerminalBarMenu(char *RetStr, TERMBAR *TB, const char *ItemStr);

//prototype for a callback function that recieves a key press
typedef int (*KEY_CALLBACK_FUNC)(STREAM *Term, int Key);

//set a callback function that will be passed all key presses
void TerminalSetKeyCallback(STREAM *Term, KEY_CALLBACK_FUNC Func);



#ifdef __cplusplus
}
#endif



#endif
