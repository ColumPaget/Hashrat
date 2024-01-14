#include "TerminalTheme.h"


ListNode *TermTheme=NULL;

ListNode *TerminalThemeInit()
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    TermTheme=ListCreate();

    SetVar(TermTheme, "Menu:CursorLeft", ">");
    SetVar(TermTheme, "Menu:SelectedLeft", "*");

    SetVar(TermTheme, "Choice:CursorLeft", "[");
    SetVar(TermTheme, "Choice:CursorRight", "]");

    ptr=getenv("LIBUSEFUL_TERMINAL_THEME");
    ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        SetVar(TermTheme, Name, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);

    return(TermTheme);
}


const char *TerminalThemeGet(const char *Name)
{
    if (! TermTheme) TerminalThemeInit();
    return(GetVar(TermTheme, Name));
}

void TerminalThemeSet(const char *Name, const char *Value)
{
    if (! TermTheme) TerminalThemeInit();
    SetVar(TermTheme, Name, Value);
}

void TerminalThemeApply(TERMWIDGET *TW, const char *Type)
{
    char *Tempstr=NULL;

    if (! TermTheme) TerminalThemeInit();
    Tempstr=MCopyStr(Tempstr, Type, ":Attribs", NULL);
    TW->Attribs=CopyStr(TW->Attribs, TerminalThemeGet(Tempstr));
    Tempstr=MCopyStr(Tempstr, Type, ":CursorAttribs", NULL);
    TW->CursorAttribs=CopyStr(TW->CursorAttribs, TerminalThemeGet(Tempstr));
    Tempstr=MCopyStr(Tempstr, Type, ":SelectedAttribs", NULL);
    TW->SelectedAttribs=CopyStr(TW->SelectedAttribs, TerminalThemeGet(Tempstr));
    Tempstr=MCopyStr(Tempstr, Type, ":CursorLeft", NULL);
    TW->CursorLeft=CopyStr(TW->CursorLeft, TerminalThemeGet(Tempstr));
    Tempstr=MCopyStr(Tempstr, Type, ":CursorRight", NULL);
    TW->CursorRight=CopyStr(TW->CursorRight, TerminalThemeGet(Tempstr));

    Destroy(Tempstr);
}
