#include "../libUseful.h"

main()
{
STREAM *S;
char *Tempstr=NULL;

S=STREAMOpen("ssh:opti2|tcp:127.0.0.1:12001", "");
if (S)
{
STREAMWriteLine("list wallpaper_mgr\n", S);
Tempstr=STREAMReadLine(Tempstr, S);
printf("GOT: %s\n", Tempstr);
STREAMClose(S);
}

Destroy(Tempstr);
}
