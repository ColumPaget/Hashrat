#include "../libUseful.h"


main()
{
STREAM *S;
char *Line=NULL, *Tempstr=NULL;

S=STREAMOpen("tcp:mail.gmail.com:25", "");
if (S)
{
Tempstr=SetStrLen(Tempstr, 255);
gethostname(Tempstr, 255);

Line=MCopyStr(Line, "HELO ", Tempstr, "\r\n",NULL);
STREAMWriteLine(Line, S);

Line=STREAMReadLine(Line, S);

}
}
