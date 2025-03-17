#include "../libUseful.h"

main()
{
TLineEdit *LE;
STREAM *Term;
char *Tempstr=NULL;
int inchar, pos;


Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS | TERM_SAVEATTRIBS);

LE=LineEditCreate(LINE_EDIT_HISTORY);
LineEditSetMaxHistory(LE, 3);
while (1)
{
inchar=TerminalReadChar(Term);
pos=LineEditHandleChar(LE, inchar);
if (pos == LINE_EDIT_ENTER) 
{
Tempstr=LineEditDone(Tempstr, LE);
printf("\rYou Typed: %s\n", Tempstr);
}
else
{
printf("\r%s   ", LE->Line);
Tempstr=CopyStrLen(Tempstr, LE->Line, LE->Cursor);
printf("\r%s", Tempstr);
}
fflush(NULL);
}

Destroy(Tempstr);
}
