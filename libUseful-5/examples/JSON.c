#include "../libUseful.h"

main()
{
PARSER *P;
char *Tempstr=NULL;

P=ParserParseDocument("json", "{'this': 'this is some text','that': 'that is some text','the other': 1234, 'subitem': {'stuff in a subitem': 'whatever', 'contents': 'with contents'}}");
Tempstr=ParserExport(Tempstr, "json", P);
Tempstr=ParserExport(Tempstr, "xml", P);
printf("%s\n", Tempstr);

Destroy(Tempstr);
}
