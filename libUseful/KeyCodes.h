#ifndef LIBUSEFUL_KEYCODES_H
#define LIBUSEFUL_KEYCODES_H

#ifdef HAVE_LINUX_INPUT_H
#include <linux/input.h>
#include <linux/input-event-codes.h>
#endif

//these keycodes try not to clash with those in linux/input.h. The linux/evdev system doesn't
//use keycodes for shift and control modified keys, instead it uses a seperate modifier value
//so the shift- or control- values here are given values high enough to put them out of the
//keycode range of linux/input, whereas individual keypress values, like TKEY_F1 are only 
//defined if they are not already defined (probably because linux/input.h isn't present) 


#define KEYMOD_SHIFT 1
#define KEYMOD_CTRL  2
#define KEYMOD_ALT   4

//Keycode definitions
#define ESCAPE 0x1b

#ifndef TKEY_ENTER
#define TKEY_ENTER  '\n'
#endif


#ifndef TKEY_F1
#define TKEY_F1 0x111
#endif

#ifndef TKEY_F2 
#define TKEY_F2 0x112
#endif

#ifndef TKEY_F3
#define TKEY_F3 0x113
#endif

#ifndef TKEY_F4
#define TKEY_F4 0x114
#endif

#ifndef TKEY_F5
#define TKEY_F5 0x115
#endif

#ifndef TKEY_F6
#define TKEY_F6 0x116
#endif

#ifndef TKEY_F7
#define TKEY_F7 0x117
#endif

#ifndef TKEY_F8
#define TKEY_F8 0x118
#endif

#ifndef TKEY_F9
#define TKEY_F9 0x119
#endif

#ifndef TKEY_F10
#define TKEY_F10 0x11A
#endif

#ifndef TKEY_F11
#define TKEY_F11 0x11B
#endif

#ifndef TKEY_F12
#define TKEY_F12 0x11C
#endif

#ifndef TKEY_F13
#define TKEY_F13 0x11D
#endif

#ifndef TKEY_F14
#define TKEY_F14 0x11E
#endif


#ifndef TKEY_UP
#define TKEY_UP 0x141
#endif

#ifndef TKEY_DOWN
#define TKEY_DOWN 0x142
#endif

#ifndef TKEY_LEFT
#define TKEY_LEFT 0x143
#endif

#ifndef TKEY_RIGHT
#define TKEY_RIGHT 0x144
#endif

#ifndef TKEY_HOME
#define TKEY_HOME 0x145
#endif

#ifndef TKEY_END
#define TKEY_END 0x146
#endif

#ifndef TKEY_PAUSE
#define TKEY_PAUSE 0x147
#endif

#ifndef TKEY_PRINT
#define TKEY_PRINT 0x148
#endif

#ifndef TKEY_SCROLL_LOCK
#define TKEY_SCROLL_LOCK 0x149
#endif

#ifndef TKEY_INSERT
#define TKEY_INSERT 0x14A
#endif

#ifndef TKEY_DELETE
#define TKEY_DELETE 0x14B
#endif

#ifndef TKEY_PGUP
#define TKEY_PGUP   0x14C
#endif

#ifndef TKEY_PGDN
#define TKEY_PGDN   0x14D
#endif

#ifndef TKEY_WIN
#define TKEY_WIN    0x14E
#endif

#ifndef TKEY_MENU
#define TKEY_MENU   0x14F
#endif

//these are for situations where we want to treat these as keypresses, not modifiers
#define TKEY_LSHIFT 0x151
#define TKEY_RSHIFT 0x152
#define TKEY_LCNTRL 0x153
#define TKEY_RCNTRL 0x154


//'internet' and 'multimedia' keyboards

#define TKEY_WWW         0x200
#define TKEY_HOME_PAGE   0x201
#define TKEY_BACK        0x202
#define TKEY_FORWARD     0x203
#define TKEY_RELOAD      0x204
#define TKEY_MAIL        0x205
#define TKEY_CALC        0x206
#define TKEY_FAVES       0x207
#define TKEY_SEARCH      0x208
#define TKEY_MYCOMPUTER  0x209
#define TKEY_MESSENGER   0x20A
#define TKEY_LIGHTBULB   0x20B
#define TKEY_SHOP        0x20C
#define TKEY_WLAN        0x20D
#define TKEY_WEBCAM      0x20E
#define TKEY_TERMINAL    0x20F

#define TKEY_MEDIA       0x210
#define TKEY_MEDIA_PAUSE 0x212
#define TKEY_MEDIA_MUTE  0x213
#define TKEY_MEDIA_PREV  0x214
#define TKEY_MEDIA_NEXT  0x215
#define TKEY_MEDIA_STOP  0x216
#define TKEY_EJECT       0x217
#define TKEY_VOL_UP      0x218
#define TKEY_VOL_DOWN    0x219

#define TKEY_STANDBY     0x220
#define TKEY_WAKEUP      0x221
#define TKEY_SLEEP       0x222
#define TKEY_POWER_DOWN  0x223
#define TKEY_OPEN        0x224
#define TKEY_CLOSE       0x225
#define TKEY_COPY        0x226
#define TKEY_CUT         0x227
#define TKEY_CLEAR       0x228
#define TKEY_FOCUS_IN    0x229
#define TKEY_FOCUS_OUT   0x22A
#define TKEY_STOP        0x22B


#define TKEY_SHIFT_BASE 0x1000
#define TKEY_SHIFT_F1  0x1111
#define TKEY_SHIFT_F2  0x1112
#define TKEY_SHIFT_F3  0x1113
#define TKEY_SHIFT_F4  0x1114
#define TKEY_SHIFT_F5  0x1115
#define TKEY_SHIFT_F6  0x1116
#define TKEY_SHIFT_F7  0x1117
#define TKEY_SHIFT_F8  0x1118
#define TKEY_SHIFT_F9  0x1119
#define TKEY_SHIFT_F10 0x111A
#define TKEY_SHIFT_F11 0x111B
#define TKEY_SHIFT_F12 0x111C
#define TKEY_SHIFT_F13 0x111D
#define TKEY_SHIFT_F14 0x111E

#define TKEY_SHIFT_UP    0x1141
#define TKEY_SHIFT_DOWN  0x1142
#define TKEY_SHIFT_LEFT  0x1143
#define TKEY_SHIFT_RIGHT 0x1144
#define TKEY_SHIFT_HOME  0x1145
#define TKEY_SHIFT_END   0x1146
#define TKEY_SHIFT_PAUSE 0x1147
#define TKEY_SHIFT_FOCUS_IN  0x1148
#define TKEY_SHIFT_FOCUS_OUT 0x1149
#define TKEY_SHIFT_INSERT 0x114A
#define TKEY_SHIFT_DELETE 0x114B
#define TKEY_SHIFT_PGUP   0x114C
#define TKEY_SHIFT_PGDN   0x114D
#define TKEY_SHIFT_WIN    0x114E
#define TKEY_SHIFT_MENU   0x114F


#define TKEY_CTRL_BASE 0x2000
#define TKEY_CTRL_F1  0x2111
#define TKEY_CTRL_F2  0x2112
#define TKEY_CTRL_F3  0x2113
#define TKEY_CTRL_F4  0x2114
#define TKEY_CTRL_F5  0x2115
#define TKEY_CTRL_F6  0x2116
#define TKEY_CTRL_F7  0x2117
#define TKEY_CTRL_F8  0x2118
#define TKEY_CTRL_F9  0x2119
#define TKEY_CTRL_F10 0x211A
#define TKEY_CTRL_F11 0x211B
#define TKEY_CTRL_F12 0x211C
#define TKEY_CTRL_F13 0x211D
#define TKEY_CTRL_F14 0x211E

#define TKEY_CTRL_UP    0x2141
#define TKEY_CTRL_DOWN  0x2142
#define TKEY_CTRL_LEFT  0x2143
#define TKEY_CTRL_RIGHT 0x2144
#define TKEY_CTRL_HOME  0x2145
#define TKEY_CTRL_END   0x2146
#define TKEY_CTRL_PAUSE 0x2147
#define TKEY_CTRL_FOCUS_IN  0x2148
#define TKEY_CTRL_FOCUS_OUT 0x2149
#define TKEY_CTRL_INSERT 0x214A
#define TKEY_CTRL_DELETE 0x214B
#define TKEY_CTRL_PGUP   0x214C
#define TKEY_CTRL_PGDN   0x214D
#define TKEY_CTRL_WIN    0x214E
#define TKEY_CTRL_MENU   0x214F


#define TKEY_ALT_BASE 0x3000
#define TKEY_ALT_F1   0x3111
#define TKEY_ALT_F2   0x3112
#define TKEY_ALT_F3   0x3113
#define TKEY_ALT_F4   0x3114
#define TKEY_ALT_F5   0x3115
#define TKEY_ALT_F6   0x3116
#define TKEY_ALT_F7   0x3117
#define TKEY_ALT_F8   0x3118
#define TKEY_ALT_F9   0x3119
#define TKEY_ALT_F10  0x311A
#define TKEY_ALT_F11  0x311B
#define TKEY_ALT_F12  0x311C
#define TKEY_ALT_F13  0x311D
#define TKEY_ALT_F14  0x311E

#define TKEY_ALT_UP    0x3141
#define TKEY_ALT_DOWN  0x3142
#define TKEY_ALT_LEFT  0x3143
#define TKEY_ALT_RIGHT 0x3144
#define TKEY_ALT_HOME  0x3145
#define TKEY_ALT_END   0x3146
#define TKEY_ALT_PAUSE 0x3147
#define TKEY_ALT_FOCUS_IN  0x3148
#define TKEY_ALT_FOCUS_OUT 0x3149
#define TKEY_ALT_INSERT 0x314A
#define TKEY_ALT_DELETE 0x314B
#define TKEY_ALT_PGUP   0x314C
#define TKEY_ALT_PGDN   0x314D
#define TKEY_ALT_WIN    0x314E
#define TKEY_ALT_MENU   0x314F







//not used by libUseful, but assigned so other programs can use
//these key definitions and mouse input without clashing
#define MOUSE_BTN_1 0xFF01
#define MOUSE_BTN_2 0xFF02
#define MOUSE_BTN_3 0xFF03
#define MOUSE_BTN_4 0xFF04 //4 & 5 scrollwheel
#define MOUSE_BTN_5 0xFF05
#define MOUSE_BTN_6 0xFF06 //6 & 7 thumbbuttons
#define MOUSE_BTN_7 0xFF07
#define MOUSE_BTN_8 0xFF08 //from here on in is gaming mice!
#define MOUSE_BTN_9 0xFF09
#define MOUSE_BTN_10 0xFF010
#define MOUSE_BTN_11 0xFF011
#define MOUSE_BTN_12 0xFF012
#define MOUSE_BTN_13 0xFF012
#define MOUSE_BTN_14 0xFF014
#define MOUSE_BTN_15 0xFF015
#define MOUSE_BTN_16 0xFF016
#define MOUSE_BTN_17 0xFF017
#define MOUSE_BTN_18 0xFF018
#define MOUSE_BTN_19 0xFF019
#define MOUSE_BTN_20 0xFF020

#endif
