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
    char *Tempstr=NULL, *LPad=NULL, *RPad=NULL;
    ListNode *Curr;
    int choice_pos=0, len, pos;

    LPad=PadStr(LPad, ' ', TerminalStrLen(Chooser->CursorLeft));
    RPad=PadStr(RPad, ' ', TerminalStrLen(Chooser->CursorRight));

    Curr=ListGetNext(Chooser->Options);
    while (Curr)
    {
        if (Chooser->Options->Side==Curr)
        {
            Tempstr=MCatStr(Tempstr, Chooser->CursorAttribs, Chooser->CursorLeft, Curr->Tag, Chooser->CursorRight, "~0", NULL);
            choice_pos=StrLen(Tempstr);
        }
        else Tempstr=MCatStr(Tempstr, LPad, Curr->Tag, RPad, NULL);

        Curr=ListGetNext(Curr);
    }

    //choice pos is the position of the end of the selected option/choice
    //if this is outside of the 'window' of the chooser, we'll have to use
    //memmove to move the option into the 'window' of the chooser
    choice_pos += StrLen(Chooser->Text);
    if (Chooser->wid > 0)
    {
        if (choice_pos > Chooser->wid)
        {
            len=StrLen(Tempstr);
            pos=choice_pos - Chooser->wid;
            len -= pos;
            memmove(Tempstr, Tempstr + pos, len+1);
        }

    }


    //from here on we use LPad for the 'final' string that's printed out

    //either move to position or use '\r' to go to start of the line
    LPad=CopyStr(LPad, "");
    if (Chooser->Flags & TERMMENU_POSITIONED) TerminalCommand(TERM_CURSOR_MOVE, Chooser->x, Chooser->y, Chooser->Term);
    else LPad=CopyStr(LPad, "\r");

    //Add the prompt
    if (StrValid(Chooser->Text)) LPad=CatStr(LPad, Chooser->Text);
    LPad=CatStr(LPad, Tempstr);

    if (Chooser->wid > 0)
    {
        len=TerminalStrLen(LPad);
        LPad=PadStr(LPad, ' ', Chooser->wid);
        StrTrunc(LPad, Chooser->wid);
    }

    TerminalPutStr(LPad, Chooser->Term);
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



