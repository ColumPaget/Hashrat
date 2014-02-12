#ifndef LIB_USEFUL_LIST
#define LIB_USEFUL_LIST

#define LIST_FLAG_DELETE 1

typedef struct lnode
{
int ItemType;
int Flags;
char *Tag;
void *Item;
struct lnode *Head;
struct lnode *Jump;
struct lnode *Prev;
struct lnode *Next;
} ListNode;


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LIST_ITEM_DESTROY_FUNC)(void *);
typedef void *(*LIST_ITEM_CLONE_FUNC)(void *);

ListNode *ListCreate();
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
ListNode *ListFindNamedItem(ListNode *Head, const char *Name);
ListNode *ListFindItem(ListNode *Head, void *Item);
ListNode *ListJoin(ListNode *, ListNode *);
ListNode *ListClone(ListNode *, LIST_ITEM_CLONE_FUNC);
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
