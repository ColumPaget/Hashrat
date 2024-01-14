#include "TerminalChoice.h"
#include "TerminalTheme.h"

TERMCHOICE *TerminalChoiceCreate(STREAM *Term, const char *Config)
{
    TERMCHOICE *TC;

    TC=TerminalWidgetCreate(Term, "cursor_left=< cursor_right=>");
    TerminalThemeApply(TC, "Choice");
    TerminalWidgetParseConfig(TC, Config);
    return(TC);
}

void TerminalChoiceDraw(TERMCHOICE *Chooser)
{
    ListNode *Curr;
    char *Tempstr=NULL, *LPad=NULL, *RPad=NULL;

    if (Chooser->Flags & TERMMENU_POSITIONED) TerminalCommand(TERM_CURSOR_MOVE, Chooser->x, Chooser->y, Chooser->Term);
    else Tempstr=CopyStr(Tempstr, "\r");

    if (StrValid(Chooser->Text)) Tempstr=CatStr(Tempstr, Chooser->Text);

    LPad=PadStr(LPad, ' ', TerminalStrLen(Chooser->CursorLeft));
    RPad=PadStr(RPad, ' ', TerminalStrLen(Chooser->CursorRight));

    Curr=ListGetNext(Chooser->Options);
    while (Curr)
    {
        if (Chooser->Options->Side==Curr)
        {
            Tempstr=MCatStr(Tempstr, Chooser->CursorAttribs, Chooser->CursorLeft, Curr->Tag, Chooser->CursorRight, "~0", NULL);
        }
        else Tempstr=MCatStr(Tempstr, LPad, Curr->Tag, RPad, NULL);

        Curr=ListGetNext(Curr);
    }

    TerminalPutStr(Tempstr, Chooser->Term);
    STREAMFlush(Chooser->Term);

    Destroy(Tempstr);
    Destroy(LPad);
    Destroy(RPad);
}


char *TerminalChoiceOnKey(char *RetStr, TERMCHOICE *Chooser, int key)
{
    ListNode *Curr;

    RetStr=CopyStr(RetStr, "");

    switch (key)
    {
    case EOF:
        Destroy(RetStr);
        return(NULL);
        break;

    case '<':
    case TKEY_LEFT:
        Curr=ListGetPrev(Chooser->Options->Side);
        if (Curr) Chooser->Options->Side=Curr;
        break;

    case '>':
    case TKEY_RIGHT:
        Curr=ListGetNext(Chooser->Options->Side);
        if (Curr) Chooser->Options->Side=Curr;
        break;

    case '\r':
    case '\n':
        RetStr=CopyStr(RetStr, Chooser->Options->Side->Tag);
        break;
    }

    TerminalChoiceDraw(Chooser);

    return(RetStr);
}


char *TerminalChoiceProcess(char *RetStr, TERMCHOICE *Chooser)
{
    int inchar;

    TerminalChoiceDraw(Chooser);
    inchar=TerminalReadChar(Chooser->Term);
    while (TRUE)
    {
        RetStr=TerminalChoiceOnKey(RetStr, Chooser, inchar);

        //there's two scenarios where we're 'done' with the menubar.
        //The first is if the user selects an item, in which case 'RetStr'
        //will have a length (and StrValid will return true).
        //The other case is if the user presses 'escape', in which case
        //RetStr will be NULL
        if ((! RetStr) || StrValid(RetStr)) break;
        inchar=TerminalReadChar(Chooser->Term);
    }

    return(RetStr);
}


char *TerminalChoice(char *RetStr, STREAM *Term, const char *Config)
{
    TERMCHOICE *Chooser;

    Chooser=TerminalChoiceCreate(Term, Config);
    RetStr=TerminalChoiceProcess(RetStr, Chooser);
    TerminalChoiceDestroy(Chooser);

    return(RetStr);
}



