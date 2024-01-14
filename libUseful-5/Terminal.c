#include "Terminal.h"
#include "Unicode.h"
#include "Pty.h"
#include "String.h"
#include <sys/ioctl.h>
#include <termios.h>
#include "TerminalKeys.h" //for TerminalBarsInit
#include "TerminalBar.h" //for TerminalBarsInit
#include "TerminalTheme.h"
#include "Encodings.h"

static const char *ANSIColorStrings[]= {"none","black","red","green","yellow","blue","magenta","cyan","white",NULL};



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
        //all unicode has the top 2 bits set in the first byte
        //even if it has more than these set it will at least
        //have the first two
        if (**ptr & 192)
        {
            //if starts with 11110 then there will be 3 unicode bytes after
            if ((**ptr & 248)==240) ptr_incr(ptr, 3); //
            //if starts with 1110 then there will be 2 unicode bytes after
            else if ((**ptr & 240)==224) ptr_incr(ptr, 2); //
            //if starts with 110 then there will be 1 unicode bytes after
            else if ((**ptr & 224)==192) ptr_incr(ptr, 1); //
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


char *TerminalStripControlSequences(char *RetStr, const char *Str)
{
    const char *ptr;

    RetStr=CopyStr(RetStr, "");

    for (ptr=Str; *ptr !='\0'; ptr++)
    {
        if (*ptr == ESCAPE)
        {
            ptr++;
            switch (*ptr)
            {
            case '[':
                ptr++;
                if (*ptr=='0') while ((*ptr != '\0') && (*ptr != 'm')) ptr++;
                break;
            }
        }
        else
        {
            RetStr=AddCharToStr(RetStr, *ptr);
        }

    }

    return(RetStr);
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





void TerminalGeometry(STREAM *S, int *wid, int *len)
{
    struct winsize w;
    const char *ptr;
    int val=0;

    memset(&w,0,sizeof(struct winsize));
    ioctl(S->out_fd, TIOCGWINSZ, &w);

    ptr=STREAMGetValue(S, "Terminal:force_cols");
    if (StrValid(ptr))
    {
        val=atoi(ptr);
        if (val > 0) w.ws_col=val;
    }

    ptr=STREAMGetValue(S, "Terminal:force_rows");
    if (StrValid(ptr))
    {
        val=atoi(ptr);
        if (val > 0) w.ws_row=val;
    }

    *wid=w.ws_col;
    *len=w.ws_row;
}


void TerminalInternalConfig(const char *Config, int *ForeColor, int *BackColor, int *Flags, int *width, int *height)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    if (width) *width=TERM_AUTODETECT;
    if (height) *height=TERM_AUTODETECT;

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
            else if (strcmp(Name,"focus")==0) *Flags |= TERM_FOCUS_EVENTS;
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
            if (strcasecmp(Name,"hidecursor") ==0) *Flags |= TERM_HIDECURSOR;
            if (strcasecmp(Name,"hidetext")==0) *Flags |= TERM_HIDETEXT;
            if (height && (strcasecmp(Name,"height")==0)) *height=atoi(Value);
            break;

        case 'm':
        case 'M':
            if (strcasecmp(Name,"mouse")==0) *Flags=TERM_MOUSE;
            break;

	case 'n':
            if (strcasecmp(Name,"nocolor")==0) *Flags=TERM_NOCOLOR;
	    break;

        case 'r':
        case 'R':
            if (strcasecmp(Name, "rawkeys") ==0) *Flags |= TERM_RAWKEYS;
            break;

        case 's':
        case 'S':
            if (strcasecmp(Name,"stars")==0) *Flags |= TERM_SHOWSTARS;
            if (strcasecmp(Name,"stars+1")==0) *Flags |= TERM_SHOWTEXTSTARS;
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
            if (width && (strcasecmp(Name,"width")==0) ) *width=atoi(Value);
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

    //this could be a longer string, and so could be in cache
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


const char *TerminalParseColor(const char *Str, int *Fg, int *Bg)
{
    const char *ptr;
    int offset=0;

    ptr=Str;
    if (*ptr=='+')
    {
        offset=ANSI_DARKGREY;
        ptr++;
    }

    switch (*ptr)
    {
    case 'r':
        *Fg=ANSI_RED + offset;
        break;
    case 'R':
        *Bg=ANSI_RED + offset;
        break;
    case 'g':
        *Fg=ANSI_GREEN + offset;
        break;
    case 'G':
        *Bg=ANSI_GREEN + offset;
        break;
    case 'b':
        *Fg=ANSI_BLUE + offset;
        break;
    case 'B':
        *Bg=ANSI_BLUE + offset;
        break;
    case 'n':
        *Fg=ANSI_BLACK + offset;
        break;
    case 'N':
        *Bg=ANSI_BLACK + offset;
        break;
    case 'w':
        *Fg=ANSI_WHITE + offset;
        break;
    case 'W':
        *Fg=ANSI_WHITE + offset;
        break;
    case 'y':
        *Fg=ANSI_YELLOW + offset;
        break;
    case 'Y':
        *Bg=ANSI_YELLOW + offset;
        break;
    case 'm':
        *Fg=ANSI_MAGENTA + offset;
        break;
    case 'M':
        *Bg=ANSI_MAGENTA + offset;
        break;
    case 'c':
        *Fg=ANSI_CYAN + offset;
        break;
    case 'C':
        *Bg=ANSI_CYAN + offset;
        break;
    }


    return(ptr);
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
    int Fg, Bg;

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
                Fg=0;
                Bg=0;
                ptr=TerminalParseColor(ptr, &Fg, &Bg);
		//if we have a Term object, then TERM_STREAM_NOCOLOR must not be set
                if ((! Term) || (! (Term->Flags & TERM_STREAM_NOCOLOR))) *RetStr=TerminalCommandStr(*RetStr, TERM_COLOR, Fg, Bg);
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
        else
        {
            *RetStr=AddCharToStr(*RetStr, *ptr);
        }
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
    int result;

    if (Char <= 0x7f)
    {
        towrite=Char;
        if (S) STREAMWriteChar(S, towrite);
        else result=write(1, &towrite, 1);
    }
    else
    {
        Tempstr=UnicodeStr(Tempstr, Char);

        //do not use StrLenFromCache here, as string will be short
        if (S) STREAMWriteLine(Tempstr, S);
        else result=write(1,Tempstr,StrLen(Tempstr));
    }

    Destroy(Tempstr);
}




void TerminalPutStr(const char *Str, STREAM *S)
{
    char *Tempstr=NULL;
    int len, result;

    Tempstr=TerminalFormatStr(Tempstr, Str, S);
    //this could be a long-ish string, so we do use StrLenFromCache
    len=StrLenFromCache(Tempstr);
    if (S) STREAMWriteBytes(S, Tempstr, len);
    else result=write(1,Tempstr,len);

    Destroy(Tempstr);
}


void XtermSetTerminalSize(STREAM *Term, int wide, int high)
{
    char *Tempstr=NULL;

    Tempstr=FormatStr(Tempstr, "\x1b[8;%d;%dt", high, wide);
    STREAMWriteLine(Tempstr, Term);
    STREAMFlush(Term);
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

void XtermStringBase64Command(const char *Prefix, const char *Str, const char *Postfix, STREAM *S)
{
    char *Tempstr=NULL;

    Tempstr=EncodeBytes(Tempstr, Str, StrLen(Str), ENCODE_BASE64);
    XtermStringCommand(Prefix, Tempstr, Postfix, S);
    Destroy(Tempstr);
}



char *XtermGetClipboard(char *RetStr, STREAM *S)
{
    int inchar;

    RetStr=CopyStr(RetStr, "");
    XtermRequestClipboard(S);
    inchar=TerminalReadChar(S);
    while (inchar > 0)
    {
        if (inchar == XTERM_CLIPBOARD)
        {
            RetStr=CopyStr(RetStr, XtermReadClipboard(S));
            break;
        }
        inchar=TerminalReadChar(S);
    }

    return(RetStr);
}

char *XtermGetSelection(char *RetStr, STREAM *S)
{
    int inchar;

    RetStr=CopyStr(RetStr, "");
    XtermRequestSelection(S);
    inchar=TerminalReadChar(S);
    while (inchar > 0)
    {
        if (inchar == XTERM_SELECTION)
        {
            RetStr=CopyStr(RetStr, XtermReadSelection(S));
            break;
        }
        inchar=TerminalReadChar(S);
    }

    return(RetStr);
}


void XtermSetDefaultColors(STREAM *S, const char *Str)
{
    const char *ptr;
    char *Output=NULL;
    int Fg=0, Bg=0;

    for (ptr=Str; *ptr != '\0'; ptr++)
    {
        if (*ptr == '~')
        {
            ptr++;
            if (*ptr==0) break;
            ptr=TerminalParseColor(ptr, &Fg, &Bg);
        }
    }

    if (Fg > 0)
    {
        Output=FormatStr(Output,"\x1b]10;%d\007",Fg);
        STREAMWriteLine(Output, S);
    }

    if (Bg > 0)
    {
        Output=FormatStr(Output,"\x1b]11;%d\007",Bg);
        STREAMWriteLine(Output, S);
    }

    Destroy(Output);
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
            return(TerminalReadCSISeq(S));
            break;

        case ']':
            return(TerminalReadOSCSeq(S));
            break;


        case 'O':
            return(TerminalReadSSCSeq(S));
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
    if (strcasecmp(Config, "textstars")==0) return(TERM_SHOWTEXTSTARS);
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


    if (Flags & TERM_NOCOLOR) S->Flags |= TERM_STREAM_NOCOLOR;
    if (Flags & TERM_HIDECURSOR) TerminalCursorHide(S);

    if (isatty(S->in_fd))
    {
        if (Flags & TERM_SAVEATTRIBS) ttyflags=TTYFLAG_SAVE;
        if (Flags & TERM_RAWKEYS) ttyflags |= TTYFLAG_IN_CRLF | TTYFLAG_OUT_CRLF;
        if (ttyflags) TTYConfig(S->in_fd, 0, ttyflags);
    }

    XtermSetDefaultColors(S, TerminalThemeGet("Terminal:Attribs"));
    TerminalBarsInit(S);
    if (Flags & TERM_WHEELMOUSE) STREAMWriteLine("\x1b[?1000h", S);
    else if (Flags & TERM_MOUSE) STREAMWriteLine("\x1b[?9h", S);

    if (Flags & TERM_FOCUS_EVENTS) STREAMWriteLine("\x1b[?1004h", S);

    Destroy(Tempstr);
    return(TRUE);
}


void TerminalSetup(STREAM *S, const char *Config)
{
    int Flags=0, FColor, BColor, width, height;
    char *Tempstr=NULL;

    TerminalInternalConfig(Config, &FColor, &BColor, &Flags, &width, &height);

    if (width > TERM_AUTODETECT)
    {
        Tempstr=FormatStr(Tempstr, "%d", width);
        STREAMSetValue(S, "Terminal:force_cols", Tempstr);
    }

    if (height > TERM_AUTODETECT)
    {
        Tempstr=FormatStr(Tempstr, "%d", height);
        STREAMSetValue(S, "Terminal:force_rows", Tempstr);
    }

    TerminalInit(S, Flags);

    Destroy(Tempstr);
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
    static TMouseEvent MouseEvent;
    char *ptr;

    ptr=(char *) STREAMGetValue(Term, "LU_MouseEvent");
    if (! StrValid(ptr)) return NULL;

    MouseEvent.button=strtol(ptr, &ptr, 10);
    if (*ptr==':') ptr++;
    MouseEvent.x=strtol(ptr, &ptr, 10);
    if (*ptr==':') ptr++;
    MouseEvent.y=strtol(ptr, &ptr, 10);
    if (*ptr==':') ptr++;

    return(&MouseEvent);
}
