#include "../libUseful.h"

//demonstrates a few functions related to terminal control

void Interact(STREAM *Term, const char *Prompt, int Flags)
{
char *Input=NULL, *Tempstr=NULL;

TerminalPutStr(Prompt, Term);

Input=TerminalReadText(Input, Flags, Term);

TerminalPutStr("\r\n", Term);
Tempstr=MCopyStr(Tempstr, "~eYou typed:~0 ", Input, "\r\n", NULL);
TerminalPutStr(Tempstr, Term);


DestroyString(Tempstr);
DestroyString(Input);
}



main(int argc, char *argv[])
{
STREAM *Term;
char *Tempstr=NULL, *Input=NULL;
TERMBAR *TB;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS);
TerminalClear(Term);

Interact(Term, "~y~RHello. Type something and I'll ~eecho it~>~0\r\n", 0);
Interact(Term, "~r~WNow type something and I'll ~ehide it~>~0\r\n", TERM_HIDETEXT);
Interact(Term, "~c~BNow type something and I'll ~estar it out like a password~>~0\r\n", TERM_SHOWSTARS);
Interact(Term, "~w~GNow type something and I'll ~estar but show the last letter~>~0\r\n", TERM_SHOWTEXTSTARS);
TerminalPutStr("~iOkay, that's all folks. exiting..~>~0\r\n", Term);

sleep(3);
TerminalReset(Term);

DestroyString(Tempstr);
DestroyString(Input);
}
