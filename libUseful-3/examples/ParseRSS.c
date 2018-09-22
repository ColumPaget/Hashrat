#include "../libUseful.h"


//demonstrates parsing an RSS feed 

void main()
{
STREAM *S;
ListNode *P, *SubItem, *Curr;
char *Tempstr=NULL;
const char *ptr;

S=STREAMOpen("http://feeds.feedburner.com/dancarlin/commonsense?format=xml", "r");
if (S)
{
Tempstr=STREAMReadDocument(Tempstr, S);
P=ParserParseDocument("rss",Tempstr);
Curr=ListGetNext(P);
while (Curr)
{
if (Curr->ItemType==ITEM_VALUE) printf("%s: %s\n",Curr->Tag, (char *) Curr->Item);
else
{
	SubItem=ParserOpenItem(P, Curr->Tag);
	ptr=ParserGetValue(SubItem, "title");
	if (StrValid(ptr))
	{
	printf("\n%s\n", ptr);
	ptr=ParserGetValue(SubItem, "description");
	if (StrValid(ptr)) printf("%s\n", ptr);
	ptr=ParserGetValue(SubItem, "enclosure_url");
	if (StrValid(ptr)) printf("%s\n", ptr);
	}
}
Curr=ListGetNext(Curr);
}
}

DestroyString(Tempstr);
ParserItemsDestroy(P);
}
