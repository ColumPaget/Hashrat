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

static char *TerminalThemeLoadValue(char *RetStr, const char *Theme, const char *Attrib)
{
    char *Tempstr=NULL;
    const char *ptr;

    if (! TermTheme) TerminalThemeInit();

    Tempstr=MCopyStr(Tempstr, Theme, ":", Attrib, NULL);
    ptr=TerminalThemeGet(Tempstr);
    if (StrValid(ptr)) RetStr=CopyStr(RetStr, ptr);

    Destroy(Tempstr);
    return(RetStr);
}

void TerminalThemeApply(TERMWIDGET *TW, const char *Type)
{
    TW->Attribs=TerminalThemeLoadValue(TW->Attribs, Type, "Attribs");
    TW->CursorAttribs=TerminalThemeLoadValue(TW->CursorAttribs, Type, "CursorAttribs");
    TW->SelectedAttribs=TerminalThemeLoadValue(TW->SelectedAttribs, Type, "SelectedAttribs");
    TW->CursorLeft=TerminalThemeLoadValue(TW->CursorLeft, Type, "CursorLeft");
    TW->CursorRight=TerminalThemeLoadValue(TW->CursorRight, Type, "CursorRight");
}
