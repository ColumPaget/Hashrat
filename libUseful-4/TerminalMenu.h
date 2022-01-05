#ifndef LIBUSEFUL_TERMINAL_MENU_H
#define LIBUSEFUL_TERMINAL_MENU_H

#include "includes.h"
#include "TerminalWidget.h"

#ifdef __cplusplus
extern "C" {
#endif


//Terminal menu functions give you a selectable 'box' menu of options that's operated by 'up', 'down'
//'enter' to select and 'escape' to back out of it. The menu has an internal list called 'options' to
//which you add named items. The names are the menu options that will be displayed. You can pass NULL
//for the list item, or an object you want associated with the menu option. When an option is selected
//the list node is returned, so you have both the option name, as Node->Tag, and any associated item
//as Node->Item

// Supported keys are arrows and ctrl-WASD:

//UP: up-arrow, ctrl-w
//DOWN: down-arrow, ctrl-s
//SELECT: enter, ctrl-d
//EXIT MENU: escape, ctrl-a


#define TERMMENU_POSITIONED 1
#define TERMMENU_SELECTED LIST_FLAG_USER1

#define TERMMENU TERMWIDGET
#define TerminalMenuDestroy TerminalWidgetDestroy

TERMMENU *TerminalMenuCreate(STREAM *Term, int x, int y, int wid, int high);

//draw the menu in it's current state.
void TerminalMenuDraw(TERMMENU *Menu);

//process key and draw menu in its state after the keypress. Return a choice if
//one is selected, else return NULL. This can be used if you want to handle some
//keypresses outside of the menu, only passing relevant keys to the menu
ListNode *TerminalMenuOnKey(TERMMENU *Menu, int key);

//run a menu after it's been setup. This keeps reading keypresses until an option is
//selected. Returns the selected option, or returns NULL if escape is pressed
ListNode *TerminalMenuProcess(TERMMENU *Menu);

//set menu cursor to top
ListNode *TerminalMenuTop(TERMMENU *Menu);

//move menu cursor up one line
ListNode *TerminalMenuUp(TERMMENU *Menu);

//move menu cursor down one line
ListNode *TerminalMenuDown(TERMMENU *Menu);


//create a menu from a list of options, and run it.
//This keeps reading keypresses until an option is selected. 
//Returns the selected option, or returns NULL if escape is pressed.
//if TERMMENU_SELECTED is set on the head of Options, then the menu will allow setting
//'selected' against multiple values
ListNode *TerminalMenu(STREAM *Term, ListNode *Options, int x, int y, int wid, int high);

//create a menu from a line of text, and run it.
//Options in the text a seperated by '|' so "This|That|The Other".
//This keeps reading keypresses until an option is selected. 
//Returns the selected option as string copied into 'RetStr', or returns NULL if escape is pressed.
//if TERMMENU_SELECTED is set on the head of Options, then the menu will allow setting
//'selected' against multiple values
char *TerminalMenuFromText(char *RetStr, STREAM *Term, const char *Options, int x, int y, int wid, int high);


#ifdef __cplusplus
}
#endif



#endif

