#ifndef LIB_USEFUL_LIST
#define LIB_USEFUL_LIST

#include <time.h>

#define LIST_FLAG_DELETE 1
#define LIST_FLAG_CASE 2
#define LIST_FLAG_SELFORG 4
#define LIST_FLAG_ORDERED 8
#define LIST_FLAG_CACHE 16
#define LIST_FLAG_TIMEOUT 32
#define LIST_FLAG_DEBUG 8192

#define LIST_FLAG_USER1 1048576
#define LIST_FLAG_USER2 2097152
#define LIST_FLAG_USER3 4194304
#define LIST_FLAG_USER4 8388608
#define LIST_FLAG_USER5 16777216
#define LIST_FLAG_USER6 33554432
#define LIST_FLAG_USER7	67108864
#define LIST_FLAG_USER8	134217728
#define LIST_FLAG_USER0	268435456
#define LIST_FLAG_USERA	536870912
#define LIST_FLAG_USERB	1073741824
#define LIST_FLAG_USERC	2147483648
#define LIST_FLAG_USERD	4294967296


typedef struct lnode
{
unsigned int ItemType;
unsigned int Flags;
char *Tag;
void *Item;
struct lnode *Head;
struct lnode *Jump;
struct lnode *Prev;
struct lnode *Next;
unsigned int Hits;
time_t Time;
} ListNode;


#ifdef __cplusplus
extern "C" {
#endif


typedef void (*LIST_ITEM_DESTROY_FUNC)(void *);
typedef void *(*LIST_ITEM_CLONE_FUNC)(void *);


ListNode *ListCreate();
void ListSetFlags(ListNode *List, int Flags);

void *IndexArrayOnList(ListNode *);
void *AddItemToArray(void *Array,int size, void *Item);
void *DeleteItemFromArray(void *Array,int size, int ItemNo);
void ListDestroy(ListNode *, LIST_ITEM_DESTROY_FUNC);
void ListClear(ListNode *, LIST_ITEM_DESTROY_FUNC);
ListNode *ListAddItem(ListNode *,void *);
ListNode *ListAddNamedItem(ListNode *, const char *Name, void *);
ListNode *ListInsertItem(ListNode *,void *);
ListNode *ListInsertNamedItem(ListNode *,const char *,void *);
ListNode *OrderedListAddNamedItem(ListNode *Head, const char *Name, void *Item);
ListNode *SortedListInsertItem(ListNode *, void *, int (*LessThanFunc)(void *, void *, void *));
ListNode *ListAddNamedItemAfter(ListNode *ListStart,const char *Name,void *Item);
ListNode *ListGetNext(ListNode *);
ListNode *ListGetPrev(ListNode *);
ListNode *ListGetHead(ListNode *);
ListNode *ListGetLast(ListNode *);
ListNode *ListGetNth(ListNode *Head, int n);
ListNode *ListFindNamedItemInsert(ListNode *Head, const char *Name);
ListNode *ListFindNamedItem(ListNode *Head, const char *Name);
ListNode *ListFindItem(ListNode *Head, void *Item);
ListNode *ListJoin(ListNode *, ListNode *);
ListNode *ListClone(ListNode *, LIST_ITEM_CLONE_FUNC);
void ListAppendItems(ListNode *Dest, ListNode *Src, LIST_ITEM_CLONE_FUNC ItemCloner);
void ListSort(ListNode *, void *Data, int (*LessThanFunc)(void *, void *, void *));
void ListSortNamedItems(ListNode *List);

void *ListDeleteNode(ListNode *);
void *ListDeleteItem(ListNode *Head, void *Item);

void ListSwapItems(ListNode *, ListNode *);
int ListSize(ListNode *);

#ifdef __cplusplus
}
#endif


#endif
