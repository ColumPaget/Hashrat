#include "../libUseful.h"

main(int argc, char *argv[])
{
char *Tempstr=NULL;

Tempstr=MCopyStr(Tempstr, "ns=",argv[1],NULL);

if (xfork(Tempstr)==0)
{
Tempstr=SetStrLen(Tempstr, 255);
gethostname(Tempstr, 255);
printf("hostname: %s\n",Tempstr);
sleep (20);
}

DestroyString(Tempstr);
}
