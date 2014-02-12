#include "includes.h"


ListNode *ListGetHead(ListNode *Node)
{
if (! Node) return(NULL);
return(Node->Head);
}



int ListSize(ListNode *List)
{
ListNode *Head;
int *intptr;

if (! List) return(0);
Head=ListGetHead(List);
intptr=(int *) Head->Item;

return(*intptr);

}




int ListSetNoOfItems(ListNode *LastItem, int val)
{
ListNode *Head;
int *intptr;

Head=ListGetHead(LastItem);
if (LastItem->Next==NULL) 
   Head->Prev=LastItem; /* The head Item has its Prev as being the last item! */
intptr=(int *) Head->Item;
(*intptr)=val;

return(*intptr);
}



int ListIncrNoOfItems(ListNode *LastItem)
{
ListNode *Head;
int *intptr;

Head=ListGetHead(LastItem);
if (LastItem->Next==NULL) 
   Head->Prev=LastItem; /* The head Item has its Prev as being the last item! */
intptr=(int *) Head->Item;
(*intptr)++;

return(*intptr);
}

int ListDecrNoOfItems(ListNode *LastItem)
{
ListNode *Head;
int *intptr;

Head=ListGetHead(LastItem);
if (LastItem->Next==NULL) Head->Prev=LastItem->Prev; /* The head Item has its Prev as being the last item! */
intptr=(int *) Head->Item;
(*intptr)--;
return(*intptr);
}


ListNode *ListCreate()
{
ListNode *TempPtr;

TempPtr=(ListNode *)calloc(1,sizeof(ListNode));
TempPtr->Head=TempPtr;
TempPtr->Prev=TempPtr;
TempPtr->Next=NULL;
TempPtr->Item=calloc(1,sizeof(int));

(*(int *)TempPtr->Item)=0;
return(TempPtr);
}

void ListClear(ListNode *ListStart, LIST_ITEM_DESTROY_FUNC ItemDestroyer)
{
  ListNode *Curr,*Next;

  if (! ListStart) return; 
  Curr=ListGetNext(ListStart);
  while (Curr !=NULL) 
  {
      Next=Curr->Next;
      if (ItemDestroyer && Curr->Item) ItemDestroyer(Curr->Item);
      DestroyString(Curr->Tag);
     free(Curr);
     Curr=Next;
  }
ListStart->Next=NULL;
ListStart->Head=ListStart;
ListStart->Prev=ListStart;
ListSetNoOfItems(ListStart,0);
}

void ListDestroy(ListNode *ListStart, LIST_ITEM_DESTROY_FUNC ItemDestroyer)
{
  ListNode *Curr,*Next;

  if (! ListStart) return; 
  ListClear(ListStart, ItemDestroyer);
  free(ListStart->Item);
  free(ListStart);
}


ListNode *ListClone(ListNode *ListStart, LIST_ITEM_CLONE_FUNC ItemCloner)
{
  ListNode *Curr,*NewList;
  void *Item;

  if (! ItemCloner) return(NULL); 
  NewList=ListCreate();

  Curr=ListGetNext(ListStart);
  while (Curr !=NULL) 
  {
     if (ItemCloner)
     {
	     Item=ItemCloner(Curr->Item);
	     ListAddNamedItem(NewList,Curr->Tag,Item);
     }
     Curr=ListGetNext(Curr);
  }
  return(NewList);
}

ListNode *ListAddNamedItemAfter(ListNode *ListStart,const char *Name,void *Item)
{
ListNode *Curr;

if (ListStart==NULL) return(NULL);

Curr=ListStart;
Curr->Next=(ListNode *) calloc(1,sizeof(ListNode)); 
Curr->Next->Prev=Curr;
Curr=Curr->Next;
Curr->Item=Item;
Curr->Head=ListGetHead(ListStart);
Curr->Next=NULL;
if (Name) Curr->Tag=CopyStr(NULL,Name);

ListIncrNoOfItems(Curr);
return(Curr);
}



ListNode *ListAddNamedItem(ListNode *ListStart,const char *Name,void *Item)
{
ListNode *Head, *Curr;

Curr=ListGetLast(ListStart);
if (Curr==NULL) return(Curr);
return(ListAddNamedItemAfter(Curr,Name,Item));
}



ListNode *ListAddItem(ListNode *ListStart,void *Item)
{
return(ListAddNamedItem(ListStart,NULL,Item));
}

void ListUnthreadNode(ListNode *Node)
{
ListNode *Prev, *Next;

Prev=Node->Prev;
Next=Node->Next;
if (Prev) Prev->Next=Next;
if (Next) Next->Prev=Prev;
}

void ListThreadNode(ListNode *Prev, ListNode *Node)
{
ListNode *NewItem, *Next;

//Never thread something to itself!
if (Prev==Node) return;

Next=Prev->Next;
Node->Prev=Prev;
Prev->Next=Node;
Node->Next=Next;
if (Next) Next->Prev=Node; /* Next might be NULL! */
}


ListNode *ListInsertNamedItem(ListNode *InsertNode, const char *Name, void *Item)
{
ListNode *NewItem, *Next;

Next=InsertNode->Next;
NewItem=(ListNode *) calloc(1,sizeof(ListNode)); 
NewItem->Item=Item;
NewItem->Prev=InsertNode;
NewItem->Next=Next;
InsertNode->Next=NewItem;
NewItem->Head=InsertNode->Head;
if (Next) Next->Prev=NewItem; /* Next might be NULL! */
ListIncrNoOfItems(NewItem);
if (StrLen(Name)) NewItem->Tag=CopyStr(NewItem->Tag,Name);
return(NewItem);
}


void OrderedListAddJump(ListNode *From, ListNode *To)
{
int result;

if (! From) return;
if (! To) return;

if (From->Jump)
{
	result=strcmp(From->Jump->Tag, To->Tag);
	if (result > 0)  OrderedListAddJump(From->Next,To);
	else if (result==0)
	{
		 return;
	}
	else
	{
	OrderedListAddJump(From->Next,From->Jump);
	From->Jump=To;
	}
}
else From->Jump=To;
}


ListNode *OrderedListAddNamedItem(ListNode *Head, const char *Name, void *Item)
{
ListNode *NewItem, *Prev, *Curr, *Start;
int count=0, jcount=0, result=-1;


if (! Head) return(NULL);
Prev=Head;
Curr=Head->Next;
Start=Curr;

while (Curr)
{
if (Curr->Jump)
{
	count=0;
	result=strcmp(Curr->Jump->Tag,Name);
	if (result < 0)
	{
		 Curr=Curr->Jump;
		 Prev=Curr->Prev;
		 jcount++;
		if (jcount > 5)
		{
		 OrderedListAddJump(Head->Next, Curr);
		 jcount=0;
		}
	Start=Curr->Next;
	}
}

if (Curr->Tag) result=strcmp(Curr->Tag,Name);
if (result > -1) break;

count++;
if (count > 100)
{
 OrderedListAddJump(Start, Curr);
 count=0;
 Start=Curr->Next;
}

Prev=Curr;
Curr=Curr->Next;
}

NewItem=(ListNode *) calloc(1,sizeof(ListNode)); 
NewItem->Item=Item;
NewItem->Prev=Prev;
NewItem->Next=Prev->Next;
Prev->Next=NewItem;
NewItem->Head=Prev->Head;
if (NewItem->Next) NewItem->Next->Prev=NewItem; /* Next might be NULL! */
ListIncrNoOfItems(NewItem);
if (StrLen(Name)) NewItem->Tag=CopyStr(NewItem->Tag,Name);


result=ListSize(NewItem);

if ((result % 2000)==0)
{
Curr=ListGetNth(Head,result / 2);
OrderedListAddJump(Head->Next, Curr);
}


return(NewItem);
}



ListNode *ListInsertItem(ListNode *InsertNode, void *Item)
{
return(ListInsertNamedItem(InsertNode, NULL, Item));
}


ListNode *InsertItemIntoSortedList(ListNode *List, void *Item, int (*LessThanFunc)(void *, void *, void *))
{
ListNode *Curr, *Prev;

Prev=List;
Curr=ListGetNext(Prev);
while (Curr && (LessThanFunc(NULL, Curr->Item,Item)) )
{
Prev=Curr;
Curr=ListGetNext(Prev);
}

return(ListInsertItem(Prev,Item));
}


ListNode *ListGetNext(ListNode *CurrItem)
{
if (CurrItem !=NULL) return(CurrItem->Next);
else return(NULL);
}

ListNode *ListGetPrev(ListNode *CurrItem)
{
ListNode *Prev;
if (CurrItem !=NULL)
{
Prev=CurrItem->Prev;
/* Don't return the dummy header! */
if (Prev && (Prev->Prev !=NULL) && (Prev != Prev->Head)) return(Prev);
else return(NULL);
}
else return(NULL);
}


ListNode *ListGetLast(ListNode *CurrItem)
{
ListNode *Head;


Head=ListGetHead(CurrItem);
if (! Head) return(NULL);
/* the dummy header has a 'Prev' entry that points to the last item! */
return(Head->Prev);
}

ListNode *ListGetNth(ListNode *Head, int n)
{
ListNode *Curr;
int count=0;

if (! Head) return(NULL);

Curr=ListGetNext(Head);
while (Curr && (count < n))
{
   count++;
   Curr=ListGetNext(Curr);
}
if (count < n) return(NULL);
return(Curr);
}



void *IndexArrayOnList(ListNode *ListHead)
{
ListNode *Curr;
int count, list_size;
void **PtrArray;

Curr=ListGetNext(ListHead);  /* Skip past empty list 'header' item */
list_size=0;
while (Curr !=NULL) 
{
Curr=ListGetNext(Curr);
list_size++;
}
PtrArray=calloc(list_size+1,sizeof(void *));

Curr=ListGetNext(ListHead);  /* All lists have a dummy header, remember? */
for (count=0;count < list_size; count++)
{
PtrArray[count]=Curr->Item;
Curr=ListGetNext(Curr);

}
PtrArray[count]=NULL;
return(PtrArray);

}


void *AddItemToArray(void *Array,int size, void *Item)
{
void **PtrArray;


/* two more than number of items in array, one is the new item, and*/
/* one is the terminating null */
PtrArray=Array;
PtrArray=realloc(PtrArray, (size+2) *sizeof(void *));

PtrArray[size]=Item;
PtrArray[size+1]=NULL;
return(PtrArray);
}

void *DeleteItemFromArray(void *Array,int size, int ItemNo)
{
int count;
void **PtrArray;


/* two more than number of items in array, one is the new item, and*/
/* one is the terminating null */
PtrArray=Array;

/* size is actually number of items in array, hence size+1 is the */
/* terminating null. So we include that in the copy */
for (count=ItemNo; count < (size+1); count++)
{
PtrArray[count]=PtrArray[count+1];
}

PtrArray=realloc(PtrArray, (size+1) *sizeof(void *));
return(PtrArray);
}




ListNode *ListJoin(ListNode *List1, ListNode *List2)
{
ListNode *Curr, *StartOfList2;

Curr=List1;
/*Lists all have a dummy header!*/
StartOfList2=List2->Next;

while (Curr->Next !=NULL) Curr=Curr->Next;
Curr->Next=StartOfList2;
StartOfList2->Prev=Curr;

while (Curr->Next !=NULL) Curr=Curr->Next;
return(Curr);
}


//Item1 is before Item2!
void ListSwapItems(ListNode *Item1, ListNode *Item2)
{
ListNode *Head, *Prev, *Next;

if (! Item1) return;
if (! Item2) return;
Prev=Item1->Prev;
Next=Item2->Next;
Head=ListGetHead(Item1);

if (Head->Next==Item1) Head->Next=Item2;
if (Prev) Prev->Next=Item2;
Item1->Prev=Item2;
Item1->Next=Next;

if (Next) Next->Prev=Item1;
Item2->Prev=Prev;
Item2->Next=Item1;

}


void ListSort(ListNode *List, void *Data, int (*LessThanFunc)(void *, void *, void *))
{
ListNode *Curr=NULL, *Prev=NULL;
int sorted=0;

while (! sorted)
{ 
  sorted=1;
  Prev=NULL;
  Curr=ListGetNext(List);
  while (Curr)
  {
    if (Prev !=NULL)
    {
       if ( (*LessThanFunc)(Data,Curr->Item,Prev->Item) )
       {
         sorted=0;
         ListSwapItems(Prev,Curr);
       }
    }
		Prev=Curr;
    Curr=ListGetNext(Curr);
  }
}

}

void ListSortNamedItems(ListNode *List)
{
ListNode *Curr=NULL, *Prev=NULL;
int sorted=0;

while (! sorted)
{ 
  sorted=1;
  Prev=NULL;
  Curr=ListGetNext(List);
  while (Curr)
  {
    if (Prev !=NULL)
    {
       if (strcmp(Prev->Tag,Curr->Tag) < 0)
       {
         sorted=0;
         ListSwapItems(Prev,Curr);
       }
    }

    Prev=Curr;
    Curr=ListGetNext(Curr);
  }
}

}


ListNode *ListFindNamedItem(ListNode *Head, const char *Name)
{
ListNode *Curr;
int result;

if (! StrLen(Name)) return(NULL);
Curr=ListGetNext(Head);
while (Curr)
{
   if (Curr->Jump)
   {
		result=strcasecmp(Curr->Jump->Tag,Name);
		if (result < 0) Curr=Curr->Jump;
   }
   if (Curr->Tag && (strcasecmp(Curr->Tag,Name)==0)) return(Curr);
   Curr=ListGetNext(Curr);
}
return(Curr);
}




ListNode *ListFindItem(ListNode *Head, void *Item)
{
ListNode *Curr;

if (! Item) return(NULL);
Curr=ListGetNext(Head);
while (Curr)
{
   if (Curr->Item==Item) return(Curr);
   Curr=ListGetNext(Curr);
}
return(Curr);
}


void *ListDeleteNode(ListNode *Node)
{
ListNode *Prev, *Next, *Curr;
void *Contents;
int result;

if (Node==NULL)
{
 return(NULL);
}

Curr=Node->Head;
if (Curr) Curr=Curr->Next;
while (Curr)
{
if (Curr->Jump)
{
	if (Curr->Jump==Node) Curr->Jump=NULL;
	/*
	if (strcmp(Curr->Jump->Tag,Node->Tag) > -1)
	{
		Curr=Curr->Jump;
		continue;
	}
	*/
}

Curr=ListGetNext(Curr);
}

Prev=Node->Prev;
Next=Node->Next;
if (Prev !=NULL) Prev->Next=Next;
if (Next !=NULL) Next->Prev=Prev;


Contents=Node->Item;

ListDecrNoOfItems(Node);
DestroyString(Node->Tag);
free(Node);
return(Contents);
}


void *ListDeleteItem(ListNode *Head, void *Item)
{
ListNode *Node;

Node=ListFindItem(Head, Item);
if (Node) ListDeleteNode(Node);
}


