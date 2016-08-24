
#ifndef LIBUSEFUL_ANSI_H
#define LIBUSEFUL_ANSI_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {ANSI_NONE, ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_YELLOW, ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_WHITE, ANSI_RESET, ANSI_RESET2, ANSI_DARKGREY, ANSI_LIGHTRED, ANSI_LIGHTGREEN, ANSI_LIGHTYELLOW, ANSI_LIGHTBLUE, ANSI_LIGHTMAGENTA, ANSI_LIGHTCYAN, ANSI_LIGHTWHITE} T_ANSI_COLORS;

#define TERM_SHOWTEXT  1
#define TERM_SHOWSTARS 2
#define TERM_SHOWTEXTSTARS 4

#define ANSI_HIDE			65536
#define ANSI_BOLD			131072
#define ANSI_FAINT		262144
#define ANSI_UNDER		524288
#define ANSI_BLINK		1048576
#define ANSI_INVERSE  2097152
#define ANSI_NORM "\x1b[0m"
#define ANSI_BACKSPACE 0x08


char *ANSICode(int Color, int BgColor, int Flags);
int ANSIParseColor(const char *Str);
char *TerminalReadText(char *RetStr, int Flags, STREAM *S);


#ifdef __cplusplus
}
#endif



#endif
