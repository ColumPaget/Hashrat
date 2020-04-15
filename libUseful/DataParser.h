/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_DATAPARSE_H
#define LIBUSEFUL_DATAPARSE_H

#include "includes.h"

/*
DataParser functions provide a method for parsing JSON, RSS, and YAML documents and also 'config file' documents
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
if (Curr->ItemType==ITEM_VALUE) printf("atrrib: %s = %s\n",Curr->Tag, (char *) Curr->Item);
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
*/



#ifdef __cplusplus
extern "C" {
#endif

typedef enum {PARSER_JSON, PARSER_XML, PARSER_RSS, PARSER_YAML, PARSER_CONFIG, PARSER_INI, PARSER_URL, PARSER_FORK} EParsers;

//this typedef is simply to create a typename that makes code clearer, you can just use 'ListNode' if you prefer
typedef struct lnode PARSER;

#define ITEM_ROOT   0
#define ITEM_VALUE  1
#define ITEM_ENTITY 2
#define ITEM_ARRAY  3

ListNode *ParserParseDocument(const char *DocType, const char *Doc);
void ParserItemsDestroy(ListNode *Items);
ListNode *ParserFindItem(ListNode *Items, const char *Name);
ListNode *ParserOpenItem(ListNode *Items, const char *Name);
const char *ParserGetValue(ListNode *Items, const char *Name);

#ifdef __cplusplus
}
#endif


#endif
