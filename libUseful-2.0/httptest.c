#include "libUseful.h"

main()
{
STREAM *S;

/*
S=STREAMCreate();
if (STREAMConnectToHost(S,"icanhazip.com",80,0))
{
printf("Connected!\n");
}
else printf("No connection\n");
*/

HTTPSetFlags(HTTP_DEBUG);
printf("IP: %s\n",GetExternalIP(NULL));
}
