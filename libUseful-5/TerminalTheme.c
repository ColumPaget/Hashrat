#include "TerminalTheme.h"


ListNode *TermTheme=NULL;

ListNode *TerminalThemeInit()
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    TermTheme=ListCreate();

    SetVar(TermTheme, "Menu:CursorLeft", ">");
    SetVar(TermTheme, "Menu:SelectedLeft", "*");
    SetVar(TermTheme, "Menu:CursorAttribs", "~e");
    SetVar(TermTheme, "Menu:SelectedAttribs", "~e");

    SetVar(TermTheme, "Progress:CursorLeft", "[");
    SetVar(TermTheme, "Progress:CursorRight", "]");
    SetVar(TermTheme, "Progress:Progress", "*");
    SetVar(TermTheme, "Progress:Remain", " ");


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


const char *TerminalThemeGet(const char *Scope, const char *Attrib)
{
    char *Tempstr=NULL;
    const char *p_Value;

    if (! TermTheme) TerminalThemeInit();
    Tempstr=MCopyStr(Tempstr, Scope, ":", Attrib, NULL);
    p_Value=GetVar(TermTheme, Tempstr);

    Destroy(Tempstr);

    return(p_Value);
}


void TerminalThemeSet(const char *Name, const char *Value)
{
    if (! TermTheme) TerminalThemeInit();
    SetVar(TermTheme, Name, Value);
}


static char *TerminalThemeLoadValue(char *RetStr, const char *Theme, const char *Attrib)
{
    const char *ptr;

    if (! TermTheme) TerminalThemeInit();
    ptr=TerminalThemeGet(Theme, Attrib);
    if (StrValid(ptr)) RetStr=CopyStr(RetStr, ptr);

    return(RetStr);
}

void TerminalThemeApply(TERMWIDGET *TW, const char *Type)
{
    TW->Attribs=TerminalThemeLoadValue(TW->Attribs, Type, "Attribs");
    TW->CursorAttribs=TerminalThemeLoadValue(TW->CursorAttribs, Type, "CursorAttribs");
    TW->SelectedAttribs=TerminalThemeLoadValue(TW->SelectedAttribs, Type, "SelectedAttribs");
    TW->CursorLeft=TerminalThemeLoadValue(TW->CursorLeft, Type, "CursorLeft");
    TW->CursorRight=TerminalThemeLoadValue(TW->CursorRight, Type, "CursorRight");

    if (strcasecmp(Type, "progress")==0)
    {
        SetVar(TW->Options, "progress", TerminalThemeGet(Type, "progress"));
        SetVar(TW->Options, "remain", TerminalThemeGet(Type, "remain"));
    }
}
