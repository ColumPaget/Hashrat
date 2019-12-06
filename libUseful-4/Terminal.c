#include "Terminal.h"
#include "Unicode.h"
#include "Pty.h"
#include "String.h"
#include <sys/ioctl.h>
#include <termios.h>

static const char *ANSIColorStrings[]= {"none","black","red","green","yellow","blue","magenta","cyan","white",NULL};



int TerminalStrLen(const char *Str)
{
    const char *ptr;
    int len=0;

    if (! Str) return(len);
    for (ptr=Str; *ptr !='\0'; ptr++)
    {
    		if (*ptr & 128)
				{
					switch (*ptr & 224)
					{ 
					case 224:  ptr_incr(&ptr, 1);
					case 192:  ptr_incr(&ptr, 1);
					}

					len++;
				}
        else if (*ptr=='~')
        {
						ptr_incr(&ptr, 1);
            if (*ptr=='~') len++;
        }
        else len++;
    }

    return(len);
}


char *TerminalStrTrunc(const char *Str, int MaxLen)
{
    const char *ptr;
    int len=0;

    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (*ptr=='~')
        {
            ptr++;
            if (*ptr=='~') len++;
        }
        else len++;

				if (len > MaxLen)
				{
					Str=StrTrunc(Str, ptr+1-Str);
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
        if (Color >= ANSI_DARKGREY) FgVal=90+Color - ANSI_DARKGREY;
        else FgVal=30+Color-1;
    }

    if (BgColor > 0)
    {
        if (BgColor >= ANSI_DARKGREY) BgVal=100+BgColor - ANSI_DARKGREY;
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
    case ESCAPE:
        return("ESC");
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
    }

    KeyStr[0]='?';
    KeyStr[1]='\0';
    if ((key > 31) && (key < 127)) KeyStr[0]=key & 0xFF;
    else if ((key=='\n') || (key=='\r')) KeyStr[0]=key & 0xFF;
    return(KeyStr);
}




int TerminalTranslateKeyStrWithMod(const char *str, int *mod)
{

if (*mod) *mod=0;

//read as many modifiers as are found, then break
while (1)
{
  if (strncasecmp(str, "shift-", 6)==0)
  {
  if (mod !=NULL) *mod |= KEYMOD_SHIFT;
  str+=6;
  }
  else if (strncasecmp(str, "ctrl-", 5)==0)
  {
  if (mod !=NULL) *mod |= KEYMOD_CTRL;
  str+=5;
  }
  else if (strncasecmp(str, "alt-", 4)==0)
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
  if (strcasecmp(str, "calc")==0) return(TKEY_CALC);
  if (strcasecmp(str, "calculator")==0) return(TKEY_CALC);
  if (strcasecmp(str, "copy")==0) return(TKEY_COPY);
  if (strcasecmp(str, "cut")==0) return(TKEY_CUT);
  if (strcasecmp(str, "clear")==0) return(TKEY_CLEAR);
  if (strcasecmp(str, "computer")==0) return(TKEY_MYCOMPUTER);
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
	if (strcasecmp(str, "voldown")==0) return(TKEY_UP);
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

        case 'r':
        case 'R':
            if (strcasecmp(Name, "rawkeys") ==0) *Flags |= TERM_RAWKEYS;
            break;

        case 's':
            if (strcasecmp(Name,"stars")==0) *Flags |= TERM_SHOWSTARS;
            break;


        case 't':
        case 'T':
            if (strcasecmp(Name, "top") ==0) *Flags |= TERMBAR_UPPER;
            if (strcasecmp(Name,"textstars")==0) *Flags |= TERM_SHOWTEXTSTARS;
            break;
        }

        ptr=GetNameValuePair(ptr, " ","=",&Name,&Value);
    }


    DestroyString(Name);
    DestroyString(Value);

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
    }

    DestroyString(Tempstr);
    return(RetStr);
}


const char *TerminalFormatSubStr(const char *Str, char **RetStr, STREAM *Term)
{
    const char *ptr, *end;
		char *Tempstr=NULL;
    long val;

    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (*ptr=='~')
        {
            ptr++;
            if (*ptr=='\0') break;
            switch (*ptr)
            {
            case '~':
                *RetStr=AddCharToStr(*RetStr, *ptr);
                break;
            case 'r':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_RED, 0);
                break;
            case 'R':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_RED);
                break;
            case 'g':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_GREEN, 0);
                break;
            case 'G':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_GREEN);
                break;
            case 'b':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_BLUE, 0);
                break;
            case 'B':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_BLUE);
                break;
            case 'n':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_BLACK, 0);
                break;
            case 'N':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_BLACK);
                break;
            case 'w':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_WHITE, 0);
                break;
            case 'W':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_WHITE);
                break;
            case 'y':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_YELLOW, 0);
                break;
            case 'Y':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_YELLOW);
                break;
            case 'm':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_MAGENTA, 0);
                break;
            case 'M':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_MAGENTA);
                break;
            case 'c':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, ANSI_CYAN, 0);
                break;
            case 'C':
                *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, 0, ANSI_CYAN);
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

    DestroyString(Tempstr);
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

    DestroyString(Tempstr);
}




void TerminalPutStr(const char *Str, STREAM *S)
{
    char *Tempstr=NULL;

    Tempstr=TerminalFormatStr(Tempstr, Str, S);
    if (S) STREAMWriteLine(Tempstr, S);
    else write(1,Tempstr,StrLenFromCache(Tempstr));

    DestroyString(Tempstr);
}





int TerminalReadCSISeq(STREAM *S, char PrevChar)
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
    		inchar=STREAMReadChar(S);
				switch (inchar)
				{
					case '2':
    				inchar=STREAMReadChar(S);
						switch (inchar)
						{
							case 'A': return(TKEY_SHIFT_UP); break;
							case 'B': return(TKEY_SHIFT_DOWN); break;
							case 'C': return(TKEY_SHIFT_RIGHT); break;
							case 'D': return(TKEY_SHIFT_LEFT); break;
							case 'F': return(TKEY_SHIFT_END); break;
							case 'H': return(TKEY_SHIFT_HOME); break;
						}
					break;

					case '3':
    				inchar=STREAMReadChar(S);
						switch (inchar)
						{
							case 'A': return(TKEY_ALT_UP); break;
							case 'B': return(TKEY_ALT_DOWN); break;
							case 'C': return(TKEY_ALT_RIGHT); break;
							case 'D': return(TKEY_ALT_LEFT); break;
							case 'F': return(TKEY_ALT_END); break;
							case 'H': return(TKEY_ALT_HOME); break;
						}
					break;

	
					case '5':
    				inchar=STREAMReadChar(S);
						switch (inchar)
						{
							case 'A': return(TKEY_CTRL_UP); break;
							case 'B': return(TKEY_CTRL_DOWN); break;
							case 'C': return(TKEY_CTRL_RIGHT); break;
							case 'D': return(TKEY_CTRL_LEFT); break;
							case 'F': return(TKEY_CTRL_END); break;
							case 'H': return(TKEY_CTRL_HOME); break;
	
						}
					break;
				}
			break;

    }

    DestroyString(Tempstr);

    return(0);
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
            case 'H':
                return(TKEY_HOME);
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
            //backspace over previous character and replace with star
            if (Flags & TERM_SHOWTEXTSTARS) STREAMWriteString("\x08*",S);
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
    TerminalPutStr("\r~>",S);
    TerminalPutStr(Prompt, S);
    STREAMFlush(S);
    return(TerminalReadText(RetStr, Flags, S));
}


void TerminalBarsInit(STREAM *S)
{
    int top=0, bottom=0, cols, rows;
    ListNode *Curr;
		char *Tempstr=NULL;
    TERMBAR *TB;

    TerminalGeometry(S, &cols, &rows);
    Curr=ListGetNext(S->Items);
    while (Curr)
    {
        if (strcmp(Curr->Tag, "termbar")==0)
        {
            TB=(TERMBAR *) Curr->Item;
            if (TB->Flags & TERMBAR_UPPER) top=1;
            else bottom=1;
        }
        Curr=ListGetNext(Curr);
    }

    if ((top > 0) || (bottom > 0))
    {
        if (top > 0) 
				{
						Tempstr=FormatStr(Tempstr, "%d", top);			
						STREAMSetValue(S, "Terminal:top",Tempstr);
				}

        if (bottom > 0) 
				{
						Tempstr=FormatStr(Tempstr, "%d", bottom);			
						STREAMSetValue(S, "Terminal:bottom",Tempstr);
				}

        TerminalCommand(TERM_SCROLL_REGION, top+2, rows-(top+bottom), S);
        TerminalCommand(TERM_CURSOR_MOVE, 0, top+2, S);
    }

Destroy(Tempstr);
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

    DestroyString(Tempstr);
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

		STREAMFlush(S);
    if (isatty(S->in_fd)) TTYReset(S->in_fd);
}


void TerminalSetKeyCallback(STREAM *Term, TKEY_CALLBACK_FUNC Func)
{
    STREAMSetItem(Term, "KeyCallbackFunc", Func);
}


void TerminalBarSetConfig(TERMBAR *TB, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

		//first check for options only used in terminal bars
    ptr=GetNameValuePair(Config, " ","=",&Name,&Value);
    while (ptr)
    {
        switch (*Name)
        {
        case 'm':
        case 'M':
            if (strcasecmp(Name,"MenuPadLeft")==0) TB->MenuPadLeft=CopyStr(TB->MenuPadLeft, Value);
            if (strcasecmp(Name,"MenuPadRight")==0) TB->MenuPadRight=CopyStr(TB->MenuPadRight, Value);
            if (strcasecmp(Name,"MenuCursorLeft")==0) TB->MenuCursorLeft=CopyStr(TB->MenuCursorLeft, Value);
            if (strcasecmp(Name,"MenuCursorRight")==0) TB->MenuCursorRight=CopyStr(TB->MenuCursorRight, Value);
            break;
        }
        ptr=GetNameValuePair(ptr, " ","=",&Name,&Value);
    }

		//then check for default options, backcolor and forecolor reversed because terminal bars
		//are inverse text
    TerminalInternalConfig(Config, &(TB->BackColor), &(TB->ForeColor), &(TB->Flags));

    DestroyString(Name);
    DestroyString(Value);
}


TERMBAR *TerminalBarCreate(STREAM *Term, const char *Config, const char *Text)
{
    TERMBAR *TB;
    char *Tempstr=NULL;
    const char *ptr;

    TB=(TERMBAR *) calloc(1,sizeof(TERMBAR));
    TB->Term=Term;
    TB->Flags = ANSI_INVERSE;
    TB->MenuPadLeft=CopyStr(NULL, "  ");
    TB->MenuPadRight=CopyStr(NULL, "  ");
    TB->MenuCursorLeft=CopyStr(NULL, " [");
    TB->MenuCursorRight=CopyStr(NULL, "] ");

    TerminalBarSetConfig(TB, Config);
    STREAMSetItem(Term, "termbar", TB);

    TerminalBarsInit(Term);

    if (StrValid(Text)) TerminalBarUpdate(TB, Text);
    DestroyString(Tempstr);

    return(TB);
}


void TerminalBarDestroy(TERMBAR *TB)
{
    DestroyString(TB->MenuPadLeft);
    DestroyString(TB->MenuPadRight);
    DestroyString(TB->MenuCursorLeft);
    DestroyString(TB->MenuCursorRight);
    free(TB);
}


void TerminalBarUpdate(TERMBAR *TB, const char *Text)
{
    int rows, cols, x=0, y=0, TextLen;
    char *Str=NULL;

    TextLen=TerminalStrLen(Text);
    TerminalGeometry(TB->Term, &cols, &rows);
    TerminalCommand(TERM_CURSOR_SAVE, 0, 0, TB->Term);

    if (TB->Flags & TERMBAR_UPPER) y=0;
    else y=rows-1;

    TerminalCommand(TERM_CURSOR_MOVE, 0, y, TB->Term);

    Str=MCopyStr(Str, ANSICode(TB->ForeColor, TB->BackColor, TB->Flags), Text, NULL);
    Str=PadStr(Str, ' ', cols-TextLen);


    TerminalPutStr(Str, TB->Term);
    TerminalCommand(TERM_CURSOR_UNSAVE, 0, 0, TB->Term);

    DestroyString(Str);
}







char *TerminalBarReadText(char *RetStr, TERMBAR *TB, int Flags, const char *Prompt)
{
    int inchar, PromptLen;
    TKEY_CALLBACK_FUNC Func;
    char *DisplayString=NULL;
    const char *ptr;
    int cols, rows;

    RetStr=CopyStr(RetStr,"");
    PromptLen=StrLen(Prompt);
    TerminalGeometry(TB->Term, &cols, &rows);
    TerminalBarUpdate(TB, Prompt);
    TB->TextLen=0;

    inchar=TerminalReadChar(TB->Term);
    while (inchar != EOF)
    {
        Func=STREAMGetItem(TB->Term, "KeyCallbackFunc");
        if (Func) Func(TB->Term, inchar);

        switch (inchar)
        {
        case STREAM_TIMEOUT:
        case STREAM_NODATA:
            DestroyString(DisplayString);
            return(RetStr);
            break;

        case '\n':
            RetStr=CopyStrLen(RetStr, TB->Text, TB->TextLen);
            TB->TextLen=0;
            DestroyString(DisplayString);
            return(RetStr);
            break;

        case '\b':
            TB->Text[TB->TextLen]='\0';
            if (TB->TextLen > 0) TB->TextLen--;
            break;

        default:
            if ((inchar > 31) && (inchar < 127))
            {
                TB->Text=AddCharToBuffer(TB->Text,TB->TextLen,inchar);
                TB->TextLen++;
            }
            break;
        }

        if ((TB->TextLen + PromptLen) > (cols-1)) ptr=TB->Text + (TB->TextLen - (cols - 1 - PromptLen));
        else ptr=TB->Text;

        if (Flags & TERM_HIDETEXT) ptr=NULL ;
        else if (Flags & TERM_SHOWSTARS) DisplayString=CopyPadStr(DisplayString, Prompt, '*', StrLen(ptr));
        else DisplayString=MCopyStr(DisplayString, Prompt, ptr, NULL);

        TerminalBarUpdate(TB, DisplayString);
        inchar=TerminalReadChar(TB->Term);
    }

    DestroyString(DisplayString);
    DestroyString(RetStr);
    return(NULL);
}



void TerminalBarMenuUpdate(TERMBAR *TB, ListNode *Items)
{
    ListNode *Curr;
    char *Tempstr=NULL;

    Curr=ListGetNext(Items);
    while (Curr)
    {
        if (Items->Side==Curr)
        {
            Tempstr=MCatStr(Tempstr, TB->MenuCursorLeft, Curr->Tag, TB->MenuCursorRight,NULL);
        }
        else Tempstr=MCatStr(Tempstr, TB->MenuPadLeft,Curr->Tag,TB->MenuPadRight,NULL);

        Curr=ListGetNext(Curr);
    }

    TerminalBarUpdate(TB, Tempstr);

    DestroyString(Tempstr);
}



char *TerminalBarMenu(char *RetStr, TERMBAR *TB, const char *ItemStr)
{
    ListNode *Items, *Curr;
    const char *ptr;
    char *Token=NULL;
    int inchar, Done=FALSE;

    Items=ListCreate();
    ptr=GetToken(ItemStr, ",", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        ListAddNamedItem(Items, Token, NULL);
        ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
    }

    Curr=ListGetNext(Items);
    Items->Side=Curr;

    TerminalBarMenuUpdate(TB, Items);
    inchar=TerminalReadChar(TB->Term);
    while (! Done)
    {
        switch (inchar)
        {
        case EOF:
            Done=TRUE;
            break;

        case '<':
        case TKEY_LEFT:
            Curr=ListGetPrev(Items->Side);
            if (Curr) Items->Side=Curr;
            break;

        case '>':
        case TKEY_RIGHT:
            Curr=ListGetNext(Items->Side);
            if (Curr) Items->Side=Curr;
            break;

        case '\r':
        case '\n':
            RetStr=CopyStr(RetStr, Items->Side->Tag);
            Done=TRUE;
            break;
        }

        if (Done) break;
        TerminalBarMenuUpdate(TB, Items);
        inchar=TerminalReadChar(TB->Term);
    }


    DestroyString(Token);
    ListDestroy(Items, NULL);

    return(RetStr);
}




TERMMENU *TerminalMenuCreate(STREAM *Term, int x, int y, int wid, int high)
{
TERMMENU *Item;

Item=(TERMMENU *) calloc(1,sizeof(TERMMENU));
Item->Term=Term;
Item->x=x;
Item->y=y;
Item->wid=wid;
Item->high=high;
Item->Options=ListCreate();
Item->MenuAttribs=CopyStr(Item->MenuAttribs, "~C~n");
Item->MenuCursorLeft=CopyStr(Item->MenuCursorLeft, "~W~n");

return(Item);
}


void TerminalMenuDestroy(TERMMENU *Item)
{
Destroy(Item->MenuAttribs);
Destroy(Item->MenuCursorLeft);
Destroy(Item->MenuCursorRight);

ListDestroy(Item->Options, NULL);
free(Item);
}




void TerminalMenuDraw(TERMMENU *Menu)
{
		ListNode *Curr, *Prev;
		char *Contents=NULL, *Tempstr=NULL, *Output=NULL;
		char *p_Attribs, *p_Cursor, *p_Color=NULL;
		int y, yend, count;

		y=Menu->y;
		yend=y+Menu->high;

    if (Menu->Options->Side)
    {
      Curr=Menu->Options->Side;
      Prev=ListGetPrev(Curr);
      count=0;
      while (Prev)
      {
      Curr=Prev;
      count++;
      if (count >= Menu->high) break;
      Prev=ListGetPrev(Curr);
      }
    }
    else Curr=ListGetNext(Menu->Options);

		while (Curr)
		{
					TerminalCursorMove(Menu->Term, Menu->x, y);
					if (Menu->Options->Side==Curr) 
					{
						p_Attribs=Menu->MenuCursorLeft;
						p_Cursor="> ";
					}
					else 
					{
						p_Attribs=Menu->MenuAttribs;
						p_Cursor="  ";
					}

					Contents=ReplaceStr(Contents, Curr->Tag, "~0", p_Attribs);
					Contents=TerminalStrTrunc(Contents, Menu->wid-4);
					
					Tempstr=MCopyStr(Tempstr, p_Attribs, p_Cursor, Contents, NULL);

					Output=CopyStr(Output, "");
					TerminalFormatSubStr(Tempstr, &Output, Menu->Term);

					//length has two added for the leading space for the cursor

					count=TerminalStrLen(Contents);
					while (count < Menu->wid-2)
					{
							Output=CatStr(Output, " ");
							count++;
					}
					STREAMWriteString(Output, Menu->Term);
					STREAMWriteString(ANSI_NORM, Menu->Term);
					y++;
					if (y > yend) break;
					Curr=ListGetNext(Curr);
		}

		Tempstr=CopyStr(Tempstr, "");
		Tempstr=PadStrTo(Tempstr, ' ', Menu->wid);
		while (y < yend)
		{
			STREAMWriteString(Tempstr, Menu->Term);
			y++;
		}

		STREAMFlush(Menu->Term);

		Destroy(Contents);
		Destroy(Tempstr);
		Destroy(Output);
}


ListNode *TerminalMenuOnKey(TERMMENU *Menu, int key)
{
int i;

	switch (key)
	{
		case TKEY_UP:
				if (Menu->Options->Side) 
				{
					if (Menu->Options->Side->Prev && (Menu->Options->Side->Prev != Menu->Options)) Menu->Options->Side=Menu->Options->Side->Prev;
				}
				else Menu->Options->Side=ListGetNext(Menu->Options);
			 break;

		case TKEY_DOWN:
				if (Menu->Options->Side) 
				{
					if (Menu->Options->Side->Next) Menu->Options->Side=Menu->Options->Side->Next;
				}
				else Menu->Options->Side=ListGetNext(Menu->Options);
			 break;

		case TKEY_PGUP:
			 for (i=0; i < Menu->high; i++)
			 {
				if (Menu->Options->Side) 
				{
					if (Menu->Options->Side->Next) Menu->Options->Side=Menu->Options->Side->Next;
				}
				else Menu->Options->Side=ListGetNext(Menu->Options);
			 }
			 break;	


		case TKEY_PGDN:
			 for (i=0; i < Menu->high; i++)
			 {
				if (Menu->Options->Side) 
				{
					if (Menu->Options->Side->Next) Menu->Options->Side=Menu->Options->Side->Next;
				}
				else Menu->Options->Side=ListGetNext(Menu->Options);
			 }	 
			 break;	

		case '\r':
		case '\n':
			 return(Menu->Options->Side);
			 break;
	}
	TerminalMenuDraw(Menu);

	return(NULL);
}



ListNode *TerminalMenuProcess(TERMMENU *Menu)
{
ListNode *Node;
int key;

TerminalMenuDraw(Menu);

key=TerminalReadChar(Menu->Term);
while (1)
{
	if (key == ESCAPE)
	{
		Node=NULL;
		break;
	}
	Node=TerminalMenuOnKey(Menu, key);
	if (Node) break;
	key=TerminalReadChar(Menu->Term);
}


return(Node);
}


ListNode *TerminalMenu(STREAM *Term, ListNode *Options, int x, int y, int wid, int high)
{
TERMMENU *Menu;
ListNode *Node;
int key;

Menu=TerminalMenuCreate(Term, x, y, wid, high);
Node=ListGetNext(Options);
while (Node)
{
	ListAddNamedItem(Menu->Options, Node->Tag, NULL);
	Node=ListGetNext(Node);
}
Menu->Options->Side=ListGetNext(Menu->Options);

Node=TerminalMenuProcess(Menu);

TerminalMenuDestroy(Menu);
return(Node);
}
