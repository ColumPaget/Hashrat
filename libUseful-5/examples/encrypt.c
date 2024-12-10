#include "../libUseful.h"

main()
{
STREAM *S;
char *Tempstr=NULL;
int result=0;

S=STREAMOpen("file.enc", "we");
printf("should fail: %d\n",S);

S=STREAMOpen("file.enc", "we encrypt=whatever");
STREAMWriteLine("Twas Brillig and the slivey tooves didst gyre and gimble in the wabe\n", S);
STREAMClose(S);

S=STREAMOpen("file.enc", "re encrypt=whatever");
if (S)
{
Tempstr=STREAMReadLine(Tempstr, S);
//Tempstr=SetStrLen(Tempstr, 4096);
//result=STREAMReadBytes(S, Tempstr, 4096);
//StrTrunc(Tempstr, result);
printf("%s\n", Tempstr);
STREAMClose(S);
}

Destroy(Tempstr);
}
