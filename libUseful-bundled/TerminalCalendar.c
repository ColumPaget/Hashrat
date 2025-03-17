#include "TerminalCalendar.h"
#include "StringList.h"


void TerminalCalendarSetMonthYear(TERMCALENDAR *TC, int Month, int Year)
{
    char *Tempstr=NULL;

    Tempstr=FormatStr(Tempstr, "%d", Month);
    SetVar(TC->Options, "CurrMonth", Tempstr);
    Tempstr=FormatStr(Tempstr, "%d", Year);
    SetVar(TC->Options, "CurrYear", Tempstr);

    Destroy(Tempstr);
}



void TerminalCalendarSetDateState(TERMCALENDAR *TC, int Day, int Month, int Year, const char *State, const char *Attribs)
{
    char *Tempstr=NULL, *Key=NULL;
    const char *ptr;

    Key=FormatStr(Key, "datestate:%04d-%02d-%02d", Year, Month, Day);
    SetVar(TC->Options, Key, State);

    if (StrValid(ptr))
    {
        Tempstr=FormatStr(Tempstr, "stateattribs:%s", Key);
        SetVar(TC->Options, Tempstr, Attribs);
    }

    Destroy(Tempstr);
    Destroy(Key);
}



//Lookup TerminalAttributes (colors, bold, etc) for a day in the calendar.
static char *TerminalCalendarLookupDayAttribs(char *Attribs, TERMCALENDAR *TC, const char *DayStr, int Day, int Month, int Year)
{
    char *Tempstr=NULL;
    const char *ptr;

    Attribs=CopyStr(Attribs, "");

    Tempstr=FormatStr(Tempstr, "datestate:%04d-%02d-%02d", Year, Month, Day);
    ptr=GetVar(TC->Options, Tempstr);

    if (StrValid(ptr))
    {
        Tempstr=FormatStr(Tempstr, "stateattribs:%s", ptr);
        ptr=GetVar(TC->Options, Tempstr);
    }

    if (StrValid(ptr)) Attribs=CatStr(Attribs, ptr);
    else if ((*DayStr == '-') || (*DayStr == '+')) Attribs=CatStr(Attribs, GetVar(TC->Options, "OutsideMonthAttribs"));
    else
    {
        Attribs=CatStr(Attribs, GetVar(TC->Options, "InsideMonthAttribs"));
        if (IsToday(Day, Month, Year)) Attribs=CatStr(Attribs, GetVar(TC->Options, "TodayAttribs"));
    }

    Destroy(Tempstr);

    return(Attribs);
}


void TerminalCalendarDraw(TERMCALENDAR *TC)
{
    char *Tempstr=NULL, *Token=NULL, *CalStr=NULL, *Output=NULL, *Attribs=NULL;
    const char *ptr, *p_DayStr, *p_LeftCursor, *p_RightCursor;
    int i, y=0, count, Selector;
    int Day, Month, Year;

    Selector=atoi(GetVar(TC->Options, "Selector"));
    Month=atoi(GetVar(TC->Options, "CurrMonth"));
    Year=atoi(GetVar(TC->Options, "CurrYear"));

    CalStr=CalendarFormatCSV(CalStr, Month, Year);

    //print out month-year title
    ptr=GetVar(TC->Options, "MonthNames");
    Token=StringListGet(Token, ptr, ",", Month-1);
    Tempstr=CopyStr(Tempstr, GetVar(TC->Options, "TitleAttribs"));
    Tempstr=CatStr(Tempstr, GetVar(TC->Options, "TitleMonthAttribs"));
    Tempstr=CatStr(Tempstr, Token);
    Tempstr=CatStr(Tempstr, GetVar(TC->Options, "TitleAttribs"));
    Tempstr=CatStr(Tempstr, GetVar(TC->Options, "TitleYearAttribs"));
    Token=FormatStr(Token, " %d", Year);
    Tempstr=CatStr(Tempstr, Token);
    Tempstr=TerminalPadStr(Tempstr, ' ', 33);
    Tempstr=CatStr(Tempstr, "~0");


    TerminalCursorMove(TC->Term, TC->x, TC->y);
    y=TerminalWidgetPutLine(TC, y, Tempstr);

    //print out 'day names' headers
    ptr=GetVar(TC->Options, "DayHeaders");
    for (i=0; i < 7; i++)
    {
        Token=StringListGet(Token, ptr, ",", i);
        Output=MCatStr(Output, Token, "  ", NULL);
    }

    y=TerminalWidgetPutLine(TC, y, Output);
    Output=CopyStr(Output, "");

    count=0;
    ptr=GetToken(CalStr, ",|\n", &Token, GETTOKEN_MULTI_SEP | GETTOKEN_INCLUDE_SEP);
    while (ptr)
    {
        if (strcmp(Token, "\n") == 0)
        {
            y=TerminalWidgetPutLine(TC, y, Output);
            Output=CopyStr(Output, "");
        }
        else if (strcmp(Token, ",") == 0) /* do nothing */;
        else
        {
            p_DayStr=Token;
            if ((*p_DayStr == '-') || (*p_DayStr == '+')) p_DayStr++;
            Day=atoi(p_DayStr);

            if (count == Selector)
            {
                p_LeftCursor=TC->CursorLeft;
                p_RightCursor=TC->CursorRight;
            }
            else
            {
                p_LeftCursor=" ";
                p_RightCursor=" ";
            }


            //'Token' passed here is the day with the leading +/- to indicate days outside of the month
            Attribs=TerminalCalendarLookupDayAttribs(Attribs, TC, Token, Day, Month, Year);
            Tempstr=FormatStr(Tempstr, "%s%s%s~0%s ", p_LeftCursor, Attribs, p_DayStr, p_RightCursor);

            count++;
            Output=MCatStr(Output, Tempstr);
        }

        ptr=GetToken(ptr, ",|\n", &Token, GETTOKEN_MULTI_SEP | GETTOKEN_INCLUDE_SEP);
    }
    SetNumericVar(TC->Options, "MaxSelector", count-1);


    if (StrValid(Output)) y=TerminalWidgetPutLine(TC, y, Output);
    STREAMFlush(TC->Term);

    Destroy(Tempstr);
    Destroy(Output);
    Destroy(CalStr);
    Destroy(Token);
}



char *TerminalCalendarReturnDate(char *RetStr, TERMCALENDAR *TC)
{
    char *CalStr=NULL, *Tempstr=NULL;
    int Selector, Month, Year;
    const char *p_Day;

    Month=atoi(GetVar(TC->Options, "CurrMonth"));
    Year=atoi(GetVar(TC->Options, "CurrYear"));
    CalStr=CalendarFormatCSV(CalStr, Month, Year);

    Selector=atoi(GetVar(TC->Options, "Selector"));
    Tempstr=StringListGet(Tempstr, CalStr, ",", Selector);

    p_Day=Tempstr;
    if (*p_Day=='-')
    {
        Month--;
        if (Month < 1)
        {
            Month=12;
            Year--;
        }
        p_Day++;
    }

    if (*p_Day=='+')
    {
        Month++;
        if (Month > 12)
        {
            Month=1;
            Year++;
        }
        p_Day++;
    }

    RetStr=FormatStr(RetStr, "%04d-%02d-%02d", Year, Month, atoi(p_Day));

    Destroy(Tempstr);
    Destroy(CalStr);

    return(RetStr);
}


char *TerminalCalendarOnKey(char *RetStr, TERMCALENDAR *TC, int Key)
{
    int Selector, MaxSelector, val;

    Selector=atoi(GetVar(TC->Options, "Selector"));
    MaxSelector=atoi(GetVar(TC->Options, "MaxSelector"));

    switch (Key)
    {
    case TKEY_ESCAPE:
        Destroy(RetStr);
        return(NULL);
        break;
    case TKEY_LEFT:
        Selector--;
        break;
    case TKEY_RIGHT:
        Selector++;
        break;
    case TKEY_UP:
        Selector -= 7;
        break;
    case TKEY_DOWN:
        Selector += 7;
        break;
    case TKEY_PGUP:
        val=AddToNumericVar(TC->Options, "CurrMonth", 1);
        if (val > 12)
        {
            AddToNumericVar(TC->Options, "CurrYear", 1);
            val=1;
        }
        SetNumericVar(TC->Options, "CurrMonth", val);
        break;
    case TKEY_PGDN:
        val=AddToNumericVar(TC->Options, "CurrMonth", -1);
        if (val < 1)
        {
            AddToNumericVar(TC->Options, "CurrYear", -1);
            val=12;
        }
        SetNumericVar(TC->Options, "CurrMonth", val);
        break;

    case TKEY_ENTER:
        RetStr=TerminalCalendarReturnDate(RetStr, TC);
        break;
    }

    if (Selector < 0) Selector=0;
    if (Selector > MaxSelector) Selector=MaxSelector;

    SetNumericVar(TC->Options, "Selector", Selector);

    return(RetStr);
}


static void TerminalCalendarParseConfig(TERMCALENDAR *TC, const char *Config, int *Month, int *Year)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    TerminalWidgetParseConfig(TC, Config);

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "month")==0) *Month=atoi(Value);
        else if (strcasecmp(Name, "year")==0) *Year=atoi(Value);
        else SetVar(TC->Options, Name, Value);

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);
}


TERMCALENDAR *TerminalCalendarCreate(STREAM *Term, int x, int y, const char *Config)
{
    TERMCALENDAR *TC;

    time_t when;
    struct tm *Now;
    int Month, Year;

    when=time(NULL);
    Now=localtime(&when);
    Month=Now->tm_mon +1;
    Year= Now->tm_year + 1900;

    TC=TerminalWidgetNew(Term, x, y, 0, 0, "type=calendar");
    TerminalCalendarParseConfig(TC, Config, &Month, &Year);
    SetVar(TC->Options, "Selector", "0");


    TerminalCalendarSetMonthYear(TC, Month, Year);

    return(TC);
}


char *TerminalCalendarProcess(char *RetStr, TERMCALENDAR *TC)
{
    int Key;

    TerminalCalendarDraw(TC);
    while (1)
    {
        Key=TerminalReadChar(TC->Term);
        RetStr=TerminalCalendarOnKey(RetStr, TC, Key);
        if (StrValid(RetStr)) break;
        TerminalCalendarDraw(TC);
    }

    return(RetStr);
}


char *TerminalCalendar(char *RetStr, STREAM *Term, int x, int y, const char *Config)
{
    TERMCALENDAR *TC;


    RetStr=CopyStr(RetStr, "");
    TC=TerminalCalendarCreate(Term, x, y, Config);
    if (TC) RetStr=TerminalCalendarProcess(RetStr, TC);

    return(RetStr);
}
