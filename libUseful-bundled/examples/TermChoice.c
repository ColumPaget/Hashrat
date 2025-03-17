#include "../libUseful.h"

main()
{
STREAM *Term;
ListNode *Options;
char *Tempstr=NULL;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_SAVE_ATTRIBS | TERM_RAWKEYS);

TerminalClear(Term);
TerminalCursorMove(Term, 0, 0);
Tempstr=TerminalChoice(Tempstr, Term, "prompt=choose: width=40 left_cursor='~e<' right_cursor='>~0' options='this,that,the other,even more, option this, do anything, but whatever' ");

TerminalReset(Term);

Destroy(Tempstr);
}
