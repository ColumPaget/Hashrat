#include "../libUseful.h"

main(int argc, const char *argv[])
{
STREAM *S, *Out;
char *Tempstr=NULL;
int result=0;

Out=STREAMFromDualFD(0,1);
S=STREAMOpen(argv[1], "re encrypt=whatever");
if (S)
{
Tempstr=SetStrLen(Tempstr, 4096);
result=STREAMReadBytes(S, Tempstr, 4096);
while (result > 0)
{
STREAMWriteBytes(Out, Tempstr, result);
result=STREAMReadBytes(S, Tempstr, 4096);
}
STREAMClose(S);
}

Destroy(Tempstr);
}
