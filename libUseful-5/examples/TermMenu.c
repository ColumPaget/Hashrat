#include "../libUseful.h"

main()
{
STREAM *Term;
ListNode *Options;

Options=ListCreate();
SetVar(Options, "this", NULL);
SetVar(Options, "that", NULL);
SetVar(Options, "the other", NULL);

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_SAVE_ATTRIBS | TERM_RAWKEYS);

TerminalClear(Term);
TerminalCursorMove(Term, 0, 0);
TerminalMenu(Term, Options, 2, 2, -4, 6);

TerminalReset(Term);
}
