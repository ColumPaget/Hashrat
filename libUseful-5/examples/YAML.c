#include "../libUseful.h"

main()
{
PARSER *P;
char *Tempstr=NULL;

P=ParserParseDocument("yaml", "this: this is some text\nthat: that is some text\n'the other': 1234\nsubitem:\n  stuff in a subitem: whatever\n  contents: with contents\n  block: |\n    this is some\n    block text\n  final: final item\n");
Tempstr=ParserExport(Tempstr, "json", P);
Tempstr=ParserExport(Tempstr, "xml", P);
Tempstr=ParserExport(Tempstr, "yaml", P);
Tempstr=ParserExport(Tempstr, "cmon", P);
printf("%s\n", Tempstr);

Destroy(Tempstr);
}
