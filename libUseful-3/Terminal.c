#include "Terminal.h"
#include "Unicode.h"
#include "Pty.h"
#include <sys/ioctl.h>

const char *ANSIColorStrings[]= {"none","black","red","green","yellow","blue","magenta","cyan","white",NULL};




int TerminalStrLen(const char *Str)
{
    const char *ptr;
    int i=0;

    if (! Str) return(i);
    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (*ptr=='~')
        {
            ptr++;
            if (*ptr=='~') i++;
        }
        else i++;
    }


    return(i);
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
    case KEY_F1:
        return("F1");
        break;
    case KEY_F2:
        return("F2");
        break;
    case KEY_F3:
        return("F3");
        break;
    case KEY_F4:
        return("F4");
        break;
    case KEY_F5:
        return("F5");
        break;
    case KEY_F6:
        return("F6");
        break;
    case KEY_F7:
        return("F7");
        break;
    case KEY_F8:
        return("F8");
        break;
    case KEY_F9:
        return("F9");
        break;
    case KEY_F10:
        return("F10");
        break;
    case KEY_F11:
        return("F11");
        break;
    case KEY_F12:
        return("F12");
        break;
    case KEY_F13:
        return("F13");
        break;
    case KEY_F14:
        return("F14");
        break;

    case KEY_SHIFT_F1:
        return("SHIFT_F1");
        break;
    case KEY_SHIFT_F2:
        return("SHIFT_F2");
        break;
    case KEY_SHIFT_F3:
        return("SHIFT_F3");
        break;
    case KEY_SHIFT_F4:
        return("SHIFT_F4");
        break;
    case KEY_SHIFT_F5:
        return("SHIFT_F5");
        break;
    case KEY_SHIFT_F6:
        return("SHIFT_F6");
        break;
    case KEY_SHIFT_F7:
        return("SHIFT_F7");
        break;
    case KEY_SHIFT_F8:
        return("SHIFT_F8");
        break;
    case KEY_SHIFT_F9:
        return("SHIFT_F9");
        break;
    case KEY_SHIFT_F10:
        return("SHIFT_F10");
        break;
    case KEY_SHIFT_F11:
        return("SHIFT_F11");
        break;
    case KEY_SHIFT_F12:
        return("SHIFT_F12");
        break;
    case KEY_SHIFT_F13:
        return("SHIFT_F13");
        break;
    case KEY_SHIFT_F14:
        return("SHIFT_F14");
        break;

    case KEY_CTRL_F1:
        return("CTRL_F1");
        break;
    case KEY_CTRL_F2:
        return("CTRL_F2");
        break;
    case KEY_CTRL_F3:
        return("CTRL_F3");
        break;
    case KEY_CTRL_F4:
        return("CTRL_F4");
        break;
    case KEY_CTRL_F5:
        return("CTRL_F5");
        break;
    case KEY_CTRL_F6:
        return("CTRL_F6");
        break;
    case KEY_CTRL_F7:
        return("CTRL_F7");
        break;
    case KEY_CTRL_F8:
        return("CTRL_F8");
        break;
    case KEY_CTRL_F9:
        return("CTRL_F9");
        break;
    case KEY_CTRL_F10:
        return("CTRL_F10");
        break;
    case KEY_CTRL_F11:
        return("CTRL_F11");
        break;
    case KEY_CTRL_F12:
        return("CTRL_F12");
        break;
    case KEY_CTRL_F13:
        return("CTRL_F13");
        break;
    case KEY_CTRL_F14:
        return("CTRL_F14");
        break;

    case KEY_UP:
        return("UP");
        break;
    case KEY_DOWN:
        return("DOWN");
        break;
    case KEY_LEFT:
        return("LEFT");
        break;
    case KEY_RIGHT:
        return("RIGHT");
        break;
    case KEY_HOME:
        return("HOME");
        break;
    case KEY_END:
        return("END");
        break;
    case KEY_PAUSE:
        return("PAUSE");
        break;
    case KEY_FOCUS_IN:
        return("FOCUS_IN");
        break;
    case KEY_FOCUS_OUT:
        return("FOCUS_OUT");
        break;
    case KEY_INSERT:
        return("INSERT");
        break;
    case KEY_DELETE:
        return("DELETE");
        break;
    case KEY_PGUP:
        return("PGUP");
        break;
    case KEY_PGDN:
        return("PGDN");
        break;
    case KEY_WIN:
        return("WIN");
        break;
    case KEY_MENU:
        return("MENU");
        break;

    case KEY_SHIFT_UP:
        return("SHIFT_UP");
        break;
    case KEY_SHIFT_DOWN:
        return("SHIFT_DOWN");
        break;
    case KEY_SHIFT_LEFT:
        return("SHIFT_LEFT");
        break;
    case KEY_SHIFT_RIGHT:
        return("SHIFT_RIGHT");
        break;
    case KEY_SHIFT_HOME:
        return("SHIFT_HOME");
        break;
    case KEY_SHIFT_END:
        return("SHIFT_END");
        break;
    case KEY_SHIFT_PAUSE:
        return("SHIFT_PAUSE");
        break;
    case KEY_SHIFT_FOCUS_IN:
        return("SHIFT_FOCUS_IN");
        break;
    case KEY_SHIFT_FOCUS_OUT:
        return("SHIFT_FOCUS_OUT");
        break;
    case KEY_SHIFT_INSERT:
        return("SHIFT_INSERT");
        break;
    case KEY_SHIFT_DELETE:
        return("SHIFT_DELETE");
        break;
    case KEY_SHIFT_PGUP:
        return("SHIFT_PGUP");
        break;
    case KEY_SHIFT_PGDN:
        return("SHIFT_PGDN");
        break;
    case KEY_SHIFT_WIN:
        return("SHIFT_WIN");
        break;
    case KEY_SHIFT_MENU:
        return("SHIFT_MENU");
        break;

    case KEY_CTRL_UP:
        return("CTRL_UP");
        break;
    case KEY_CTRL_DOWN:
        return("CTRL_DOWN");
        break;
    case KEY_CTRL_LEFT:
        return("CTRL_LEFT");
        break;
    case KEY_CTRL_RIGHT:
        return("CTRL_RIGHT");
        break;
    case KEY_CTRL_HOME:
        return("CTRL_HOME");
        break;
    case KEY_CTRL_END:
        return("CTRL_END");
        break;
    case KEY_CTRL_PAUSE:
        return("CTRL_PAUSE");
        break;
    case KEY_CTRL_FOCUS_IN:
        return("CTRL_FOCUS_IN");
        break;
    case KEY_CTRL_FOCUS_OUT:
        return("CTRL_FOCUS_OUT");
        break;
    case KEY_CTRL_INSERT:
        return("CTRL_INSERT");
        break;
    case KEY_CTRL_DELETE:
        return("CTRL_DELETE");
        break;
    case KEY_CTRL_PGUP:
        return("CTRL_PGUP");
        break;
    case KEY_CTRL_PGDN:
        return("CTRL_PGDN");
        break;
    case KEY_CTRL_WIN:
        return("CTRL_WIN");
        break;
    case KEY_CTRL_MENU:
        return("CTRL_MENU");
        break;
    }

    KeyStr[0]='?';
    KeyStr[1]='\0';
    if ((key > 31) && (key < 127)) KeyStr[0]=key & 0xFF;
    return(KeyStr);
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
        if (Arg1 < 0) Tempstr=FormatStr(Tempstr,"\x1b[%dT",0-Arg1);
        else Tempstr=FormatStr(Tempstr,"\x1b[%dS",Arg1);
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


char *TerminalFormatStr(char *RetStr, const char *Str)
{
    const char *ptr, *end;
    long len, val;

    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (*ptr=='~')
        {
            ptr++;
            if (*ptr=='\0') break;
            switch (*ptr)
            {
            case '~':
                RetStr=AddCharToStr(RetStr, *ptr);
                break;
            case 'r':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_RED, 0);
                break;
            case 'R':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_RED);
                break;
            case 'g':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_GREEN, 0);
                break;
            case 'G':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_GREEN);
                break;
            case 'b':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_BLUE, 0);
                break;
            case 'B':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_BLUE);
                break;
            case 'n':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_BLACK, 0);
                break;
            case 'N':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_BLACK);
                break;
            case 'w':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_WHITE, 0);
                break;
            case 'W':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_WHITE);
                break;
            case 'y':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_YELLOW, 0);
                break;
            case 'Y':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_YELLOW);
                break;
            case 'm':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_MAGENTA, 0);
                break;
            case 'M':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, 0, ANSI_MAGENTA);
                break;
            case 'c':
                RetStr=TerminalCommandStr(RetStr, TERM_COLOR, ANSI_CYAN, 0);
                break;
            case 'e':
                RetStr=TerminalCommandStr(RetStr, TERM_TEXT, ANSI_BOLD, 0);
                break;
            case 'i':
                RetStr=TerminalCommandStr(RetStr, TERM_TEXT, ANSI_INVERSE, 0);
                break;
            case 'u':
                RetStr=TerminalCommandStr(RetStr, TERM_TEXT, ANSI_UNDER, 0);
                break;
            case 'I':
                RetStr=TerminalCommandStr(RetStr, TERM_TEXT, ANSI_INVERSE, 0);
                break;
            case '<':
                RetStr=TerminalCommandStr(RetStr, TERM_CLEAR_STARTLINE, 0, 0);
                break;
            case '>':
                RetStr=TerminalCommandStr(RetStr, TERM_CLEAR_ENDLINE, 0, 0);
                break;
            case 'U':
                ptr++;
                if (! strntol(&ptr, 4, 16, &val)) break;
                ptr--; //because of ptr++ on next loop
                RetStr=TerminalCommandStr(RetStr, TERM_UNICODE, val, 0);
                break;
            case '0':
                RetStr=TerminalCommandStr(RetStr, TERM_NORM, 0, 0);
                break;
            }
        }
        //if top bit is set then this is unicode
        else if (*ptr & 128)
        {
            val=UnicodeDecode(&ptr);
            if (val > 0) RetStr=TerminalCommandStr(RetStr, TERM_UNICODE, val, 0);
        }
        else RetStr=AddCharToStr(RetStr, *ptr);
    }

    return(RetStr);
}



int TerminalCommand(int Cmd, int Arg1, int Arg2, STREAM *S)
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
        else write(1,Tempstr,StrLen(Tempstr));
    }

    DestroyString(Tempstr);
}




void TerminalPutStr(const char *Str, STREAM *S)
{
    char *Tempstr=NULL;

    Tempstr=TerminalFormatStr(Tempstr, Str);
    if (S) STREAMWriteLine(Tempstr, S);
    else write(1,Tempstr,StrLen(Tempstr));

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
            return(KEY_HOME);
            break;
        case 2:
            return(KEY_INSERT);
            break;
        case 3:
            return(KEY_DELETE);
            break;
        case 4:
            return(KEY_END);
            break;
        case 5:
            return(KEY_PGUP);
            break;
        case 6:
            return(KEY_PGDN);
            break;

        case 11:
            return(KEY_F1);
            break;
        case 12:
            return(KEY_F2);
            break;
        case 13:
            return(KEY_F3);
            break;
        case 14:
            return(KEY_F4);
            break;
        case 15:
            return(KEY_F5);
            break;
        case 16:
            return(KEY_F6);
            break;
        case 17:
            return(KEY_F6);
            break;
        case 18:
            return(KEY_F7);
            break;
        case 19:
            return(KEY_F8);
            break;
        case 20:
            return(KEY_F9);
            break;
        case 21:
            return(KEY_F10);
            break;
        case 23:
            return(KEY_F11);
            break;
        case 24:
            return(KEY_F12);
            break;
        case 25:
            return(KEY_F13);
            break;
        case 26:
            return(KEY_F14);
            break;
        case 28:
            return(KEY_WIN);
            break;
        case 29:
            return(KEY_MENU);
            break;
        }
        break;


    //shift
    case '$':
        switch (val)
        {
        case 1:
            return(KEY_SHIFT_HOME);
            break;
        case 2:
            return(KEY_SHIFT_INSERT);
            break;
        case 3:
            return(KEY_SHIFT_DELETE);
            break;
        case 4:
            return(KEY_SHIFT_END);
            break;
        case 5:
            return(KEY_SHIFT_PGUP);
            break;
        case 6:
            return(KEY_SHIFT_PGDN);
            break;

        case 11:
            return(KEY_SHIFT_F1);
            break;
        case 12:
            return(KEY_SHIFT_F2);
            break;
        case 13:
            return(KEY_SHIFT_F3);
            break;
        case 14:
            return(KEY_SHIFT_F4);
            break;
        case 15:
            return(KEY_SHIFT_F5);
            break;
        case 16:
            return(KEY_SHIFT_F6);
            break;
        case 17:
            return(KEY_SHIFT_F6);
            break;
        case 18:
            return(KEY_SHIFT_F7);
            break;
        case 19:
            return(KEY_SHIFT_F8);
            break;
        case 20:
            return(KEY_SHIFT_F9);
            break;
        case 21:
            return(KEY_SHIFT_F10);
            break;
        case 23:
            return(KEY_SHIFT_F11);
            break;
        case 24:
            return(KEY_SHIFT_F12);
            break;

        case 25:
            return(KEY_SHIFT_F13);
            break;
        case 26:
            return(KEY_SHIFT_F14);
            break;
        case 28:
            return(KEY_SHIFT_WIN);
            break;
        case 29:
            return(KEY_SHIFT_MENU);
            break;
        }
        break;

    //ctrl
    case '^':
        switch (val)
        {
        case 1:
            return(KEY_CTRL_HOME);
            break;
        case 2:
            return(KEY_CTRL_INSERT);
            break;
        case 3:
            return(KEY_CTRL_DELETE);
            break;
        case 4:
            return(KEY_CTRL_END);
            break;
        case 5:
            return(KEY_CTRL_PGUP);
            break;
        case 6:
            return(KEY_CTRL_PGDN);
            break;
        case 11:
            return(KEY_CTRL_F1);
            break;
        case 12:
            return(KEY_CTRL_F2);
            break;
        case 13:
            return(KEY_CTRL_F3);
            break;
        case 14:
            return(KEY_CTRL_F4);
            break;
        case 15:
            return(KEY_CTRL_F5);
            break;
        case 16:
            return(KEY_CTRL_F6);
            break;
        case 17:
            return(KEY_CTRL_F6);
            break;
        case 18:
            return(KEY_CTRL_F7);
            break;
        case 19:
            return(KEY_CTRL_F8);
            break;
        case 20:
            return(KEY_CTRL_F9);
            break;
        case 21:
            return(KEY_CTRL_F10);
            break;
        case 23:
            return(KEY_CTRL_F11);
            break;
        case 24:
            return(KEY_CTRL_F12);
            break;

        case 25:
            return(KEY_CTRL_F13);
            break;
        case 26:
            return(KEY_CTRL_F14);
            break;
        case 28:
            return(KEY_CTRL_WIN);
            break;
        case 29:
            return(KEY_CTRL_MENU);
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
            case 'A':
                return(KEY_UP);
                break;
            case 'B':
                return(KEY_DOWN);
                break;
            case 'C':
                return(KEY_RIGHT);
                break;
            case 'D':
                return(KEY_LEFT);
                break;
            case 'H':
                return(KEY_HOME);
                break;
            case 'P':
                return(KEY_PAUSE);
                break;
            case 'Z':
                return(KEY_END);
                break;
            case 'I':
                return(KEY_FOCUS_IN);
                break;
            case 'O':
                return(KEY_FOCUS_OUT);
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
                return(KEY_CTRL_UP);
                break;
            case 'B':
                return(KEY_CTRL_DOWN);
                break;
            case 'c':
                return(KEY_CTRL_LEFT);
                break;
            case 'd':
                return(KEY_CTRL_RIGHT);
                break;
            case 'P':
                return(KEY_F1);
                break;
            case 'Q':
                return(KEY_F2);
                break;
            case 'R':
                return(KEY_F3);
                break;
            case 'S':
                return(KEY_F4);
                break;
            case 't':
                return(KEY_F5);
                break;
            case 'u':
                return(KEY_F6);
                break;
            case 'v':
                return(KEY_F7);
                break;
            case 'l':
                return(KEY_F8);
                break;
            case 'w':
                return(KEY_F9);
                break;
            case 'x':
                return(KEY_F10);
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
    KEY_CALLBACK_FUNC Func;

    inchar=TerminalReadChar(S);
    while (inchar != EOF)
    {
        Func=STREAMGetItem(S, "KeyCallbackFunc");
        if (Func) Func(S, inchar);
        outchar=inchar & 0xFF;

        switch (inchar)
        {
        case '\n':
        case '\r':
            break;

        //'backspace' key on keyboard will send the 'del' character in some cases!
        case 0x7f: //this is 'del'
        case 0x08: //this is backspace
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

        if (! (Flags & TERM_HIDETEXT))
        {
            STREAMWriteBytes(S, &outchar,1);
            STREAMFlush(S);
        }

        inchar=TerminalReadChar(S);
    }


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
        if (top > 0) STREAMSetValue(S, "Terminal:top","1");
        TerminalCommand(TERM_SCROLL_REGION, top+2, rows-(top+bottom), S);
        TerminalCommand(TERM_CURSOR_MOVE, 0, top+2, S);
    }
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
    if (atoi(STREAMGetValue(S, "Terminal:top")) > 0) STREAMWriteLine("\x1b[m\x1b[r", S);
    if (isatty(S->in_fd)) TTYReset(S->in_fd);
}


void TerminalSetKeyCallback(STREAM *Term, KEY_CALLBACK_FUNC Func)
{
    STREAMSetItem(Term, "KeyCallbackFunc", Func);
}


void TerminalBarSetConfig(TERMBAR *TB, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

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

    TerminalInternalConfig(Config, &(TB->ForeColor), &(TB->BackColor), &(TB->Flags));

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
    KEY_CALLBACK_FUNC Func;
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
        case KEY_LEFT:
            Curr=ListGetPrev(Items->Side);
            if (Curr) Items->Side=Curr;
            break;

        case '>':
        case KEY_RIGHT:
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


