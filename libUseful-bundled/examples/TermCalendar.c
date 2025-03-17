#include "../libUseful.h"

//demonstrates terminal calendar widget

main(int argc, char *argv[])
{
STREAM *Term;
char *Tempstr=NULL;
TERMCALENDAR *TC;
int i;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS | TERM_SAVEATTRIBS);
TerminalClear(Term);

//Tempstr=TerminalCalendar(Tempstr, Term, 10, 5, "");
//Tempstr=TerminalCalendar(Tempstr, Term, 10, 5, "month=4 year=2005 left_cursor=~y<~0 right_cursor=~y>~0 TitleYearAttribs=~r InsideMonthAttribs=~w OutsideMonthAttribs=~c y=2 TodayAttribs=~e");
Tempstr=TerminalCalendar(Tempstr, Term, 10, 5, "left_cursor=~y<~0 right_cursor=~y>~0 TitleYearAttribs=~r InsideMonthAttribs=~c OutsideMonthAttribs=~b y=2 TodayAttribs=~e datestate:2025-03-11=busy stateattribs:busy=~R~w ");

printf("\n%s\n", Tempstr);

TerminalReset(Term);
}
