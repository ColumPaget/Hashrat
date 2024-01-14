#include "../libUseful.h"


//Test websockets by connecting to a nostr server and printing out messages


void ParseMessage(const char *msg)
{
char *Token=NULL;
const char *ptr;
ListNode *P;
int val;

printf("MSG: %s\n", msg);
ptr=GetToken(msg, ",", &Token, 0);
ptr=GetToken(ptr, ",", &Token, 0);
ptr=GetToken(ptr, "]", &Token, 0);

P=ParserParseDocument("json", Token);
ptr=ParserGetValue(P, "kind");
if (ptr)
{
val=atoi(ptr);

switch (val)
{
case 1: printf("%s\n", ParserGetValue(P, "content")); break;
case 3: printf("TAGS: %s\n", ParserGetValue(P, "content"));
}
}

ParserItemsDestroy(P);

Destroy(Token);
}


main()
{
STREAM *S;
char *Tempstr=NULL;
int result;

S=STREAMOpen("wss://nos.lol", "");

Tempstr=FormatStr(Tempstr, "[\"REQ\", \"test\", {\"since\": %lu,\"limit\":5}]", time(NULL) - (2 * 3600));
STREAMWriteLine(Tempstr, S);

/*
Tempstr=SetStrLen(Tempstr, 4096);
while (1)
{
result=STREAMReadBytes(S, Tempstr, 4096);
printf("RESULT: %d\n", result);
if (result < 0) break;
if (result > 0)
{
StrTrunc(Tempstr, result);
printf("%s\n", Tempstr);
}
}
*/

Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
ParseMessage(Tempstr);
Tempstr=STREAMReadLine(Tempstr, S);
}

Destroy(Tempstr);
STREAMClose(S);
}
