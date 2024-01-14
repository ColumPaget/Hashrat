/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_DATAPARSE_H
#define LIBUSEFUL_DATAPARSE_H

#include "includes.h"

/*
DataParser functions provide a method for parsing JSON, RSS, and YAML documents and also 'ini file' and 'config file' documents
as described below.

'ParserParseDocument' is passed the document type and document text (which can be read into a
string with STREAMReadDocument) and returns a ListNode * that points to the root of a parse tree.

Values of 'DocType' recognized by ParserParseDocument are: json, rss, yaml, config

Items in the parse tree are represented by ListNode items (see List.h). ListNode structures map to parser items
like so:

typedef struct
{
char *Tag;      //name of parse item
int ItemType;   //type of parse item
void *Item;     //contents of parse item
} ListNode;


Parse items can be 'values', in which case the 'Item' member of list node points to a string that holds their
value, or 'Entities' and 'Arrays', in which case the 'Item' member points to a child list of items. For instance
consider the example:

const char *Document="key1=value1\nkey2=value2\nkey3=value3\n";
ListNode *Tree, *Curr;

Tree=ParserParseDocument("config", Document);
Curr=ListGetNext(Tree);
while (Curr)
{
printf("%s = %s\n",Curr->Tag, (char *) Curr->Item);
Curr=ListGetNext(Tree);
}


This should print out item names and values. Now consider a document like:

Thing
{
attribute1=value1
attribute2=value2
attribute3=value3
}


Out previous code would now only find one item in the list, and if it tired to print the value of that item, it
would likely crash! It should check the item type:

Tree=ParserParseDocument("config", Document);
Curr=ListGetNext(Tree);
while (Curr)
{
if (ParseItemIsValue(Curr)) printf("atrrib: %s = %s\n",Curr->Tag, (char *) Curr->Item);
else printf("entity: %s\n",Curr->Tag);
Curr=ListGetNext(Tree);
}

But now it will only display one output, 'entity: Thing'. The members of thing can be accessed by:

SubList=(ListNode *) Curr->Item;

and then using 'ListGetNext' on members of the SubList, but this can get pretty messy when entities contain
other entitites, so the following functions are used to simplify access:

ListNode *ParserOpenItem(ListNode *Items, const char *Name);

ParserOpenItem returns the members of an entitiy. Entities within entities are represented in a manner similar to
directory paths. e.g.

Root=ParserParseDocument("json", Document);
Student=ParserOpenItem(Root, "/School/Class/Students/BillBlogs");
Curr=ListGetNext(Student);
while (Curr)
{
if (Curr->ItemType==ITEM_VALUE) printf("%s = %s\n",Curr->Tag, (char *) Curr->Item);
Curr=ListGetNext(Tree);
}

const char *ParserGetValue(ListNode *Items, const char *Name);

ParserGetValue finds an item of a given name, and returns its value. The 'ItemType' is checked, and NULL is
returned if the item isn't an ITEM_VALUE type. e.g.

Root=ParserParseDocument("json", Document);
Student=ParserOpenItem(Root, "/School/Class/Students/BillBlogs");
printf("Grade Average: %s\n",ParserGetValue(Student, "grade_avg"));
printf("Days Sick: %s\n",ParserGetValue(Student, "sickdays"));


When finished you should free the top level tree by passing it to 'ParserItemsDestroy'. NEVER DO THIS WITH BRANCHES
OBTAINED VIA ParserOpenItem. Only do it to the root node obtained with ParserParseDocument

'config' style documents contain either

<name> <value>
<name>:<value>
<name>=<value>

<group name>
{
<name>=<value>
<name>=<value>
}

so lines consist of a name, and a value seperated by space, colon or '='
names can be quoted if they contain spaces, so:

"ultimate answer" = 42

values do not need to be quoted in the same way, as they are read until the end of line
the value is treated as a string *including any quotes it contains*. Thus you can use
quotes within values and have those passed to the program. e.g.:

command="tar -zcf /tmp/myfile.tgz /root" user=root group=backup

will set the value to be the entire string, including any quotes

key-value pairs can be grouped into objects like so:

"bill blogs"
{
FirstName=Bill
LastName=Blogs
Age=immortal
}
*/



#ifdef __cplusplus
extern "C" {
#endif

typedef enum {PARSER_JSON, PARSER_XML, PARSER_RSS, PARSER_YAML, PARSER_CONFIG, PARSER_INI, PARSER_URL, PARSER_CMON} EParsers;

//this typedef is simply to create a typename that makes code clearer, you can just use 'ListNode' if you prefer
typedef struct lnode PARSER;

#define ITEM_ROOT    0          //dummy header at top level of the parse tree
#define ITEM_STRING  1          //text string
#define ITEM_ENTITY  2          //an object/set with members
#define ITEM_ARRAY   3          //a list of things
#define ITEM_INTEGER 4          //integer value (actually number rather than integer)
#define ITEM_INTERNAL_LIST 100  //list of items that is internal to an entity or array
#define ITEM_ENTITY_LINE   102  //an entity that should be expressed as a single line when output


ListNode *ParserParseDocument(const char *DocType, const char *Doc);
void ParserItemsDestroy(ListNode *Items);
ListNode *ParserFindItem(ListNode *Items, const char *Name);
ListNode *ParserSubItems(ListNode *Node);
ListNode *ParserOpenItem(ListNode *Items, const char *Name);
int ParserItemIsValue(ListNode *Node);
const char *ParserGetValue(ListNode *Items, const char *Name);
ListNode *ParserAddValue(ListNode *Parent, const char *Name, const char *Value);
ListNode *ParserNewObject(ListNode *Parent, int Type, const char *Name);
ListNode *ParserAddArray(ListNode *Parent, const char *Name);
char *ParserExport(char *RetStr, const char *Format, PARSER *P);

#ifdef __cplusplus
}
#endif


#endif
