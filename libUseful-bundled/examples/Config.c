#include "../libUseful.h"

main()
{
STREAM *S;
char *Tempstr=NULL;
PARSER *P;

S=STREAMOpen("asciiblast.conf", "r");
Tempstr=STREAMReadDocument(Tempstr, S);
STREAMClose(S);

P=ParserParseDocument("config", Tempstr);

if (P)
{
Tempstr=CopyStr(Tempstr, "");
Tempstr=ParserExport(Tempstr, "yaml", P);
printf("%s\n", Tempstr);
}

}
