#include "TerminalProgress.h"
#include "TerminalTheme.h"

TERMPROGRESS *TerminalProgressCreate(STREAM *Term, const char *Config)
{
    TERMPROGRESS *TP;

    TP=TerminalWidgetCreate(Term, "progress=* remain=' ' left-contain=' [' right-contain='] '");
    TerminalThemeApply(TP, "Progress");
    TerminalWidgetParseConfig(TP, Config);
    return(TP);
}


void TerminalProgressDraw(TERMPROGRESS *TP, float Fract, const char *Info)
{
    char *Tempstr=NULL, *Bar=NULL, *Remain=NULL;
    int i, len;

    if (TP->Flags & TERMMENU_POSITIONED) TerminalCommand(TERM_CURSOR_MOVE, TP->x, TP->y, TP->Term);
    else Tempstr=CopyStr(Tempstr, "\r");

    if (StrValid(TP->Text)) Tempstr=CatStr(Tempstr, TP->Text);

    Tempstr=MCatStr(Tempstr, GetVar(TP->Options, "LeftContainer"), TP->SelectedAttribs, NULL);

    len=(int) (TP->wid * Fract + 0.5);
    Bar=PadStr(Bar, *TP->CursorLeft, len);

    Tempstr=CatStr(Tempstr, Bar);

    if (StrValid(TP->Attribs)) Tempstr=CatStr(Tempstr, TP->Attribs);
    else if (StrValid(TP->SelectedAttribs)) Tempstr=CatStr(Tempstr, "~0");

    Remain=PadStr(Remain, *TP->CursorRight, TP->wid - len);
    Tempstr=CatStr(Tempstr, Remain);

    if (StrValid(TP->Attribs)) Tempstr=CatStr(Tempstr, "~0");
    Tempstr=CatStr(Tempstr, GetVar(TP->Options, "RightContainer"));
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


