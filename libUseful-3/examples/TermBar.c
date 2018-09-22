#include "../libUseful.h"

//demonstrates terminal bars

TERMBAR *UpperTB, *LowerTB;

int KeyCallback(STREAM *S, int key)
{
char *Tempstr=NULL;

if (key==KEY_F4) TerminalBarUpdate(UpperTB, "you pressed F4");
if (key==KEY_F5) TerminalBarUpdate(UpperTB, "you pressed F5");
if (key==KEY_F7) TerminalBarUpdate(UpperTB, "you pressed F7");
if (key==KEY_MENU) TerminalBarUpdate(UpperTB, "you pressed Menu");
if (key==KEY_WIN) TerminalBarUpdate(UpperTB, "you pressed win");
if (key==KEY_CTRL_WIN) TerminalBarUpdate(UpperTB, "you pressed crtrl win");


if (key==KEY_HOME) 
{
	UpperTB->ForeColor=ANSI_RED;
	UpperTB->BackColor=ANSI_YELLOW;
	UpperTB->Flags=ANSI_INVERSE;
	UpperTB->MenuCursorLeft=CopyStr(UpperTB->MenuCursorLeft," {");
	UpperTB->MenuCursorRight=CopyStr(UpperTB->MenuCursorRight,"} ");
	Tempstr=TerminalBarMenu(Tempstr, UpperTB, "this,that,the other");
	printf("%s\r\n",Tempstr);
}

DestroyString(Tempstr);
}

main(int argc, char *argv[])
{
STREAM *Term;
char *Tempstr=NULL;
int i;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS);
TerminalSetKeyCallback(Term, KeyCallback);
TerminalClear(Term);

UpperTB=TerminalBarCreate(Term, "top", "This is an upper terminal bar");
LowerTB=TerminalBarCreate(Term, "lower forecolor=cyan backcolor=blue", "this is a lower terminal bar");

/*
for (i=0; i < 100; i++) 
{
printf("CSCROLL %d\r\n", i);
sleep(1);
}
*/

Tempstr=TerminalBarReadText(Tempstr, LowerTB, 0, ">>");
while (Tempstr)
{
//void TerminalBarUpdate(TERMBAR *TB, const char *Text);
Tempstr=CatStr(Tempstr,"\r\n");
TerminalPutStr(Tempstr, Term);
Tempstr=TerminalBarReadText(Tempstr, LowerTB, 0, ">>");
}

}
