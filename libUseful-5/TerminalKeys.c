#include "TerminalKeys.h"
#include "KeyCodes.h"
#include "Encodings.h"

const char *TerminalTranslateKeyCode(int key)
{
    static char KeyStr[2];
    switch (key)
    {
    case 0:
        return("NUL");
        break;

    case TKEY_TAB:
        return("	");
        break;

    case TKEY_BACKSPACE:
        return("BACKSPACE");
        break;

    case ESCAPE:
        return("ESC");
        break;

    case TKEY_CTRL_A:
        return("CTRL_A");
        break;
    case TKEY_CTRL_B:
        return("CTRL_B");
        break;
    case TKEY_CTRL_C:
        return("CTRL_C");
        break;
    case TKEY_CTRL_D:
        return("CTRL_D");
        break;
    case TKEY_CTRL_E:
        return("CTRL_E");
        break;
    case TKEY_CTRL_F:
        return("CTRL_F");
        break;
    case TKEY_CTRL_G:
        return("CTRL_G");
        break;

    /* NEVER RETURN CTRL_H, as this is backspace!
        case TKEY_CTRL_H:
    	return("CTRL_H");
        break;
    */

    /* NEVER RETURN CTRL_I, as this is tab!
        case TKEY_CTRL_I:
    	return("CTRL_I");
        break;
    */

    /* NEVER RETURN CTRL_J, as this is LINEFEED/ENTER
        case TKEY_CTRL_J:
    	return("CTRL_J");
        break;
    */

    case TKEY_CTRL_K:
        return("CTRL_K");
        break;
    case TKEY_CTRL_L:
        return("CTRL_L");
        break;

    /* NEVER RETURN CTRL-M, as this is carriage return!
        case TKEY_CTRL_M:
    	return("CTRL_M");
        break;
    */
    case TKEY_CTRL_N:
        return("CTRL_N");
        break;
    case TKEY_CTRL_O:
        return("CTRL_O");
        break;
    case TKEY_CTRL_P:
        return("CTRL_P");
        break;
    case TKEY_CTRL_Q:
        return("CTRL_Q");
        break;
    case TKEY_CTRL_R:
        return("CTRL_R");
        break;
    case TKEY_CTRL_S:
        return("CTRL_S");
        break;
    case TKEY_CTRL_T:
        return("CTRL_T");
        break;
    case TKEY_CTRL_U:
        return("CTRL_U");
        break;
    case TKEY_CTRL_V:
        return("CTRL_V");
        break;
    case TKEY_CTRL_W:
        return("CTRL_W");
        break;
    case TKEY_CTRL_X:
        return("CTRL_X");
        break;
    case TKEY_CTRL_Y:
        return("CTRL_Y");
        break;
    case TKEY_CTRL_Z:
        return("CTRL_Z");
        break;

    case TKEY_F1:
        return("F1");
        break;
    case TKEY_F2:
        return("F2");
        break;
    case TKEY_F3:
        return("F3");
        break;
    case TKEY_F4:
        return("F4");
        break;
    case TKEY_F5:
        return("F5");
        break;
    case TKEY_F6:
        return("F6");
        break;
    case TKEY_F7:
        return("F7");
        break;
    case TKEY_F8:
        return("F8");
        break;
    case TKEY_F9:
        return("F9");
        break;
    case TKEY_F10:
        return("F10");
        break;
    case TKEY_F11:
        return("F11");
        break;
    case TKEY_F12:
        return("F12");
        break;
    case TKEY_F13:
        return("F13");
        break;
    case TKEY_F14:
        return("F14");
        break;

    case TKEY_SHIFT_F1:
        return("SHIFT_F1");
        break;
    case TKEY_SHIFT_F2:
        return("SHIFT_F2");
        break;
    case TKEY_SHIFT_F3:
        return("SHIFT_F3");
        break;
    case TKEY_SHIFT_F4:
        return("SHIFT_F4");
        break;
    case TKEY_SHIFT_F5:
        return("SHIFT_F5");
        break;
    case TKEY_SHIFT_F6:
        return("SHIFT_F6");
        break;
    case TKEY_SHIFT_F7:
        return("SHIFT_F7");
        break;
    case TKEY_SHIFT_F8:
        return("SHIFT_F8");
        break;
    case TKEY_SHIFT_F9:
        return("SHIFT_F9");
        break;
    case TKEY_SHIFT_F10:
        return("SHIFT_F10");
        break;
    case TKEY_SHIFT_F11:
        return("SHIFT_F11");
        break;
    case TKEY_SHIFT_F12:
        return("SHIFT_F12");
        break;
    case TKEY_SHIFT_F13:
        return("SHIFT_F13");
        break;
    case TKEY_SHIFT_F14:
        return("SHIFT_F14");
        break;

    case TKEY_CTRL_F1:
        return("CTRL_F1");
        break;
    case TKEY_CTRL_F2:
        return("CTRL_F2");
        break;
    case TKEY_CTRL_F3:
        return("CTRL_F3");
        break;
    case TKEY_CTRL_F4:
        return("CTRL_F4");
        break;
    case TKEY_CTRL_F5:
        return("CTRL_F5");
        break;
    case TKEY_CTRL_F6:
        return("CTRL_F6");
        break;
    case TKEY_CTRL_F7:
        return("CTRL_F7");
        break;
    case TKEY_CTRL_F8:
        return("CTRL_F8");
        break;
    case TKEY_CTRL_F9:
        return("CTRL_F9");
        break;
    case TKEY_CTRL_F10:
        return("CTRL_F10");
        break;
    case TKEY_CTRL_F11:
        return("CTRL_F11");
        break;
    case TKEY_CTRL_F12:
        return("CTRL_F12");
        break;
    case TKEY_CTRL_F13:
        return("CTRL_F13");
        break;
    case TKEY_CTRL_F14:
        return("CTRL_F14");
        break;

    case TKEY_UP:
        return("UP");
        break;
    case TKEY_DOWN:
        return("DOWN");
        break;
    case TKEY_LEFT:
        return("LEFT");
        break;
    case TKEY_RIGHT:
        return("RIGHT");
        break;
    case TKEY_HOME:
        return("HOME");
        break;
    case TKEY_END:
        return("END");
        break;

    case TKEY_PAUSE:
        return("PAUSE");
        break;

    case TKEY_PRINT:
        return("PRINT");
        break;

    case TKEY_SCROLL_LOCK:
        return("SCROLL_LOCK");
        break;


    case XTERM_FOCUS_IN:
        return("FOCUS_IN");
        break;
    case XTERM_FOCUS_OUT:
        return("FOCUS_OUT");
        break;
    case TKEY_INSERT:
        return("INSERT");
        break;
    case TKEY_DELETE:
        return("DELETE");
        break;
    case TKEY_PGUP:
        return("PGUP");
        break;
    case TKEY_PGDN:
        return("PGDN");
        break;
    case TKEY_WIN:
        return("WIN");
        break;
    case TKEY_MENU:
        return("MENU");
        break;

    case TKEY_WWW:
        return("WWW");
        break;

    case TKEY_HOME_PAGE:
        return("HOMEPAGE");
        break;

    case TKEY_FAVES:
        return("FAVES");
        break;

    case TKEY_SEARCH:
        return("SEARCH");
        break;

    case TKEY_BACK:
        return("BACK");
        break;

    case TKEY_FORWARD:
        return("FORWARD");
        break;

    case TKEY_MAIL:
        return("MAIL");
        break;

    case TKEY_MESSENGER:
        return("MESSENGER");
        break;

    case TKEY_MEDIA:
        return("MEDIA");
        break;

    case TKEY_MEDIA_MUTE:
        return("MUTE");
        break;

    case TKEY_MYCOMPUTER:
        return("MYCOMPUTER");
        break;

    case TKEY_CALC:
        return("CALCULATOR");
        break;

    case TKEY_SHIFT_UP:
        return("SHIFT_UP");
        break;
    case TKEY_SHIFT_DOWN:
        return("SHIFT_DOWN");
        break;
    case TKEY_SHIFT_LEFT:
        return("SHIFT_LEFT");
        break;
    case TKEY_SHIFT_RIGHT:
        return("SHIFT_RIGHT");
        break;
    case TKEY_SHIFT_HOME:
        return("SHIFT_HOME");
        break;
    case TKEY_SHIFT_END:
        return("SHIFT_END");
        break;
    case TKEY_SHIFT_PAUSE:
        return("SHIFT_PAUSE");
        break;
    case TKEY_SHIFT_INSERT:
        return("SHIFT_INSERT");
        break;
    case TKEY_SHIFT_DELETE:
        return("SHIFT_DELETE");
        break;
    case TKEY_SHIFT_PGUP:
        return("SHIFT_PGUP");
        break;
    case TKEY_SHIFT_PGDN:
        return("SHIFT_PGDN");
        break;
    case TKEY_SHIFT_WIN:
        return("SHIFT_WIN");
        break;
    case TKEY_SHIFT_MENU:
        return("SHIFT_MENU");
        break;

    case TKEY_CTRL_UP:
        return("CTRL_UP");
        break;
    case TKEY_CTRL_DOWN:
        return("CTRL_DOWN");
        break;
    case TKEY_CTRL_LEFT:
        return("CTRL_LEFT");
        break;
    case TKEY_CTRL_RIGHT:
        return("CTRL_RIGHT");
        break;
    case TKEY_CTRL_HOME:
        return("CTRL_HOME");
        break;
    case TKEY_CTRL_END:
        return("CTRL_END");
        break;
    case TKEY_CTRL_PAUSE:
        return("CTRL_PAUSE");
        break;
    case TKEY_CTRL_INSERT:
        return("CTRL_INSERT");
        break;
    case TKEY_CTRL_DELETE:
        return("CTRL_DELETE");
        break;
    case TKEY_CTRL_PGUP:
        return("CTRL_PGUP");
        break;
    case TKEY_CTRL_PGDN:
        return("CTRL_PGDN");
        break;
    case TKEY_CTRL_WIN:
        return("CTRL_WIN");
        break;
    case TKEY_CTRL_MENU:
        return("CTRL_MENU");
        break;

    case TKEY_ALT_UP:
        return("ALT_UP");
        break;
    case TKEY_ALT_DOWN:
        return("ALT_DOWN");
        break;
    case TKEY_ALT_LEFT:
        return("ALT_LEFT");
        break;
    case TKEY_ALT_RIGHT:
        return("ALT_RIGHT");
        break;
    case TKEY_ALT_HOME:
        return("ALT_HOME");
        break;
    case TKEY_ALT_END:
        return("ALT_END");
        break;
    case TKEY_ALT_PAUSE:
        return("ALT_PAUSE");
        break;
    case TKEY_ALT_INSERT:
        return("ALT_INSERT");
        break;
    case TKEY_ALT_DELETE:
        return("ALT_DELETE");
        break;
    case TKEY_ALT_PGUP:
        return("ALT_PGUP");
        break;
    case TKEY_ALT_PGDN:
        return("ALT_PGDN");
        break;
    case TKEY_ALT_WIN:
        return("ALT_WIN");
        break;
    case TKEY_ALT_MENU:
        return("ALT_MENU");
        break;

    case TKEY_ALT_F1:
        return("ALT_F1");
        break;

    case TKEY_ALT_F2:
        return("ALT_F2");
        break;

    case TKEY_ALT_F3:
        return("ALT_F3");
        break;

    case TKEY_ALT_F4:
        return("ALT_F4");
        break;

    case TKEY_ALT_F5:
        return("ALT_F5");
        break;

    case TKEY_ALT_F6:
        return("ALT_F6");
        break;

    case TKEY_ALT_F7:
        return("ALT_F7");
        break;

    case TKEY_ALT_F8:
        return("ALT_F8");
        break;

    case TKEY_ALT_F9:
        return("ALT_F9");
        break;

    case TKEY_ALT_F10:
        return("ALT_F10");
        break;

    case TKEY_ALT_F11:
        return("ALT_F11");
        break;

    case TKEY_ALT_F12:
        return("ALT_F12");
        break;

    case MOUSE_BTN_0:
        return("MOUSE0");
        break;

    case MOUSE_BTN_1:
        return("MOUSE1");
        break;

    case MOUSE_BTN_2:
        return("MOUSE2");
        break;

    case MOUSE_BTN_3:
        return("MOUSE3");
        break;

    case MOUSE_BTN_4:
        return("MOUSE4");
        break;

    case MOUSE_BTN_5:
        return("MOUSE5");
        break;
    }

    KeyStr[0]='?';
    KeyStr[1]='\0';
    if ((key > 31) && (key < 127)) KeyStr[0]=key & 0xFF;
    else if ((key=='\n') || (key=='\r')) KeyStr[0]=key & 0xFF;
    return(KeyStr);
}




int TerminalTranslateKeyStrWithMod(const char *str, int *mod)
{
    if (mod != NULL) *mod=0;

//read as many modifiers as are found, then break
    while (1)
    {
        if ( (strncasecmp(str, "shift-", 6)==0) || (strncasecmp(str, "shift_", 6)==0))
        {
            if (mod !=NULL) *mod |= KEYMOD_SHIFT;
            str+=6;
        }
        else if ((strncasecmp(str, "ctrl-", 5)==0) || (strncasecmp(str, "ctrl_", 5)==0))
        {
            if (mod !=NULL) *mod |= KEYMOD_CTRL;
            str+=5;
        }
        else if ((strncasecmp(str, "alt-", 4)==0) || (strncasecmp(str, "alt_", 4)==0))
        {
            if (mod !=NULL) *mod |= KEYMOD_ALT;
            str+=4;
        }
        else if ((strncasecmp(str, "lalt-", 5)==0) || (strncasecmp(str, "lalt_", 5)==0))
        {
            if (mod !=NULL) *mod |= KEYMOD_ALT;
            str+=5;
        }
        else if ((strncasecmp(str, "ralt-", 5)==0) || (strncasecmp(str, "ralt_", 5)==0))
        {
            if (mod !=NULL) *mod |= KEYMOD_ALT2;
            str+=5;
        }
        else if ((strncasecmp(str, "super-", 6)==0) || (strncasecmp(str, "super_", 6)==0))
        {
            if (mod !=NULL) *mod |= KEYMOD_SUPER;
            str+=6;
        }

        else break;
    }


    switch (*str)
    {
    case 'b':
    case 'B':
        if (strcasecmp(str, "back")==0) return(TKEY_BACK);
        break;

    case 'c':
    case 'C':
        if (strcasecmp(str, "ctrl")==0) return(TKEY_LCNTRL);
        if (strcasecmp(str, "cntrl")==0) return(TKEY_LCNTRL);
        if (strcasecmp(str, "calc")==0) return(TKEY_CALC);
        if (strcasecmp(str, "calculator")==0) return(TKEY_CALC);
        if (strcasecmp(str, "copy")==0) return(TKEY_COPY);
        if (strcasecmp(str, "cut")==0) return(TKEY_CUT);
        if (strcasecmp(str, "clear")==0) return(TKEY_CLEAR);
        if (strcasecmp(str, "computer")==0) return(TKEY_MYCOMPUTER);
        if (strcasecmp(str, "caps")==0) return(TKEY_CAPS_LOCK);
        if (strcasecmp(str, "capslock")==0) return(TKEY_CAPS_LOCK);
        break;

    case 'd':
    case 'D':
        if (strcasecmp(str, "DOWN")==0) return(TKEY_DOWN);
        if (strcasecmp(str, "delete")==0) return(TKEY_DELETE);
        if (strcasecmp(str, "del")==0) return(TKEY_DELETE);
        break;

    case 'e':
    case 'E':
        if (strcasecmp(str, "end")==0) return(TKEY_END);
        if (strcasecmp(str, "ESC")==0) return(ESCAPE);
        if (strcasecmp(str, "ENTER")==0) return(TKEY_ENTER);
        if (strcasecmp(str, "eject")==0) return(TKEY_EJECT);
        break;


    case 'f':
    case 'F':
        if (strcasecmp(str, "faves")==0) return(TKEY_FAVES);
        if (strcasecmp(str, "forward")==0) return(TKEY_FORWARD);
        if (strcasecmp(str, "F1")==0) return(TKEY_F1);
        if (strcasecmp(str, "F2")==0) return(TKEY_F2);
        if (strcasecmp(str, "F3")==0) return(TKEY_F3);
        if (strcasecmp(str, "F4")==0) return(TKEY_F4);
        if (strcasecmp(str, "F5")==0) return(TKEY_F5);
        if (strcasecmp(str, "F6")==0) return(TKEY_F6);
        if (strcasecmp(str, "F7")==0) return(TKEY_F7);
        if (strcasecmp(str, "F8")==0) return(TKEY_F8);
        if (strcasecmp(str, "F9")==0) return(TKEY_F9);
        if (strcasecmp(str, "F10")==0) return(TKEY_F10);
        if (strcasecmp(str, "F11")==0) return(TKEY_F11);
        if (strcasecmp(str, "F12")==0) return(TKEY_F12);
        if (strcasecmp(str, "F13")==0) return(TKEY_F13);
        if (strcasecmp(str, "F14")==0) return(TKEY_F14);
        break;

    case 'h':
    case 'H':
        if (strcasecmp(str, "home")==0) return(TKEY_HOME);
        if (strcasecmp(str, "homepage")==0) return(TKEY_HOME_PAGE);
        break;

    case 'i':
    case 'I':
        if (strcasecmp(str, "insert")==0) return(TKEY_INSERT);
        if (strcasecmp(str, "ins")==0) return(TKEY_INSERT);
        break;

    case 'm':
    case 'M':
        if (strcasecmp(str, "mail")==0) return(TKEY_MAIL);
        if (strcasecmp(str, "messenger")==0) return(TKEY_MESSENGER);
        if (strcasecmp(str, "media")==0) return(TKEY_MEDIA);
        if (strcasecmp(str, "menu")==0) return(TKEY_MENU);
        if (strcasecmp(str, "mute")==0) return(TKEY_MEDIA_MUTE);
        if (strcasecmp(str, "mycomp")==0) return(TKEY_MYCOMPUTER);
        if (strcasecmp(str, "mycomputer")==0) return(TKEY_MYCOMPUTER);
        break;

    case 'l':
    case 'L':
        if (strcasecmp(str, "LEFT")==0) return(TKEY_LEFT);
        if (strcasecmp(str, "lshift")==0) return(TKEY_LSHIFT);
        if (strcasecmp(str, "lctrl")==0) return(TKEY_LCNTRL);
        if (strcasecmp(str, "lcntrl")==0) return(TKEY_LCNTRL);
        if (strcasecmp(str, "lightbulb")==0) return(TKEY_LIGHTBULB);
        break;

    case 'o':
    case 'O':
        if (strcasecmp(str, "open")==0) return(TKEY_OPEN);
        break;

    case 'p':
    case 'P':
        if (strcasecmp(str, "pgup")==0) return(TKEY_PGUP);
        if (strcasecmp(str, "pgdn")==0) return(TKEY_PGDN);
        if (strcasecmp(str, "pause")==0) return(TKEY_PAUSE);
        if (strcasecmp(str, "print")==0) return(TKEY_PRINT);
        if (strcasecmp(str, "play")==0) return(TKEY_MEDIA_PAUSE);
        break;


    case 'r':
    case 'R':
        if (strcasecmp(str, "RIGHT")==0) return(TKEY_RIGHT);
        if (strcasecmp(str, "rshift")==0) return(TKEY_RSHIFT);
        if (strcasecmp(str, "rctrl")==0) return(TKEY_RCNTRL);
        if (strcasecmp(str, "rcntrl")==0) return(TKEY_RCNTRL);
        break;


    case 's':
    case 'S':
        if (strcasecmp(str, "space")==0) return(' ');
        if (strcasecmp(str, "search")==0) return(TKEY_SEARCH);
        if (strcasecmp(str, "standby")==0) return(TKEY_STANDBY);
        if (strcasecmp(str, "sleep")==0) return(TKEY_SLEEP);
        if (strcasecmp(str, "shop")==0) return(TKEY_SHOP);
        if (strcasecmp(str, "stop")==0) return(TKEY_STOP);
        if (strcasecmp(str, "scrlck")==0) return(TKEY_SCROLL_LOCK);
        if (strcasecmp(str, "scroll-lock")==0) return(TKEY_SCROLL_LOCK);
        if (strcasecmp(str, "shift")==0) return(TKEY_LSHIFT);
        break;

    case 't':
    case 'T':
        if (strcasecmp(str, "term")==0) return(TKEY_TERMINAL);
        if (strcasecmp(str, "terminal")==0) return(TKEY_TERMINAL);
        break;

    case 'u':
    case 'U':
        if (strcasecmp(str, "UP")==0) return(TKEY_UP);
        break;

    case 'v':
    case 'V':
        if (strcasecmp(str, "volup")==0) return(TKEY_VOL_UP);
        if (strcasecmp(str, "voldn")==0) return(TKEY_VOL_DOWN);
        if (strcasecmp(str, "voldown")==0) return(TKEY_VOL_DOWN);
        break;

    case 'w':
    case 'W':
        if (strcasecmp(str, "win")==0) return(TKEY_WIN);
        if (strcasecmp(str, "www")==0) return(TKEY_WWW);
        if (strcasecmp(str, "wlan")==0) return(TKEY_WLAN);
        if (strcasecmp(str, "webcam")==0) return(TKEY_WEBCAM);
        break;

    }

    return((int) *str);
}

int TerminalTranslateKeyStr(const char *str)
{
    int key, mod;

    key=TerminalTranslateKeyStrWithMod(str, &mod);

    //if control is pressed, and the key is an alphabetic character, then
    //convert it to a control char
    if ((mod & KEYMOD_CTRL) && (*str > 65) && (*str < 122))
    {
        mod &= ~KEYMOD_CTRL; //do not add a mod for these, as the character code alone implies it
        return((tolower(*str) - 'a') +1);
    }

    if (mod==KEYMOD_SHIFT) key+=TKEY_SHIFT_BASE;
    if (mod==KEYMOD_CTRL) key+=TKEY_CTRL_BASE;
    if (mod==KEYMOD_ALT) key+=TKEY_ALT_BASE;

    return(key);
}


//Getting desperate on what to name these, this one deals
//with a sequence that goes ESC[;1
static int TerminalReadCSISeqSemicolon1(STREAM *S)
{
    int inchar;

    inchar=STREAMReadChar(S);
    switch (inchar)
    {
    case '2':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case 'A':
            return(TKEY_SHIFT_UP);
            break;
        case 'B':
            return(TKEY_SHIFT_DOWN);
            break;
        case 'C':
            return(TKEY_SHIFT_RIGHT);
            break;
        case 'D':
            return(TKEY_SHIFT_LEFT);
            break;
        case 'F':
            return(TKEY_SHIFT_END);
            break;
        case 'H':
            return(TKEY_SHIFT_HOME);
            break;
        case 'P':
            return(TKEY_SHIFT_F1);
            break;
        case 'Q':
            return(TKEY_SHIFT_F2);
            break;
        case 'R':
            return(TKEY_SHIFT_F3);
            break;
        case 'S':
            return(TKEY_SHIFT_F4);
            break;
        }
        break;

    case '3':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case 'A':
            return(TKEY_ALT_UP);
            break;
        case 'B':
            return(TKEY_ALT_DOWN);
            break;
        case 'C':
            return(TKEY_ALT_RIGHT);
            break;
        case 'D':
            return(TKEY_ALT_LEFT);
            break;
        case 'F':
            return(TKEY_ALT_END);
            break;
        case 'H':
            return(TKEY_ALT_HOME);
            break;

        case 'P':
            return(TKEY_ALT_F1);
            break;
        case 'Q':
            return(TKEY_ALT_F2);
            break;
        case 'R':
            return(TKEY_ALT_F3);
            break;
        case 'S':
            return(TKEY_ALT_F4);
            break;
        }
        break;


    case '5':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case 'A':
            return(TKEY_CTRL_UP);
            break;
        case 'B':
            return(TKEY_CTRL_DOWN);
            break;
        case 'C':
            return(TKEY_CTRL_RIGHT);
            break;
        case 'D':
            return(TKEY_CTRL_LEFT);
            break;
        case 'F':
            return(TKEY_CTRL_END);
            break;
        case 'H':
            return(TKEY_CTRL_HOME);
            break;

        case 'P':
            return(TKEY_CTRL_F1);
            break;
        case 'Q':
            return(TKEY_CTRL_F2);
            break;
        case 'R':
            return(TKEY_CTRL_F3);
            break;
        case 'S':
            return(TKEY_CTRL_F4);
            break;
        }
        break;
    }

    return(0);
}



//Getting desperate on what to name these, this one deals
//with a sequence that goes ESC[;2
static int TerminalReadCSISeqSemicolon2(STREAM *S)
{
    int inchar;

    inchar=STREAMReadChar(S);
    switch (inchar)
    {
    case '2':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case 'A':
            return(TKEY_SHIFT_UP);
            break;
        case 'B':
            return(TKEY_SHIFT_DOWN);
            break;
        case 'C':
            return(TKEY_SHIFT_RIGHT);
            break;
        case 'D':
            return(TKEY_SHIFT_LEFT);
            break;
        case 'F':
            return(TKEY_SHIFT_END);
            break;
        case 'H':
            return(TKEY_SHIFT_HOME);
            break;
        }
        break;

    case '3':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case 'A':
            return(TKEY_ALT_UP);
            break;
        case 'B':
            return(TKEY_ALT_DOWN);
            break;
        case 'C':
            return(TKEY_ALT_RIGHT);
            break;
        case 'D':
            return(TKEY_ALT_LEFT);
            break;
        case 'F':
            return(TKEY_ALT_END);
            break;
        case 'H':
            return(TKEY_ALT_HOME);
            break;
        }
        break;


    case '5':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case 'A':
            return(TKEY_CTRL_UP);
            break;
        case 'B':
            return(TKEY_CTRL_DOWN);
            break;
        case 'C':
            return(TKEY_CTRL_RIGHT);
            break;
        case 'D':
            return(TKEY_CTRL_LEFT);
            break;
        case 'F':
            return(TKEY_CTRL_END);
            break;
        case 'H':
            return(TKEY_CTRL_HOME);
            break;

        case 'P':
            return(TKEY_CTRL_F1);
            break;
        case 'Q':
            return(TKEY_CTRL_F2);
            break;
        case 'R':
            return(TKEY_CTRL_F3);
            break;
        case 'S':
            return(TKEY_CTRL_F4);
            break;
        }
        break;
    }

    return(0);
}



static int TerminalReadCSISeqSemicolon(STREAM *S, int val)
{
    int inchar;

    inchar=STREAMReadChar(S);
    switch (inchar)
    {
    case '2':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case '~':
            switch(val)
            {
            case 5:
                return(TKEY_SHIFT_PGUP);
                break;
            case 6:
                return(TKEY_SHIFT_PGDN);
                break;
            case 15:
                return(TKEY_SHIFT_F5);
                break;
            case 17:
                return(TKEY_SHIFT_F6);
                break;
            case 18:
                return(TKEY_SHIFT_F7);
                break;
            case 19:
                return(TKEY_SHIFT_F8);
                break;
            case 20:
                return(TKEY_SHIFT_F9);
                break;
            case 21:
                return(TKEY_SHIFT_F10);
                break;
            case 23:
                return(TKEY_SHIFT_F11);
                break;
            case 24:
                return(TKEY_SHIFT_F12);
                break;
            }
            break;
        }
        break;

    case '3':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case '~':
            switch(val)
            {
            case 5:
                return(TKEY_ALT_PGUP);
                break;
            case 6:
                return(TKEY_ALT_PGDN);
                break;
            case 15:
                return(TKEY_ALT_F5);
                break;
            case 17:
                return(TKEY_ALT_F6);
                break;
            case 18:
                return(TKEY_ALT_F7);
                break;
            case 19:
                return(TKEY_ALT_F8);
                break;
            case 20:
                return(TKEY_ALT_F9);
                break;
            case 21:
                return(TKEY_ALT_F10);
                break;
            case 23:
                return(TKEY_ALT_F11);
                break;
            case 24:
                return(TKEY_ALT_F12);
                break;
            }
            break;
        }
        break;

    case '5':
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case '~':
            switch(val)
            {
            case 5:
                return(TKEY_CTRL_PGUP);
                break;
            case 6:
                return(TKEY_CTRL_PGDN);
                break;
            case 15:
                return(TKEY_CTRL_F5);
                break;
            case 17:
                return(TKEY_CTRL_F6);
                break;
            case 18:
                return(TKEY_CTRL_F7);
                break;
            case 19:
                return(TKEY_CTRL_F8);
                break;
            case 20:
                return(TKEY_CTRL_F9);
                break;
            case 21:
                return(TKEY_CTRL_F10);
                break;
            case 23:
                return(TKEY_CTRL_F11);
                break;
            case 24:
                return(TKEY_CTRL_F12);
                break;
            }
            break;
        }
        break;


    case '~':
        switch (val)
        {
        case 5:
            return(TKEY_CTRL_PGUP);
            break;
        case 6:
            return(TKEY_CTRL_PGDN);
            break;
        case 15:
            return(TKEY_CTRL_F5);
            break;
        case 17:
            return(TKEY_CTRL_F6);
            break;
        case 18:
            return(TKEY_CTRL_F7);
            break;
        case 19:
            return(TKEY_CTRL_F9);
            break;
        case 20:
            return(TKEY_CTRL_F10);
            break;
        case 21:
            return(TKEY_CTRL_F11);
            break;
        case 22:
            return(TKEY_CTRL_F12);
            break;
        }
        break;
    }

    return(0);
}


// deals with sequences of the form <esc>[n where 'n' is a number
static int TerminalReadCSISeqNum(STREAM *S, char PrevChar)
{
    char *Tempstr=NULL;
    int inchar, val;

    Tempstr=AddCharToStr(Tempstr, PrevChar);
    inchar=STREAMReadChar(S);
    if (isdigit(inchar))
    {
        Tempstr=AddCharToStr(Tempstr, inchar);
        inchar=STREAMReadChar(S);
    }

    val=atoi(Tempstr);
    switch (inchar)
    {
    //no modifier
    case '~':
        switch (val)
        {
        case 1:
            return(TKEY_HOME);
            break;
        case 2:
            return(TKEY_INSERT);
            break;
        case 3:
            return(TKEY_DELETE);
            break;
        case 4:
            return(TKEY_END);
            break;
        case 5:
            return(TKEY_PGUP);
            break;
        case 6:
            return(TKEY_PGDN);
            break;

        case 11:
            return(TKEY_F1);
            break;
        case 12:
            return(TKEY_F2);
            break;
        case 13:
            return(TKEY_F3);
            break;
        case 14:
            return(TKEY_F4);
            break;
        case 15:
            return(TKEY_F5);
            break;
        case 16:
            return(TKEY_F6);
            break;
        case 17:
            return(TKEY_F6);
            break;
        case 18:
            return(TKEY_F7);
            break;
        case 19:
            return(TKEY_F8);
            break;
        case 20:
            return(TKEY_F9);
            break;
        case 21:
            return(TKEY_F10);
            break;
        case 23:
            return(TKEY_F11);
            break;
        case 24:
            return(TKEY_F12);
            break;
        case 25:
            return(TKEY_F13);
            break;
        case 26:
            return(TKEY_F14);
            break;
        case 28:
            return(TKEY_WIN);
            break;
        case 29:
            return(TKEY_MENU);
            break;
        }
        break;


    //shift
    case '$':
        switch (val)
        {
        case 1:
            return(TKEY_SHIFT_HOME);
            break;
        case 2:
            return(TKEY_SHIFT_INSERT);
            break;
        case 3:
            return(TKEY_SHIFT_DELETE);
            break;
        case 4:
            return(TKEY_SHIFT_END);
            break;
        case 5:
            return(TKEY_SHIFT_PGUP);
            break;
        case 6:
            return(TKEY_SHIFT_PGDN);
            break;

        case 11:
            return(TKEY_SHIFT_F1);
            break;
        case 12:
            return(TKEY_SHIFT_F2);
            break;
        case 13:
            return(TKEY_SHIFT_F3);
            break;
        case 14:
            return(TKEY_SHIFT_F4);
            break;
        case 15:
            return(TKEY_SHIFT_F5);
            break;
        case 16:
            return(TKEY_SHIFT_F6);
            break;
        case 17:
            return(TKEY_SHIFT_F6);
            break;
        case 18:
            return(TKEY_SHIFT_F7);
            break;
        case 19:
            return(TKEY_SHIFT_F8);
            break;
        case 20:
            return(TKEY_SHIFT_F9);
            break;
        case 21:
            return(TKEY_SHIFT_F10);
            break;
        case 23:
            return(TKEY_SHIFT_F11);
            break;
        case 24:
            return(TKEY_SHIFT_F12);
            break;

        case 25:
            return(TKEY_SHIFT_F13);
            break;
        case 26:
            return(TKEY_SHIFT_F14);
            break;
        case 28:
            return(TKEY_SHIFT_WIN);
            break;
        case 29:
            return(TKEY_SHIFT_MENU);
            break;
        }
        break;

    //ctrl
    case '^':
        switch (val)
        {
        case 1:
            return(TKEY_CTRL_HOME);
            break;
        case 2:
            return(TKEY_CTRL_INSERT);
            break;
        case 3:
            return(TKEY_CTRL_DELETE);
            break;
        case 4:
            return(TKEY_CTRL_END);
            break;
        case 5:
            return(TKEY_CTRL_PGUP);
            break;
        case 6:
            return(TKEY_CTRL_PGDN);
            break;
        case 11:
            return(TKEY_CTRL_F1);
            break;
        case 12:
            return(TKEY_CTRL_F2);
            break;
        case 13:
            return(TKEY_CTRL_F3);
            break;
        case 14:
            return(TKEY_CTRL_F4);
            break;
        case 15:
            return(TKEY_CTRL_F5);
            break;
        case 16:
            return(TKEY_CTRL_F6);
            break;
        case 17:
            return(TKEY_CTRL_F6);
            break;
        case 18:
            return(TKEY_CTRL_F7);
            break;
        case 19:
            return(TKEY_CTRL_F8);
            break;
        case 20:
            return(TKEY_CTRL_F9);
            break;
        case 21:
            return(TKEY_CTRL_F10);
            break;
        case 23:
            return(TKEY_CTRL_F11);
            break;
        case 24:
            return(TKEY_CTRL_F12);
            break;

        case 25:
            return(TKEY_CTRL_F13);
            break;
        case 26:
            return(TKEY_CTRL_F14);
            break;
        case 28:
            return(TKEY_CTRL_WIN);
            break;
        case 29:
            return(TKEY_CTRL_MENU);
            break;
        }
        break;

    case ';':
        switch (val)
        {
        case 1:
            return(TerminalReadCSISeqSemicolon1(S));
            break;
        case 2:
            return(TerminalReadCSISeqSemicolon2(S));
            break;
        default:
            return(TerminalReadCSISeqSemicolon(S, val));
            break;
        }
        break;

    }

    Destroy(Tempstr);

    return(0);
}


// deals with sequences of the form <esc>[M which relate to mouse movement
static int TerminalReadCSIMouse(STREAM *S)
{
    char *Tempstr=NULL;
    int flags, x, y, val, keycode=0;

    flags=STREAMReadChar(S)-32;
    x=STREAMReadChar(S)-32;
    y=STREAMReadChar(S)-32;

    switch (flags)
    {
    case 0:
        keycode=MOUSE_BTN_1;
        break;
    case 1:
        keycode=MOUSE_BTN_2;
        break;
    case 2:
        keycode=MOUSE_BTN_3;
        break;
    case 3:
        keycode=MOUSE_BTN_0;
        break;
    case 64:
        keycode=MOUSE_BTN_4;
        break;
    case 65:
        keycode=MOUSE_BTN_5;
        break;
    }

    Tempstr=FormatStr(Tempstr, "%d:%d:%d", keycode, x, y);
    STREAMSetValue(S, "LU_MouseEvent", Tempstr);

    Destroy(Tempstr);

    return(keycode);
}



// sequences that begin with '<esc>[' or the 'Control Sequence Indicator (CSI)'
int TerminalReadCSISeq(STREAM *S)
{
    int inchar;

    inchar=STREAMReadChar(S);
    switch (inchar)
    {
    case 'a':
        return(TKEY_SHIFT_UP);
        break;
    case 'b':
        return(TKEY_SHIFT_DOWN);
        break;
    case 'c':
        return(TKEY_SHIFT_RIGHT);
        break;
    case 'd':
        return(TKEY_SHIFT_LEFT);
        break;

    case 'A':
        return(TKEY_UP);
        break;
    case 'B':
        return(TKEY_DOWN);
        break;
    case 'C':
        return(TKEY_RIGHT);
        break;
    case 'D':
        return(TKEY_LEFT);
        break;
    case 'F':
        return(TKEY_END);
        break;
    case 'H':
        return(TKEY_HOME);
        break;
    case 'M':
        return(TerminalReadCSIMouse(S));
        break;
    case 'P':
        return(TKEY_PAUSE);
        break;
    case 'Z':
        return(TKEY_END);
        break;
    case 'I':
        return(XTERM_FOCUS_IN);
        break;
    case 'O':
        return(XTERM_FOCUS_OUT);
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return(TerminalReadCSISeqNum(S, inchar));
        break;
    }

    return(inchar);
}


// sequences that begin with '<esc>]' or the 'Operating System Command (OSC)'
int TerminalReadOSCSeq(STREAM *S)
{
    int inchar, Key=0;
    char *Str=NULL, *Type=NULL, *Data=NULL;
    const char *ptr;

    inchar=STREAMReadChar(S);
    switch (inchar)
    {
    case '5':
        inchar=STREAMReadChar(S);
        if (inchar=='2')
        {
            inchar=STREAMReadChar(S);
            if (inchar == ';')
            {
                Str=STREAMReadToTerminator(Str, S, '\007');
                ptr=GetToken(Str, ";", &Type, 0);
                ptr=GetToken(ptr, "\007", &Data, 0);
                Str=DecodeToText(Str, Data, ENCODE_BASE64);
                if (*Type == 'c')
                {
                    STREAMSetValue(S, "LU_XTERM_CLIPBOARD", Str);
                    Key=XTERM_CLIPBOARD;
                }
                else if (*Type == 'p')
                {
                    STREAMSetValue(S, "LU_XTERM_SELECTION", Str);
                    Key=XTERM_SELECTION;
                }
            }
        }
        break;

    }

    Destroy(Str);
    Destroy(Type);
    Destroy(Data);

    return(Key);
}

// sequences that begin with '<esc>O'. This is 'single shift character' and is normally used for output, but
// is used for some input keys including some from the keypad
int TerminalReadSSCSeq(STREAM *S)
{
    int inchar;

    inchar=STREAMReadChar(S);
    switch (inchar)
    {
    case 'A':
        return(TKEY_CTRL_UP);
        break;
    case 'B':
        return(TKEY_CTRL_DOWN);
        break;
    case 'c':
        return(TKEY_CTRL_LEFT);
        break;
    case 'd':
        return(TKEY_CTRL_RIGHT);
        break;
    case 'P':
        return(TKEY_F1);
        break;
    case 'Q':
        return(TKEY_F2);
        break;
    case 'R':
        return(TKEY_F3);
        break;
    case 'S':
        return(TKEY_F4);
        break;
    case 't':
        return(TKEY_F5);
        break;
    case 'u':
        return(TKEY_F6);
        break;
    case 'v':
        return(TKEY_F7);
        break;
    case 'l':
        return(TKEY_F8);
        break;
    case 'w':
        return(TKEY_F9);
        break;
    case 'x':
        return(TKEY_F10);
        break;
    }
    return(inchar);
}
