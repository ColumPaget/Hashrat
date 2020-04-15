/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIB_USEFUL_LIST
#define LIB_USEFUL_LIST

/*

This module provides double linked lists and 'maps'. Maps are hashed arrays of linked lists, so they're very good for storing large numbers of items that can be looked up by a key or name

*/


// Functions passsed to 'ListCreate' or 'MapCreate' or set against a ListNode item
#define LIST_FLAG_DELETE 1      //internally used flag
#define LIST_FLAG_CASE 2        //when doing searches with 'ListFindNamedItem' etc, use case
#define LIST_FLAG_SELFORG 4     //self organize list so most used items are at the top
#define LIST_FLAG_ORDERED 8     //create an 'ordered' list (so insert items in order)
#define LIST_FLAG_CACHE 16      //cache the last found item (especially useful for maps)
#define LIST_FLAG_TIMEOUT 32    //internally used flag 
#define LIST_FLAG_MAP_HEAD   64 //internally used flag
#define LIST_FLAG_MAP_CHAIN 128 //internally used flag
#define LIST_FLAG_MAP (LIST_FLAG_MAP_HEAD | LIST_FLAG_MAP_CHAIN) //internally used flag
#define LIST_FLAG_STATS 256     //internally used flag


//these flags are available to be set against a listnode for whatever purpose the user
//desires
#define LIST_FLAG_USER1 1024
#define LIST_FLAG_USER2 2048
#define LIST_FLAG_USER3 4096
#define LIST_FLAG_USER4 8192
#define LIST_FLAG_USER5 16384
#define LIST_FLAG_DEBUG 32768

#define ANYTYPE -1


#include <time.h>

typedef struct
{
    time_t Time;
//In the 'head' item 'Hits' is used to hold the count of items in the list
    unsigned long Hits;
} ListStats;


//attempt to order items in likely use order, this *might* help the L1 cache
//is 32 bytes, so should fit in one cache line
typedef struct lnode
{
    struct lnode *Next;
    struct lnode *Prev;
    char *Tag;
    uint16_t ItemType;
    uint16_t Flags;
//in map heads ItemType is used to hold the number of buckets
    struct lnode *Head;
    void *Item;
    struct lnode *Side;
    ListStats *Stats;
} ListNode;


//things not worthy of a full function or which pull tricks with macros

//this allows 'ListCreate' to be called with flags or without
#define ListCreate(F) (ListInit(F + 0))

#define MapChainGetHead(Node) (((Node)->Flags & LIST_FLAG_MAP_CHAIN) ? (Node) : Node->Head)

//if L isn't NULL then return L->Head, it's fine if L->Head is null
#define ListGetHead(Node) ((Node) ? MapChainGetHead(Node) : NULL)

#define ListNodeGetHits(node) ((node)->Stats ? (node)->Stats->Hits : 0)
#define ListNodeGetTime(node) ((node)->Stats ? (node)->Stats->Time : 0)


//#define ListGetNext(Node) (Node ? (((Node)->Head->Flags & (LIST_FLAG_MAP|LIST_FLAG_MAP_CHAIN)) ? MapGetNext(Node) : (Node)->Next) : NULL)
#define ListGetNext(Node) ((Node) ? MapGetNext(Node) : NULL)

//listDeleteNode handles ListFindItem returning NULL, so no problems here
#define ListDeleteItem(list, item) (ListDeleteNode(ListFindItem((list), (item))))

#define ListInsertItem(list, item) (ListInsertTypedItem((list), 0, "", (item)))
#define ListAddItem(list, item) (ListAddTypedItem((list), 0, "", (item)))

#define ListInsertNamedItem(list, name, item) (ListInsertTypedItem((list), 0, (name), (item)))
#define ListAddNamedItem(list, name, item) (ListAddTypedItem((list), 0, (name), (item)))


//Again, NULL is handled here by ListInsertTypedItem
#define OrderedListAddNamedItem(list, name, item) (ListInsertTypedItem(ListFindNamedItemInsert((list), (name)), 0, (name), (item)))

//in map heads ItemType is used to hold the number of buckets
#define MapChainCount(Head) (((Head)->Flags & LIST_FLAG_MAP) ? (Head)->ItemType : 0)

#ifdef __cplusplus
extern "C" {
#endif

// function prototypes for 'destroy' and 'clone' functions used by
// 'DestroyList' and 'CloneList'
typedef void (*LIST_ITEM_DESTROY_FUNC)(void *);
typedef void *(*LIST_ITEM_CLONE_FUNC)(void *);

unsigned long ListSize(ListNode *Node);

//Dump stats on hash distribution in a map
void MapDumpSizes(ListNode *Head);

//create a list
ListNode *ListInit(int Flags);

//SetFlags on a list
void ListSetFlags(ListNode *List, int Flags);

//Set time on a list. Normally this is automatically set on insertion
void ListNodeSetTime(ListNode *Node, time_t When);

//set number of hits on a listnode, This is normally set by 'ListFindNamedItem' 
void ListNodeSetHits(ListNode *Node, int Hits);

//add to number of hits on a listnode
int ListNodeAddHits(ListNode *Node, int Hits);


//create a map
ListNode *MapCreate(int Buckets, int Flags);

//get nth chain in a maps hashmap
ListNode *MapGetNthChain(ListNode *Map, int n);

//get the map chain that hashes to 'Key'
ListNode *MapGetChain(ListNode *Map, const char *Key);

//destroy a list or a map
void ListDestroy(ListNode *, LIST_ITEM_DESTROY_FUNC);

//clear all items from a list or a map
void ListClear(ListNode *, LIST_ITEM_DESTROY_FUNC);


//slot a node into a list
void ListThreadNode(ListNode *Prev, ListNode *Node);

//unclip a node from a list
void ListUnThreadNode(ListNode *Node);

//add an item to a list or map, 'Type' is just a number used to identify different 
//types of thing in a list or map
ListNode *ListAddTypedItem(ListNode *List, uint16_t Type, const char *Name, void *Item);

//insert an item into a list at 'InsertPoint'
ListNode *ListInsertTypedItem(ListNode *InsertPoint,uint16_t Type, const char *Name,void *Item);



//get previous item in a list
ListNode *ListGetPrev(ListNode *);

//get last item in a list
ListNode *ListGetLast(ListNode *);

//get nth item in a list
ListNode *ListGetNth(ListNode *Head, int n);


ListNode *MapChainGetNext(ListNode *);

//get next item in a map
ListNode *MapGetNext(ListNode *);


//find node after which to insert an item in an ordered list
ListNode *ListFindNamedItemInsert(ListNode *Head, const char *Name);

//find an item of a given type by name
ListNode *ListFindTypedItem(ListNode *Head, int Type, const char *Name);

//find any item that name
ListNode *ListFindNamedItem(ListNode *Head, const char *Name);

//find a specific item
ListNode *ListFindItem(ListNode *Head, void *Item);

//Join lists together
ListNode *ListJoin(ListNode *List1, ListNode *List2);

//Clone a list. Function 'ItemCloner' creates a copy of the items (which could be any kind of structure or whatever)
ListNode *ListClone(ListNode *List, LIST_ITEM_CLONE_FUNC ItemCloner);

//Append a list of items to a list. This is different from 'join list' because instead of
//splicing two lists together, the ItemCloner function is used to create copies of the
//items and insert them into the destination list as new nodes
void ListAppendItems(ListNode *Dest, ListNode *Src, LIST_ITEM_CLONE_FUNC ItemCloner);

//sort a list of named items
void ListSortNamedItems(ListNode *List);

//sort a list of non-named items, using 'LessThanFunc' to compare them
void ListSort(ListNode *, void *Data, int (*LessThanFunc)(void *, void *, void *));

//Delete a node from a list. This returns the item that the node points to
void *ListDeleteNode(ListNode *);

//swap to items/listnodes in a list
void ListSwapItems(ListNode *Item1, ListNode *Item2);


#ifdef __cplusplus
}
#endif


#endif
