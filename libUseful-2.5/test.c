#include "libUseful.h"

/*
main()
{
char *ptr;

CredsStoreAdd("test1", "user1", "pass1");
CredsStoreAdd("test2", "user2", "pass2");
CredsStoreAdd("test3", "user3", "pass3");
CredsStoreAdd("test4", "user4", "pass4");
ptr=CredsStoreGetSecret("test3", "user3");

printf("%s\n",ptr);
}
*/


void DumpVars(ListNode *Vars)
{
ListNode *Curr;

Curr=ListGetNext(Vars);
while (Curr)
{
printf("%s = %s\n",Curr->Tag,Curr->Item);
Curr=ListGetNext(Curr);
}
}


int Tester(const char *Source, ListNode *Vars)
{
char *Tempstr=NULL;

Tempstr=FormatStr(Tempstr,"pid=%d&string=hello_back&val=%s",getpid(),GetVar(Vars,"integer"));
MessageBusWrite("parent", Tempstr);

DestroyString(Tempstr);
return(TRUE);
}


main()
{
char *Tempstr=NULL;
ListNode *Vars, *Streams;
STREAM *S;
int i;
char *IP[]={"217.33.140.70","8.8.8.8","4.2.2.1","88.198.48.36",NULL};

Vars=ListCreate();
Streams=ListCreate();
MessageBusRegister("proc:tester.test.func", 4, 10, Tester);
MessageBusRegister("http://freegeoip.net/xml/", 4, 10, NULL);

printf("PARENT: %d\n",getpid());
for (i=0; i < 4; i++)
{
Tempstr=FormatStr(Tempstr,"string=hello&integer=%d",i);
MessageBusWrite("proc:tester.test.func", Tempstr);
MessageBusWrite("http://freegeoip.net/xml/", IP[i]);

MessageQueueAddToSelect(Streams);
S=STREAMSelect(Streams, NULL);
if (S) 
{
	MessageBusRecv(S, &Tempstr, Vars);
	DumpVars(Vars);
}
else printf("NO ACTIVITY!\n");
sleep(1);
}


}
