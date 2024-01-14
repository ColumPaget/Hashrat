#include "../libUseful.h"

main()
{
PARSER *DB;
STREAM *S;
char *Tempstr=NULL;

S=STREAMOpen("/tmp/wallpaper_mgr.db", "r");
Tempstr=STREAMReadLine(Tempstr, S);
STREAMClose(S);

DB=ParserParseDocument("cmon", Tempstr);

Destroy(Tempstr);
}
