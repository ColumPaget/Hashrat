#include "Terminal.h"
#include "Unicode.h"
#include "Pty.h"
#include "String.h"
#include <sys/ioctl.h>
#include <termios.h>
#include "TerminalBar.h" //for TerminalBarsInit

static const char *ANSIColorStrings[]= {"none","black","red","green","yellow","blue","magenta","cyan","white",NULL};

TMouseEvent MouseEvent;


//Used internally by TerminalStrLen and TerminalStrTrunc this reads through a string and handles
//escaped characters (\n \b) that seem to be two chars, but are really 1, unicode strings, that
//see to be 2 or 3 characters, but area really 1, and 'tilde format' strings that describe colors
//and other terminal actions, which again seem to be 2 characters, but are really 1. If this function
//encounters something like '~r' which specifies following text is colored red, and which doesn't encode
//actual characters, it returns false. If it encounters anyting that does encode an actual character,
//including an actual character, it returns true

int TerminalConsumeCharacter(const char **ptr)
{
    int IsRealChar=FALSE;


    switch (**ptr)
    {
    case '~':
        ptr_incr(ptr, 1);
        switch (**ptr)
        {
        case '+':
            ptr_incr(ptr, 1);
            break;  // 'bright' colors, in the form ~+r so eat one more char
        case '=':
            ptr_incr(ptr, 1);
            break;  // 'fill with char' in the form ~=x (where 'x' is any char) so eat one more char
        case '~':
            IsRealChar=TRUE;
            break;  //~~ translates to ~, one character
        case ':':
            IsRealChar=TRUE;
            break;  //named unicode glyph. one character
        case 'U':                           //16-bit unicode number. one character encoded as a 4-char hex string
            ptr_incr(ptr, 4);
            IsRealChar=TRUE;
            break;
        }
        break;

    case '\\':
        ptr_incr(ptr, 1);
        switch (**ptr)
        {
        case '\0':
            break;

        //octal value
        case '0':
            ptr_incr(ptr, 4);
            IsRealChar=TRUE;
            break;

        //hex value in form
        case 'x':
            ptr_incr(ptr, 3);
            IsRealChar=TRUE;
            break;

        default:
            IsRealChar=TRUE;
            break;
        }
        break;

    default:
        //handle unicode
        if (**ptr & 128)
        {
            switch (**ptr & 224)
            {
            case 224:
                ptr_incr(ptr, 1);
            case 192:
                ptr_incr(ptr, 1);
            }

            IsRealChar=TRUE;
        }
        else IsRealChar=TRUE;
        break;
    }

    return(IsRealChar);
}



//this allows us to specify a maximum length to stop counting at
//which is useful for some internal processes
static int TerminalInternalStrLen(const char **Str, int MaxLen)
{
    const char *ptr;
    int len=0;

    if ((! Str) || (! *Str)) return(len);

    for (ptr=*Str; *ptr !='\0'; ptr++)
    {
        if (TerminalConsumeCharacter(&ptr)) len++;

        if ((MaxLen != -1) && (len > MaxLen))
        {
            *Str=ptr;
            break;
        }

    }

    *Str=ptr;
    return(len);
}


int TerminalStrLen(const char *Str)
{
    if (! Str) return(0);
    return(TerminalInternalStrLen(&Str, -1));
}



char *TerminalStrTrunc(char *Str, int MaxLen)
{
    char *ptr;
    int len=0;

    ptr=Str;
    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (TerminalConsumeCharacter((const char **) &ptr)) len++;
        if (len > MaxLen)
        {
            *ptr='\0';
            StrLenCacheAdd(Str, len);
            break;
        }
    }


    return(Str);
}




char *ANSICode(int Color, int BgColor, int Flags)
{
    static char *ANSI=NULL;
    int FgVal=-1, BgVal=-1;

    if ((! Color) && (! BgColor) && (! Flags)) return("");

    if (Color > 0)
    {
        if (Color >= ANSI_DARKGREY) FgVal=90+Color - ANSI_DARKGREY -1;
        else FgVal=30+Color-1;
    }

    if (BgColor > 0)
    {
        if (BgColor >= ANSI_DARKGREY) BgVal=100+BgColor - ANSI_DARKGREY -1;
        else BgVal=40+BgColor-1;
    }

    if ((FgVal > -1) && (BgVal > -1)) ANSI=FormatStr(ANSI,"\x1b[%d;%d",FgVal,BgVal);
    else if (BgVal > -1) ANSI=FormatStr(ANSI,"\x1b[%d",BgVal);
    else if (FgVal > -1) ANSI=FormatStr(ANSI,"\x1b[%d",FgVal);
    else ANSI=CopyStr(ANSI,"\x1b[");

    if (Flags)
    {
        if ( (FgVal > -1) || (BgVal > -1) ) ANSI=CatStr(ANSI, ";");
        if (Flags & ANSI_BOLD) ANSI=CatStr(ANSI,"01");
        if (Flags & ANSI_FAINT) ANSI=CatStr(ANSI,"02");
        if (Flags & ANSI_UNDER) ANSI=CatStr(ANSI,"04");
        if (Flags & ANSI_BLINK) ANSI=CatStr(ANSI,"05");
        if (Flags & ANSI_INVERSE) ANSI=CatStr(ANSI,"07");
    }
    ANSI=CatStr(ANSI,"m");

    return(ANSI);
}





int ANSIParseColor(const char *Str)
{
    int val;

    if (! Str) return(ANSI_NONE);
    val=MatchTokenFromList(Str, ANSIColorStrings, 0);
    if (val==-1) val=ANSI_NONE;

    return(val);
}





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


    case TKEY_FOCUS_IN:
        return("FOCUS_IN");
        break;
    case TKEY_FOCUS_OUT:
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
    case TKEY_SHIFT_FOCUS_IN:
        return("SHIFT_FOCUS_IN");
        break;
    case TKEY_SHIFT_FOCUS_OUT:
        return("SHIFT_FOCUS_OUT");
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
    case TKEY_CTRL_FOCUS_IN:
        return("CTRL_FOCUS_IN");
        break;
    case TKEY_CTRL_FOCUS_OUT:
        return("CTRL_FOCUS_OUT");
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
    case TKEY_ALT_FOCUS_IN:
        return("ALT_FOCUS_IN");
        break;
    case TKEY_ALT_FOCUS_OUT:
        return("ALT_FOCUS_OUT");
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

    //if control is pressed, and the key is an alphabetic character, then
    //convert it to a control char

    if ((*mod & KEYMOD_CTRL) && (*str > 65) && (*str < 122))
    {
        *mod=0; //do not add a mod for these, as the character code alone implies it
        return((tolower(*str) - 'a') +1);
    }
    return((int) *str);
}

int TerminalTranslateKeyStr(const char *str)
{
    int key, mod;

    key=TerminalTranslateKeyStrWithMod(str, &mod);
    if (mod==KEYMOD_SHIFT) key+=TKEY_SHIFT_BASE;
    if (mod==KEYMOD_CTRL) key+=TKEY_CTRL_BASE;
    if (mod==KEYMOD_ALT) key+=TKEY_ALT_BASE;

    return(key);
}

void TerminalGeometry(STREAM *S, int *wid, int *len)
{
    struct winsize w;

    ioctl(S->out_fd, TIOCGWINSZ, &w);

    *wid=w.ws_col;
    *len=w.ws_row;
}


void TerminalInternalConfig(const char *Config, int *ForeColor, int *BackColor, int *Flags)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;


    ptr=GetNameValuePair(Config, " ","=",&Name,&Value);
    while (ptr)
    {
        switch (*Name)
        {
        case 'f':
        case 'F':
            if (
                (strcasecmp(Name,"forecolor")==0) ||
                (strcasecmp(Name,"fgcolor")==0) ||
                (strcasecmp(Name,"fcolor")==0)
            )
            {
                if (ForeColor) *ForeColor=ANSIParseColor(Value);
            }
            break;

        case 'b':
        case 'B':
            if (
                (strcasecmp(Name,"backcolor") ==0) ||
                (strcasecmp(Name,"bgcolor") ==0) ||
                (strcasecmp(Name,"bcolor") ==0)
            )
            {
                if (BackColor) *BackColor=ANSIParseColor(Value);
            }
            else if (strcasecmp(Name, "bottom") ==0) *Flags |= TERMBAR_LOWER;
            break;

        case 'h':
        case 'H':
            if (strcasecmp(Name, "hidecursor") ==0) *Flags |= TERM_HIDECURSOR;
            if (strcasecmp(Name,"hidetext")==0) *Flags |= TERM_HIDETEXT;
            break;

        case 'm':
        case 'M':
            if (strcasecmp(Name,"mouse")==0) *Flags=TERM_MOUSE;
            break;

        case 'r':
        case 'R':
            if (strcasecmp(Name, "rawkeys") ==0) *Flags |= TERM_RAWKEYS;
            break;

        case 's':
        case 'S':
            if (strcasecmp(Name,"stars")==0) *Flags |= TERM_SHOWSTARS;
            if (strcasecmp(Name,"saveattribs")==0) *Flags |= TERM_SAVEATTRIBS;
            if (strcasecmp(Name,"save")==0) *Flags |= TERM_SAVEATTRIBS;
            break;


        case 't':
        case 'T':
            if (strcasecmp(Name, "top") ==0) *Flags |= TERMBAR_UPPER;
            if (strcasecmp(Name,"textstars")==0) *Flags |= TERM_SHOWTEXTSTARS;
            break;

        case 'w':
        case 'W':
            if (strcasecmp(Name,"wheelmouse")==0) *Flags=TERM_WHEELMOUSE;
            break;
        }

        ptr=GetNameValuePair(ptr, " ","=",&Name,&Value);
    }


    Destroy(Name);
    Destroy(Value);

}



const char *TerminalAlignText(const char *Text, char **RetStr, int Flags, STREAM *Term)
{
    int pos=0, cols, rows, len, i;
    const char *ptr;

    TerminalGeometry(Term, &cols, &rows);
    len=TerminalStrLen(Text);

    if (Flags & TERM_ALIGN_CENTER) pos=cols / 2 - len / 2;
    else if (Flags & TERM_ALIGN_RIGHT) pos=cols - len;

    len=StrLenFromCache(*RetStr);
    for (i=0; i < pos; i++)
    {
        *RetStr=AddCharToBuffer(*RetStr, len, ' ');
        len++;
    }

    for (ptr=Text; *ptr !='\0'; ptr++)
    {
        *RetStr=AddCharToBuffer(*RetStr, len, *ptr);
        len++;
    }

    StrLenCacheAdd(*RetStr, len);

    return(ptr);
}




char *TerminalCommandStr(char *RetStr, int Cmd, int Arg1, int Arg2)
{
    int i;
    char *Tempstr=NULL;

    switch (Cmd)
    {
    case TERM_CLEAR_SCREEN:
        RetStr=CatStr(RetStr, "\x1b[2J");
        break;

    case TERM_CLEAR_ENDLINE:
        RetStr=CatStr(RetStr,"\x1b[K");
        break;

    case TERM_CLEAR_STARTLINE:
        RetStr=CatStr(RetStr,"\x1b[1K");
        break;

    case TERM_CLEAR_LINE:
        RetStr=CatStr(RetStr,"\x1b[K");
        break;

    case TERM_CURSOR_HOME:
        RetStr=CatStr(RetStr,"\x1b[H");
        break;

    case TERM_CURSOR_MOVE:
        Tempstr=FormatStr(Tempstr,"\x1b[%d;%dH",Arg2+1,Arg1+1);
        RetStr=CatStr(RetStr,Tempstr);
        break;

    case TERM_CURSOR_SAVE:
        RetStr=CatStr(RetStr,"\x1b[s");
        break;

    case TERM_CURSOR_UNSAVE:
        RetStr=CatStr(RetStr,"\x1b[u");
        break;

    case TERM_CURSOR_HIDE:
        RetStr=CatStr(RetStr,"\x1b[?25l");
        break;

    case TERM_CURSOR_SHOW:
        RetStr=CatStr(RetStr,"\x1b[?25h");
        break;

    case TERM_SCROLL:
        if (Arg1 < 0) Tempstr=FormatStr(Tempstr,"\x1b[%dT", 0 - Arg1);
        else Tempstr=FormatStr(Tempstr,"\x1b[%dS", Arg1);
        RetStr=CatStr(RetStr,Tempstr);
        break;

    case TERM_SCROLL_REGION:
        Tempstr=FormatStr(Tempstr,"\x1b[%d;%dr",Arg1, Arg2);
        RetStr=CatStr(RetStr,Tempstr);
        break;

    case TERM_COLOR:
        RetStr=CatStr(RetStr,ANSICode(Arg1, Arg2, 0));
        break;

    case TERM_TEXT:
        RetStr=CatStr(RetStr,ANSICode(0, 0, Arg1));
        break;

    case TERM_NORM:
        RetStr=CatStr(RetStr,ANSI_NORM);
        break;

    case TERM_UNICODE:
        RetStr=StrAddUnicodeChar(RetStr, Arg1);
        break;

    case TERM_UNICODE_NAME:
        RetStr=StrAddUnicodeChar(RetStr, Arg1);
        break;
    }

    Destroy(Tempstr);
    return(RetStr);
}




char *TerminalColorCommandStr(char *RetStr, char inchar, int offset)
{

    switch (inchar)
    {
    case 'r':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_RED + offset, 0);
        break;
    case 'R':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_RED + offset);
        break;
    case 'g':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_GREEN + offset, 0);
        break;
    case 'G':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_GREEN + offset);
        break;
    case 'b':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_BLUE + offset, 0);
        break;
    case 'B':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_BLUE + offset);
        break;
    case 'n':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_BLACK + offset, 0);
        break;
    case 'N':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_BLACK + offset);
        break;
    case 'w':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_WHITE + offset, 0);
        break;
    case 'W':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_WHITE + offset);
        break;
    case 'y':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_YELLOW + offset, 0);
        break;
    case 'Y':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_YELLOW + offset);
        break;
    case 'm':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_MAGENTA + offset, 0);
        break;
    case 'M':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_MAGENTA + offset);
        break;
    case 'c':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_CYAN + offset, 0);
        break;
    case 'C':
        RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_CYAN + offset);
        break;
    }

    return(RetStr);
}



char *TerminalFillToEndOfLine(char *RetStr, int fill_char, STREAM *Term)
{
    const char *ptr=NULL;
    int i,twide, thigh;
    int len=0;

    if (StrValid(RetStr))
    {
        ptr=strrchr(RetStr, '\n');
        if (ptr) ptr++;
        else ptr=RetStr;
        len=TerminalStrLen(ptr);
    }

    TerminalGeometry(Term, &twide, &thigh);
    for (i=len; i < twide; i++) RetStr=AddCharToStr(RetStr, fill_char);

    return(RetStr);
}

const char *TerminalFormatSubStr(const char *Str, char **RetStr, STREAM *Term)
{
    const char *ptr, *end;
    char *Tempstr=NULL;
    long val;

    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (*ptr=='\\')
        {
            ptr++;
            switch (*ptr)
            {
            case 'a':
                *RetStr=AddCharToStr(*RetStr, '\x07');
                break;

            case 'e':
                *RetStr=AddCharToStr(*RetStr, '\x1b');
                break;

            case 'n':
                *RetStr=AddCharToStr(*RetStr, '\n');
                break;

            case 'r':
                *RetStr=AddCharToStr(*RetStr, '\r');
                break;

            case 't':
                *RetStr=AddCharToStr(*RetStr, '\t');
                break;

            case 'x':
                ptr++;
                strntol(&ptr, 2, 16, &val);
                *RetStr=AddCharToStr(*RetStr, val);
                ptr--;
                break;

            case '0':
                ptr++;
                strntol(&ptr, 3, 8, &val);
                *RetStr=AddCharToStr(*RetStr, val);
                ptr--;
                break;

            default:
                *RetStr=AddCharToStr(*RetStr, *ptr);
                break;
            }
        }
        else if (*ptr=='~')
        {
            ptr++;
            if (*ptr=='\0') break;
            switch (*ptr)
            {
            case '~':
                *RetStr=AddCharToStr(*RetStr, *ptr);
                break;
            case '+':
                ptr++;
                *RetStr=TerminalColorCommandStr(*RetStr, *ptr, ANSI_DARKGREY);
                break;

            case 'r':
            case 'R':
            case 'g':
            case 'G':
            case 'b':
            case 'B':
            case 'y':
            case 'Y':
            case 'm':
            case 'M':
            case 'c':
            case 'C':
            case 'w':
            case 'W':
            case 'n':
            case 'N':
                *RetStr=TerminalColorCommandStr(*RetStr, *ptr, 0);
                break;

            case 'e':
                *RetStr=TerminalCommandStr(*RetStr, TERM_TEXT, ANSI_BOLD, 0);
                break;
            case 'i':
                *RetStr=TerminalCommandStr(*RetStr, TERM_TEXT, ANSI_INVERSE, 0);
                break;
            case 'u':
                *RetStr=TerminalCommandStr(*RetStr, TERM_TEXT, ANSI_UNDER, 0);
                break;
            case 'I':
                *RetStr=TerminalCommandStr(*RetStr, TERM_TEXT, ANSI_INVERSE, 0);
                break;
            case '<':
                *RetStr=TerminalCommandStr(*RetStr, TERM_CLEAR_STARTLINE, 0, 0);
                break;
            case '>':
                *RetStr=TerminalCommandStr(*RetStr, TERM_CLEAR_ENDLINE, 0, 0);
                break;
            case '=':
                ptr++;
                *RetStr=TerminalFillToEndOfLine(*RetStr, *ptr, Term);
                break;
            case '|':
                Tempstr=CopyStr(Tempstr, "");
                ptr=TerminalAlignText(ptr+1, &Tempstr, TERM_ALIGN_CENTER, Term);
                TerminalFormatSubStr(Tempstr, RetStr, Term);
                ptr--; //we have to wind ptr back one place, so that next time around the loop ptr++ points to the right character
                break;
            case '}':
                Tempstr=CopyStr(Tempstr, "");
                ptr=TerminalAlignText(ptr+1, &Tempstr, TERM_ALIGN_RIGHT, Term);
                TerminalFormatSubStr(Tempstr, RetStr, Term);
                ptr--; //we have to wind ptr back one place, so that next time around the loop ptr++ points to the right character
                break;
            case 'U':
                ptr++;
                if (! strntol(&ptr, 4, 16, &val)) break;
                ptr--; //because of ptr++ on next loop
                *RetStr=TerminalCommandStr(*RetStr, TERM_UNICODE, val, 0);
                break;
            case ':':
                ptr=GetToken(ptr+1, ":", &Tempstr, 0);
                ptr--; //because of ptr++ on next loop
                *RetStr=UnicodeStrFromName(*RetStr, Tempstr);
                break;
            case '0':
                *RetStr=TerminalCommandStr(*RetStr, TERM_NORM, 0, 0);
                break;
            }
        }
        //if top bit is set then this is unicode
        else if (*ptr & 128)
        {
            val=UnicodeDecode(&ptr);
            if (val > 0) *RetStr=TerminalCommandStr(*RetStr, TERM_UNICODE, val, 0);
        }
        else *RetStr=AddCharToStr(*RetStr, *ptr);
    }

    Destroy(Tempstr);

    return(ptr);
}


char *TerminalFormatStr(char *RetStr, const char *Str, STREAM *Term)
{
    TerminalFormatSubStr(Str, &RetStr, Term);
    return(RetStr);
}


void TerminalCommand(int Cmd, int Arg1, int Arg2, STREAM *S)
{
    char *Tempstr=NULL;

    Tempstr=TerminalCommandStr(Tempstr, Cmd, Arg1, Arg2);
    STREAMWriteString(Tempstr, S);

    Destroy(Tempstr);
}




//this can put uinicode characters
void TerminalPutChar(int Char, STREAM *S)
{
    char *Tempstr=NULL;
    char towrite;

    if (Char <= 0x7f)
    {
        towrite=Char;
        if (S) STREAMWriteChar(S, towrite);
        else write(1, &towrite, 1);
    }
    else
    {
        Tempstr=UnicodeStr(Tempstr, Char);
        if (S) STREAMWriteLine(Tempstr, S);
        else write(1,Tempstr,StrLenFromCache(Tempstr));
    }

    Destroy(Tempstr);
}




void TerminalPutStr(const char *Str, STREAM *S)
{
    char *Tempstr=NULL;

    Tempstr=TerminalFormatStr(Tempstr, Str, S);
    if (S) STREAMWriteLine(Tempstr, S);
    else write(1,Tempstr,StrLenFromCache(Tempstr));

    Destroy(Tempstr);
}


void XtermStringCommand(const char *Prefix, const char *Str, const char *Postfix, STREAM *S)
{
    char *Cmd=NULL, *Tempstr=NULL;

    Cmd=MCopyStr(Cmd, Prefix, Str, Postfix, NULL);
    Tempstr=TerminalFormatStr(Tempstr, Cmd, S);
    STREAMWriteLine(Tempstr, S);
    Destroy(Tempstr);
    Destroy(Cmd);
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


static int TerminalReadCSISeq(STREAM *S, char PrevChar)
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


static int TerminalReadCSIMouse(STREAM *S)
{
    char *Tempstr=NULL;
    int flags, val, keycode=0;

    MouseEvent.flags=STREAMReadChar(S)-32;
    MouseEvent.x=STREAMReadChar(S)-32;
    MouseEvent.y=STREAMReadChar(S)-32;

    switch (MouseEvent.flags)
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

    MouseEvent.button=keycode;

    Destroy(Tempstr);

    return(keycode);
}




int TerminalReadChar(STREAM *S)
{
    int inchar;

    inchar=STREAMReadChar(S);
    if (inchar == ESCAPE)
    {
        inchar=STREAMReadChar(S);
        switch (inchar)
        {
        case ESCAPE:
            return(ESCAPE);
            break;

        case '[':
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
                return(TKEY_FOCUS_IN);
                break;
            case 'O':
                return(TKEY_FOCUS_OUT);
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
                return(TerminalReadCSISeq(S, inchar));
                break;
            }
            break;

        case 'O':
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
            break;

        }
    }

    return(inchar);
}


int TerminalTextConfig(const char *Config)
{
    if (! StrValid(Config)) return(0);
    if (strcasecmp(Config, "hidetext")==0) return(TERM_HIDETEXT);
    if (strcasecmp(Config, "stars")==0) return(TERM_SHOWSTARS);
    if (strcasecmp(Config, "stars+1")==0) return(TERM_SHOWTEXTSTARS);
    return(0);
}


char *TerminalReadText(char *RetStr, int Flags, STREAM *S)
{
    int inchar, len=0;
    char outchar;
    TKEY_CALLBACK_FUNC Func;

    len=StrLen(RetStr);
    if (len > 0)
    {
        STREAMWriteLine(RetStr, S);
        STREAMFlush(S);
    }

    inchar=TerminalReadChar(S);
    while (inchar != EOF)
    {
        Func=STREAMGetItem(S, "KeyCallbackFunc");
        if (Func) Func(S, inchar);
        outchar=inchar & 0xFF;

        switch (inchar)
        {
        case ESCAPE:
            Destroy(RetStr);
            return(NULL);
            break;

        case STREAM_TIMEOUT:
        case STREAM_NODATA:
        case '\n':
        case '\r':
            break;

        //'backspace' key on keyboard will send the 'del' character in some cases!
        case 0x7f: //this is 'del'
        case 0x08: //this is backspace
            outchar=0;
            //backspace over previous character and erase it with whitespace!
            if (len > 0)
            {
                STREAMWriteString("\x08 ",S);
                outchar=0x08;
                len--;
            }
            break;

        default:
            if (Flags & TERM_SHOWSTARS) outchar='*';
            else if ((Flags & TERM_SHOWTEXTSTARS) && (len > 0))
            {
                //backspace over previous character and replace with star
                STREAMWriteString("\x08*",S);
            }
            RetStr=AddCharToBuffer(RetStr,len++, inchar & 0xFF);
            break;
        }

        if ( (inchar == '\n') || (inchar == '\r') )
        {
            //backspace over previous character and replace with star, so the
            //last character is not left chnaging when we press 'enter'
            if (Flags & TERM_SHOWTEXTSTARS) STREAMWriteString("\x08*",S);

            //ensure we don't return NULL, but that we instead return an empty string
            if (RetStr==NULL) RetStr=CatStr(RetStr, "");

            break;
        }

        if ( (! (Flags & TERM_HIDETEXT)) && (outchar > 0) )
        {
            STREAMWriteBytes(S, &outchar,1);
            STREAMFlush(S);
        }

        inchar=TerminalReadChar(S);
    }

    StrLenCacheAdd(RetStr, len);

    return(RetStr);
}



char *TerminalReadPrompt(char *RetStr, const char *Prompt, int Flags, STREAM *S)
{
    int TTYFlags=0;

    TerminalPutStr("~>",S);
    TerminalPutStr(Prompt, S);
    STREAMFlush(S);
    if (Flags & (TERM_HIDETEXT | TERM_SHOWSTARS | TERM_SHOWTEXTSTARS))
    {
        TTYFlags=TTYGetConfig(S->in_fd);
        TTYSetEcho(S->in_fd, FALSE);
        TTYSetCanonical(S->in_fd, FALSE);
    }
    RetStr=TerminalReadText(RetStr, Flags, S);
    if (TTYFlags & TTYFLAG_ECHO) TTYSetEcho(S->in_fd, TRUE);
    if (TTYFlags & TTYFLAG_CANON) TTYSetCanonical(S->in_fd, TRUE);

    return(RetStr);
}




int TerminalInit(STREAM *S, int Flags)
{
    int cols, rows;
    char *Tempstr=NULL;
    int ttyflags=0;

    TerminalGeometry(S, &cols, &rows);
    Tempstr=FormatStr(Tempstr,"%d",cols);
    STREAMSetValue(S, "Terminal:cols", Tempstr);
    Tempstr=FormatStr(Tempstr,"%d",rows);
    STREAMSetValue(S, "Terminal:rows", Tempstr);
    STREAMSetValue(S, "Terminal:top", "0");

    if (Flags & TERM_HIDECURSOR) TerminalCursorHide(S);
    if (isatty(S->in_fd))
    {
        if (Flags & TERM_SAVEATTRIBS) ttyflags=TTYFLAG_SAVE;
        if (Flags & TERM_RAWKEYS) ttyflags |= TTYFLAG_IN_CRLF | TTYFLAG_OUT_CRLF;
        if (ttyflags) TTYConfig(S->in_fd, 0, ttyflags);
    }
    TerminalBarsInit(S);
    if (Flags & TERM_WHEELMOUSE) STREAMWriteLine("\x1b[?1000h", S);
    else if (Flags & TERM_MOUSE) STREAMWriteLine("\x1b[?9h", S);

    Destroy(Tempstr);
    return(TRUE);
}


void TerminalSetup(STREAM *S, const char *Config)
{
    int Flags=0, FColor, BColor;

    TerminalInternalConfig(Config, &FColor, &BColor, &Flags);
    TerminalInit(S, Flags);
}


void TerminalReset(STREAM *S)
{
    int top=0, bottom=0;
    const char *ptr;

    ptr=STREAMGetValue(S, "Terminal:top");
    if (ptr) top = atoi(ptr);

    ptr=STREAMGetValue(S, "Terminal:bottom");
    if (ptr) bottom = atoi(ptr);

    if ((top > 0) || (bottom > 0)) STREAMWriteLine("\x1b[m\x1b[r", S);

    STREAMWriteLine("\x1b[?9l", S);
    STREAMWriteLine("\x1b[?1000l", S);
    STREAMFlush(S);
    if (isatty(S->in_fd)) TTYReset(S->in_fd);
}


void TerminalSetKeyCallback(STREAM *Term, TKEY_CALLBACK_FUNC Func)
{
    STREAMSetItem(Term, "KeyCallbackFunc", Func);
}


TMouseEvent *TerminalGetMouse(STREAM *Term)
{
    return(&MouseEvent);
}
