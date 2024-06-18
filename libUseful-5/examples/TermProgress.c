#include "../libUseful.h"

//demonstrates terminal progress bars

main(int argc, char *argv[])
{
STREAM *Term;
char *Tempstr=NULL;
TERMPROGRESS *TP;
int i;

Term=STREAMFromDualFD(0,1);
TerminalInit(Term, TERM_RAWKEYS | TERM_SAVEATTRIBS);
TerminalClear(Term);

//TP=TerminalProgressCreate(Term, "prompt='progress: ' left-contain=' -=' right-contain='=- ' progress=| width=20");
//TP=TerminalProgressCreate(Term, "prompt='progress: ' progress=' ' progress-attribs=~i width=20");
TP=TerminalProgressCreate(Term, "prompt='progress: ' progress=' ' attribs=~B progress-attribs=~W left-contain='' right-contain=' ' width=20");
//TP=TerminalProgressCreate(Term, "prompt='progress: '  width=20");
for (i=0; i <= 20; i++) 
{
Tempstr=FormatStr(Tempstr, "NOW AT: %d", i);
TerminalProgressUpdate(TP, i, 20, Tempstr);
sleep(1);
}

TerminalReset(Term);
}
