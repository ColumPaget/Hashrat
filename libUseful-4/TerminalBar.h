#ifndef LIBUSEFUL_TERMINAL_BAR_H
#define LIBUSEFUL_TERMINAL_BAR_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* THIS FEATURE IS STILL BEING DEVELOPED
 *
 * ultimately the plan is to be able to add 'bars' to the top and bottom of the screen that can hold
 * information, menus or text input, with the scrolling region of the terminal being between them
 */


//this will eventually be replaced with
//the 'TerminalWidget' base struct
typedef struct
{
int Flags;
int x, y;
int ForeColor;
int BackColor;
int TextLen;
char *Text;
char *MenuPadLeft;
char *MenuPadRight;
char *MenuCursorLeft;
char *MenuCursorRight;
ListNode *MenuItems;
STREAM *Term;
} TERMBAR;


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


void TerminalBarsInit(STREAM *S);


#ifdef __cplusplus
}
#endif


#endif
