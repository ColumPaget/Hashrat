#include "TerminalWidget.h"
#include "TerminalTheme.h"
#include "Terminal.h"

void TerminalWidgetSetOptions(TERMWIDGET *TW, const char *Choices)
{
    const char *ptr;
    ListNode *Node;
    char *Token=NULL;

    ListClear(TW->Options, NULL);
    ptr=GetToken(Choices, ",", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        ListAddNamedItem(TW->Options, Token, NULL);
        ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
    }

    Node=ListGetNext(TW->Options);
    TW->Options->Side=Node;

    Destroy(Token);
}


void TerminalWidgetParseConfig(TERMWIDGET *TW, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    if (! StrValid(Config)) return;

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        strrep(Name, '-', '_');
        switch (*Name)
        {
        case 'x':
            TW->x=atoi(Value);
            TW->Flags |= TERMWIDGET_POSITIONED;
            break;

        case 'y':
            TW->y=atoi(Value);
            TW->Flags |= TERMWIDGET_POSITIONED;
            break;

        case 'a':
            if (strcasecmp(Name, "attribs")==0) TW->Attribs=CopyStr(TW->Attribs, Value);
            break;

        case 'c':
            if (strcasecmp(Name, "cursor")==0) TW->CursorLeft=CopyStr(TW->CursorLeft, Value);
            else if (strcasecmp(Name, "cursor_left")==0) TW->CursorLeft=CopyStr(TW->CursorLeft, Value);
            else if (strcasecmp(Name, "cursor_right")==0) TW->CursorRight=CopyStr(TW->CursorRight, Value);
            else if (strcasecmp(Name, "cursor_attribs")==0) TW->CursorAttribs=CopyStr(TW->CursorAttribs, Value);
            else if (strcasecmp(Name, "choices")==0) TerminalWidgetSetOptions(TW, Value);
            break;

        case 'l':
            if (strcasecmp(Name, "left_cursor")==0) TW->CursorLeft=CopyStr(TW->CursorLeft, Value);
            if (strcasecmp(Name, "left_contain")==0) TW->CursorLeft=CopyStr(TW->CursorLeft, Value);
            break;

        case 'h':
            if (strcasecmp(Name, "height")==0) TW->high=atoi(Value);
            break;

        case 'o':
            if (strcasecmp(Name, "options")==0) TerminalWidgetSetOptions(TW, Value);
            break;

        case 'p':
            if (strcasecmp(Name, "prompt")==0) TW->Text=CopyStr(TW->Text, Value);
            if (strcasecmp(Name, "progress")==0) SetVar(TW->Options, "progress", Value);
            if (strcasecmp(Name, "progress_attribs")==0) TW->SelectedAttribs=CopyStr(TW->SelectedAttribs, Value);
            break;

        case 'r':
            if (strcasecmp(Name, "right_cursor")==0) TW->CursorRight=CopyStr(TW->CursorRight, Value);
            if (strcasecmp(Name, "right_contain")==0) TW->CursorRight=CopyStr(TW->CursorRight, Value);
            if (strcasecmp(Name, "remain")==0) SetVar(TW->Options, "remain", Value);
            if (strcasecmp(Name, "remaining")==0) SetVar(TW->Options, "remain", Value);
            if (strcasecmp(Name, "remainder")==0) SetVar(TW->Options, "remain", Value);
            break;


        case 's':
            if (strcasecmp(Name, "selected_attribs")==0) TW->SelectedAttribs=CopyStr(TW->SelectedAttribs, Value);
            else if (strcasecmp(Name, "select_right")==0) TW->CursorRight=CopyStr(TW->CursorRight, Value);
            break;

        case 't':
            if (strcasecmp(Name, "type")==0) TerminalThemeApply(TW, Value);
            break;

        case 'w':
            if (strcasecmp(Name, "width")==0) TW->wid=atoi(Value);
            break;
        }

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);
}


int TerminalWidgetPutLine(TERMWIDGET *TW, int line, const char *Text)
{
    if (TW->x > 0) TerminalCommand(TERM_CURSOR_MOVE, TW->x, TW->y + line, TW->Term);
    else STREAMWriteLine("\r\n", TW->Term);

    TerminalPutStr(Text, TW->Term);

    line++;

    return(line);
}


TERMWIDGET *TerminalWidgetNew(STREAM *TERM, int x, int y, int width, int high, const char *Config)
{
    TERMWIDGET *TW;

    TW=(TERMWIDGET *) calloc(1, sizeof(TERMWIDGET));
    TW->Term=TERM;
    TW->x=x;
    TW->y=y;
    TW->wid=width;
    TW->high=high;
    TW->Options=ListCreate();
    TW->Attribs=CopyStr(TW->Attribs, "");
    TW->CursorAttribs=CopyStr(TW->CursorAttribs, "");
    TW->SelectedAttribs=CopyStr(TW->SelectedAttribs, "");
    TW->CursorLeft=CopyStr(TW->CursorLeft, "");
    TW->CursorRight=CopyStr(TW->CursorRight, "");
    TerminalWidgetParseConfig(TW, Config);

    return(TW);
}

TERMWIDGET *TerminalWidgetCreate(STREAM *TERM, const char *Config)
{
    return(TerminalWidgetNew(TERM, 0, 0, 0, 0, Config));
}


void TerminalWidgetDestroy(void *p_TW)
{
    TERMWIDGET *TW;

    TW=(TERMWIDGET *) p_TW;
    Destroy(TW->Text);
    Destroy(TW->Attribs);
    Destroy(TW->CursorAttribs);
    Destroy(TW->SelectedAttribs);
    Destroy(TW->CursorLeft);
    Destroy(TW->CursorRight);
    ListDestroy(TW->Options, Destroy);
    Destroy(TW);
}
