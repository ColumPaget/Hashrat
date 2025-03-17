#include "../libUseful.h"

main()
{
char *Tempstr=NULL;

Tempstr=CalendarFormatCSV(Tempstr, 1, 2025);
printf("%s\n", Tempstr);

Destroy(Tempstr);
}
