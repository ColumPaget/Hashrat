#include "TerminalWidget.h"



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


    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
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

        case 'c':
            if (strcasecmp(Name, "choices")==0) TerminalWidgetSetOptions(TW, Value);
            break;

        case 'o':
            if (strcasecmp(Name, "options")==0) TerminalWidgetSetOptions(TW, Value);
            break;

        case 'p':
            if (strcasecmp(Name, "prompt")==0) TW->Text=CopyStr(TW->Text, Value);
            else if (strcasecmp(Name, "pad-left")==0) TW->MenuPadLeft=CopyStr(TW->MenuPadLeft, Value);
            else if (strcasecmp(Name, "pad-right")==0) TW->MenuPadRight=CopyStr(TW->MenuPadRight, Value);
            break;

        case 's':
            if (strcasecmp(Name, "select-left")==0) TW->MenuCursorLeft=CopyStr(TW->MenuCursorLeft, Value);
            else if (strcasecmp(Name, "select-right")==0) TW->MenuCursorRight=CopyStr(TW->MenuCursorRight, Value);
            break;
        }

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);
}


TERMWIDGET *TerminalWidgetCreate(STREAM *TERM, const char *Config)
{
    TERMWIDGET *TW;

    TW=(TERMWIDGET *) calloc(1, sizeof(TERMWIDGET));
    TW->Term=TERM;
    TW->Options=ListCreate();
    TW->MenuPadLeft=CopyStr(TW->MenuPadLeft, " ");
    TW->MenuPadRight=CopyStr(TW->MenuPadRight, " ");
    TW->MenuCursorLeft=CopyStr(TW->MenuCursorLeft, "<");
    TW->MenuCursorRight=CopyStr(TW->MenuCursorRight, ">");

    TerminalWidgetParseConfig(TW, Config);

    return(TW);
}



void TerminalWidgetDestroy(void *p_TW)
{
    TERMWIDGET *TW;

    TW=(TERMWIDGET *) p_TW;
    Destroy(TW->Text);
    Destroy(TW->MenuAttribs);
    Destroy(TW->MenuPadLeft);
    Destroy(TW->MenuPadRight);
    Destroy(TW->MenuCursorLeft);
    Destroy(TW->MenuCursorRight);
    ListDestroy(TW->Options, Destroy);
    Destroy(TW);
}
