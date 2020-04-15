#include "../libUseful.h"


//This will download a testfile which is a list of the top million websites, load it into a map or list, and then do lookups
//in it to demonstrate the speed or otherwise of these



void Lookup(ListNode *Map, const char *Name)
{
ListNode *Node;

Node=ListFindNamedItem(Map, Name);
if (Node) printf("%s  %s %s\n",Node->Tag,Node->Item,Node->Prev->Item);
}


void DownloadTestFile()
{
STREAM *In, *Out;

In=STREAMOpen("http://downloads.majestic.com/majestic_million.csv","r");
if (In)
{
	Out=STREAMOpen("majestic_million.csv","w");
	STREAMSendFile(In, Out, 0, SENDFILE_LOOP);
	STREAMClose(Out);
	STREAMClose(In);
}

}



void LoadCSV(ListNode *List)
{
char *Tempstr=NULL, *Rank=NULL, *Site=NULL;
const char *ptr;
STREAM *S;

S=STREAMOpen("majestic_million.csv","r");
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
ptr=GetToken(Tempstr,",",&Rank,0);
ptr=GetToken(ptr,",",&Site,0);
ptr=GetToken(ptr,",",&Site,0);

ListAddNamedItem(List,Site,CopyStr(NULL,Rank));
Tempstr=STREAMReadLine(Tempstr, S);
}

STREAMClose(S);
}


void Test(ListNode *List, const char *Type)
{
long start, end, i, lookups=1000;

start=GetTime(TIME_CENTISECS);
LoadCSV(List);
end=GetTime(TIME_CENTISECS);
printf("Time to load a million records into a %s: %f secs\n", Type, (float) (end-start) /100.0);

start=GetTime(TIME_CENTISECS);
for (i=0; i < lookups; i++) Lookup(List, "freshcode.club");
end=GetTime(TIME_CENTISECS);
printf("Time to do %d lookups in a million record %s %f secs\n", lookups, Type, (float) (end-start) /100.0);
}


main()
{
ListNode *List;

if (access("majestic_million.csv",F_OK) !=0) DownloadTestFile();

List=ListCreate();
Test(List, "LIST");
ListDestroy(List, Destroy);

List=MapCreate(1000, 0);
Test(List, "MAP");
ListDestroy(List, Destroy);
}
