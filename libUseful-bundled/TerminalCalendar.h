/*
Copyright (c) 2025 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

/* This implements a terminal-based calendar widget
 *
 * Example Usage:

STREAM *Term;
char *Tempstr=NULL;
int i;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS | TERM_SAVEATTRIBS);
TerminalClear(Term);

Tempstr=TerminalCalendar(Tempstr, 0, 0, Term, "left_cursor=~y<~0 right_cursor=~y>~0 TitleYearAttribs=~r InsideMonthAttribs=~c OutsideMonthAttribs=~b y=2 TodayAttribs=~e datestate:2025-03-11=busy stateattribs:busy=~R~w");
printf("Selected date: %s\n", Tempstr);

TerminalReset(Term);
}


The date returned by 'TerminalCalendar' is in the format 'YYYY-mm-dd'

 *
 *  The 'Config' parameter of TerminalCalendar or TerminalCalendarCreate takes the following entries:
 *
 *  month=<num>                       Month in year, expressed as a number 1-12 (defaults to current month)
 *  year=<num>                        Year (defaults to current year)
 *  x=<col>                           Position the calendar at terminal column 'x'. 
 *  y=<row>                           Position the calendar at terminal row 'y'. 
 *  left_cursor=<str>                 Text/Attributes for the left hand side of the cursor
 *  right_cursor=<str>                Text/Attributes for the right hand side of the cursor
 *  TitleAttributes=<str>             Attributes for the Month/Year title bar
 *  TitleMonthAttribs=<str>           Attributes for the Month section of the title bar
 *  TitleYearAttribs=<str>            Attributes for the Year section of the title bar
 *  InsideMonthAttribs=<str>          Attributes for the days within the monthe
 *  OutsideMonthAttribs=<str>         Attributes for the days in previous and next month
 *  TodayhAttribs=<str>               Attributes for date that matches 'Today'
 *  DayHeaders=<csv>                  Comma-seperated list of day 'headers', defaults to: Sun,Mon,Tue,Wed,Thu,Fri,Sat
 *  MonthNames=<csv>                  Comma-seperated list of month names, defaults to: January,Febuary,March,April,May,June,July,August,September,October,November
 *  DateState:YYYY-MM-DDD=<state>     Set a 'state' string value against a date (date in format YYYY-MM-DD)
 *  StateAttribs:<state>=<attribs>    Set an attribute string against a given state string
 *
 * In the above settings 'Attributes' refers to tilde-formatted values that set colors and terminal attributes
 */


#ifndef LIBUSEFUL_TERMINAL_CALENDAR_H
#define LIBUSEFUL_TERMINAL_CALENDAR_H

#include "includes.h"
#include "Unicode.h"
#include "KeyCodes.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "Terminal.h"
#include "TerminalWidget.h"

#define TERMCALENDAR TERMWIDGET

#define TerminalCalendarDestroy TerminalWidgetDestroy
#define TerminalCalendarSetOptions TerminalWidgetSetOptions

//create, display and run a terminal calendar all in one command
char *TerminalCalendar(char *RetStr, STREAM *Term, int x, int y, const char *Config);

//create a calendar object
TERMCALENDAR *TerminalCalendarCreate(STREAM *Term, int x, int y, const char *Config);

//draw a calendar
void TerminalCalendarDraw(TERMCALENDAR *TC);

//'run' the calendar
char *TerminalCalendarProcess(char *RetStr, TERMCALENDAR *TC);

//handle keystrokes (up/down/left/right pgup and pgdown to chance month, enter to select)
char *TerminalCalendarOnKey(char *RetStr, TERMCALENDAR *TC, int Key);

//get the current selected date
char *TerminalCalendarReturnDate(char *RetStr, TERMCALENDAR *TC);

//set the month and year of the displayed calendar
void TerminalCalendarSetMonthYear(TERMCALENDAR *TC, int Month, int Year);

// set a 'state' and attributes for that state against a day/month/year 
void TerminalCalendarSetDateState(TERMCALENDAR *TC, int Day, int Month, int Year, const char *State, const char *Attribs);

#ifdef __cplusplus
}
#endif

#endif
