#include "../libUseful.h"

main()
{
printf("%s\n", FormatDuration("uptime: %dd %hh %mm %ss", OSSysInfoLong(OSINFO_UPTIME)));
printf("%s\n", FormatDuration("uptime: %dd %hh %mm %ss %% %%% %x %", OSSysInfoLong(OSINFO_UPTIME)));
printf("%s\n", FormatDuration("uptime: %hh %mm %ss", OSSysInfoLong(OSINFO_UPTIME)));
printf("%s\n", FormatDuration("uptime: %dd %mm %ss", OSSysInfoLong(OSINFO_UPTIME)));
}
