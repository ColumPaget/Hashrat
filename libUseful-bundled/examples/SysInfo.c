#include "../libUseful.h"
#include <locale.h>

main()
{
printf("%s\n", OSSysInfoString(OSINFO_LOCALE));
printf("%s\n", OSSysInfoString(OSINFO_LANG));
printf("%s\n", OSSysInfoString(OSINFO_COUNTRY));
printf("%s\n", OSSysInfoString(OSINFO_CURRENCY));
printf("%s\n", OSSysInfoString(OSINFO_CURRENCY_SYM));

}
