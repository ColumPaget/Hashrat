#include "TerminalProgress.h"

TERMPROGRESS *TerminalProgressCreate(STREAM *Term, const char *Config)
{
    TERMPROGRESS *TP;

    TP=TerminalWidgetCreate(Term, "type=progress");
    TerminalWidgetParseConfig(TP, Config);
    return(TP);
}


void TerminalProgressDraw(TERMPROGRESS *TP, float Fract, const char *Info)
{
    char *Tempstr=NULL, *Bar=NULL, *Remain=NULL;
    const char *ptr;
    int len, wide;

    wide=TP->wid;
    if (TP->Term)
    {
        if (wide < 0) wide=atoi(STREAMGetValue(TP->Term, "Terminal:cols")) + TP->wid - TP->x - TerminalStrLen(TP->Text) - TerminalStrLen(Info);
    }

    if (TP->Flags & TERMMENU_POSITIONED) TerminalCommand(TERM_CURSOR_MOVE, TP->x, TP->y, TP->Term);
    else Tempstr=CopyStr(Tempstr, "\r");

    if (StrValid(TP->Text)) Tempstr=CatStr(Tempstr, TP->Text);

    Tempstr=MCatStr(Tempstr, TP->CursorLeft, TP->SelectedAttribs, NULL);

    len=(int) (wide * Fract + 0.5);
    ptr=GetVar(TP->Options, "progress");
    if (! StrValid(ptr)) ptr=" ";
    Bar=PadStr(Bar, *ptr, len);
    Tempstr=CatStr(Tempstr, Bar);

    if (StrValid(TP->Attribs)) Tempstr=CatStr(Tempstr, TP->Attribs);
    else if (StrValid(TP->SelectedAttribs)) Tempstr=CatStr(Tempstr, "~0");

    ptr=GetVar(TP->Options, "remain");
    if (! StrValid(ptr)) ptr=" ";
    Remain=PadStr(Remain, *ptr, wide - len);
    Tempstr=CatStr(Tempstr, Remain);

    if (StrValid(TP->Attribs)) Tempstr=CatStr(Tempstr, "~0");
    Tempstr=CatStr(Tempstr, TP->CursorRight);
    if (StrValid(Info)) Tempstr=MCatStr(Tempstr, Info, "~>", NULL);

    TerminalPutStr(Tempstr, TP->Term);
    STREAMFlush(TP->Term);

    Destroy(Tempstr);
    Destroy(Remain);
    Destroy(Bar);
}




float TerminalProgressUpdate(TERMPROGRESS *TP, int Value, int Max, const char *Info)
{
    float Fract;

    Fract=((float) Value) / ((float) Max);

    TerminalProgressDraw(TP, Fract, Info);
    return(Fract);
}


