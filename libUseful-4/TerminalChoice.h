/* This implements a simple one-line menu that looks like:
 *
 *    proceed Y/n? <yes> no
 *
 *  If you just want to ask a question without other processing going on you can do:
 *
 *  STREAM *Term;
 *  char *Choice=NULL;
 *
 *  Term=STREAMFromDualFD(0,1);
 *  Term=TerminalInit(TERM_RAWKEYS|TERM_SAVEATTRIBS);
 *  Choice=TerminalChoice(Choice, Term, "prompt='make your choice' options='yes,no,maybe'");
 *  printf("User Chose: %s\n", Choice);
 *
 *  If the user presses escape (they may have to do this twice) then NULL is returned (and the string, in this case 'Choice' is freed if it has memory allocated). Otherwise one of the strings given in the comma-separated list 'options' is returned.
 *
 *  The 'Config' parameter of TerminalChoice takes the following entries:
 *
 *  prompt=        A text or prompt that preceeds the menu 
 *  options=       A comma-separated list of menu options
 *  choices=       Equivalent to 'options='
 *  x=             Position the menu at terminal column 'x'. Without x and/or y the menu will be on the current line
 *  y=             Position the menu at terminal row 'y'. Without x and/or y the menu will be on the current line
 *  select-left=   Text to the left of a currently selected item. Defaults to '<'
 *  select-right=  Text to the right of a currently selected item. Defaults to '>'
 *  pad-left=      Text to the left of an unselected item. Defaults to ' '
 *  pad-right=     Text to the right of an unselected item. Defaults to ' '
 *
 *
 *  The 'select' and 'pad' entries can take libUseful tilde-style formatting. If, for instance, you wanted the selected item to be bold-text you'd call:
 *
 *     Choice=TerminalChoice(Choice, Term, "select-left='~e ' select-right=' ~0' pad-left=' ' pad-right=' ' options='yes,no,maybe'");
 *
 *  In programs where blocking while the user makes a choice is not desired, the following approach can be used:
 *

    TERMCHOICE *Chooser;
    int inchar;

    Chooser=TerminalChoiceCreate(Term, Config);
    TerminalChoiceDraw(Chooser);
    inchar=TerminalReadChar(Term);
		while (TRUE)
		{
      RetStr=TerminalChoiceOnKey(RetStr, Chooser, inchar);
      if ((! RetStr) || StrValid(RetStr)) break;

			// do other things
			
      inchar=TerminalReadChar(Term);
		}
    RetStr=TerminalChoiceProcess(RetStr, Chooser);
    TerminalChoiceDestroy(Chooser);


		Normally this would probably be broken up into a part called at program startup that set up the TERMCHOICE object
		then a select/poll loop that did various things, and handed the 'TerminalReadChar' and called 'TerminalChoiceOnKey'
		and finally TerminalChoiceDestroy being called on program exit or whenever

 *
 */


#ifndef LIBUSEFUL_TERMINAL_MENUBAR_H
#define LIBUSEFUL_TERMINAL_MENUBAR_H

#include "includes.h"
#include "Unicode.h"
#include "KeyCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "Terminal.h"
#include "TerminalMenu.h"

#define TERMCHOICE TERMMENU

#define TerminalChoiceCreate TerminalWidgetCreate
#define TerminalChoiceDestroy TerminalWidgetDestroy
#define TerminalChoiceSetOptions TerminalWidgetSetOptions

void TerminalChoiceDraw(TERMCHOICE *MB);
char *TerminalChoiceOnKey(char *RetStr, TERMCHOICE *MB, int key);
char *TerminalChoiceProcess(char *RetStr, TERMCHOICE *MB);
char *TerminalChoice(char *RetStr, STREAM *Term, const char *Config);


#ifdef __cplusplus
}
#endif

#endif
