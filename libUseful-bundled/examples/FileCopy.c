#include "../libUseful.h"

main(int argc, char *argv[])
{
STREAM *In, *Out;

printf("a1=%s a2=%s\n",argv[1], argv[2]);
In=STREAMOpen(argv[1], "r");
STREAMCopy(In, argv[2]);
STREAMClose(In);
STREAMClose(Out);
}
