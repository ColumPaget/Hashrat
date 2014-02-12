#include "libUseful.h"

main()
{
char *Tempstr=NULL;

Tempstr=FormatStr(Tempstr,"%s %n","test",100);
printf("%s\n",Tempstr);

}
