#include "libUseful.h"

main()
{
STREAM *S;
char *Tempstr=NULL;

S=STREAMOpenFile("/etc/services",SF_RDONLY | SF_MMAP);
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
printf("%s",Tempstr);
Tempstr=STREAMReadLine(Tempstr, S);
}

DestroyString(Tempstr);
}
