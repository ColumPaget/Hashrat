#include "libUseful-5/libUseful.h"

main()
{
STREAM *Serv, *S;
ListNode *Connections;
char *Doc=NULL;

//Serv=STREAMServerNew("http://127.0.0.1:4040", "rw auth='password-file:/tmp/test.pw'");
Serv=STREAMServerNew("http://127.0.0.1:4040", "rw auth='ip:192.168.2.1,127.0.0.1'");
Connections=ListCreate();
ListAddItem(Connections, Serv);

while (1)
{
STREAMSelect(Connections, NULL);
S=STREAMServerAccept(Serv);
if (! STREAMAuth(S)) HTTPServerSendHeaders(S, 401, "Authentication Required", "WWW-Authenticate=Basic");
else
{
printf("GOT: %d %s %s\n", STREAMAuth(S), STREAMGetValue(S, "HTTP:Method"), STREAMGetValue(S, "HTTP:URL"));

Doc=CopyStr(Doc, "<html><body><h1>Testing testing 1234.</h1><p>This is a test, using html</p></body></html>");
HTTPServerSendDocument(S, Doc, StrLen(Doc), "text/html", "");
//HTTPServerSendFile(S, "/home/colum/C444CBC4.mp4", "", "");
}
STREAMClose(S);
}

Destroy(Doc);
}
