Using libUseful
---------------


All libUseful functionality can be accessed by including the header "libUseful.h" in your C program. As headers are usually put in a directory named after the version of libUseful, (e.g. /usr/include/libUseful-4/) this becomes:

```
#include "libUseful-4/libUseful.h"
```

If libUseful has been linked against zlib or openssl then you will have to include those libraries in your compilation, whether you are using them or not

```
gcc -omyProg myProg.c -lUseful-4 -lssl -lcrypto -lz
```


Core Concepts
-------------

There are three core elements of libUseful that you need to be familiar with to do much anything, strings, lists/maps and streams. These elements are used in many functions in libUseful.




libUseful Resizable Strings
---------------------------

libUseful string functions work with memory allocated on the heap (with malloc or calloc) rather than with character arrays allocated on the stack. Many libUseful functions will resize memory to store strings, and thus should only be passed char * variables that are created by libUseful string functions or allocated with calloc.

All functions that work with resizable strings can be passed NULL and they will create new memory as needed. For example.

```
char *Tempstr=NULL;

Tempstr=CopyStr(Tempstr, "this will allocate memory to store my string!");

Tempstr=CatStr(Tempstr, " And to store this extra string too.");
printf("%s\n",Tempstr);

Destroy(Tempstr);
```

Tempstr is initially set to NULL and passed to 'CopyStr'. As CopyStr discovers the buffer it's been passed is set to NULL, and therefore doesn't have room to store our string, it allocates new memory, copies the string into it, and then returns a pointer to that new memory. We set Tempstr to equal this new buffer.

If Tempstr was already allocated, and did have enough room to store our string, then CopyStr just copies the string into the supplied buffer, and returns a pointer to the buffer, which we set 'Tempstr' to point to again. This means memory in 'Tempstr' can be grown as needed. 'CatStr' will likely reallocate the memory again, as it adds more text to the end of what's in 'Tempstr' rather than just overwriting it like 'CopyStr' does.

When we've finished with our string we should free up it's allocated memory with 'Destroy' (or just with 'free' but 'Destroy' handles the case of strings that are still set to NULL, whereas 'free' will crash in those cases).


NEVER DO THIS:

```
char Tempstr[100];

Tempstr=CopyStr(Tempstr, "I have a bad feeling about this");
```

This example creates a fixed-size string on the stack. CopyStr won't be able to resize it, won't be able to fit text into it, and what will happen next is anyone's guess. You must use strings allocated on the heap with those libUseful functions that modify strings. In this case:

```
char *Tempstr=NULL;


Tempstr=CopyStr(Tempstr, "All's well that ends well.");
```

Here the string is set to NULL, and CopyStr will see that and allocate the appropriate amount of memory to store the string. If the string is passed back in later on, then CopyStr will use the `realloc` function to check if the memory allocation is enough to hold the new data, and allocate new memory if not, so these strings only need to be set to NULL on first use.

For those libUseful functions that don't modify the allocation of memory you can pass any 'char *' variable to them. For instance, our second argument to CopyStr, which is a string constant to be copied into Tempstr, can be any kind of null terminated string, even a char array allocated on the stack. As a rule of thumb, arguments to libUseful functions that are declared as 'const char *' in the function definition can be any string, whereas those declared just as 'char *', and into which the function is going to *put* data, rather than just read data, need to be libUseful style strings.

See 'String.h' for details of the individual functions libUseful provides for dealing with resizable strings.


One more thing to note is that you should never try to shorten libUseful strings by setting a character to '\0' (null). This is because libUseful uses an internal system of string length caching to prevent traversing a string every time we need to know its length. Use the StrTrunc and StrTruncChar functions to truncate strings.


Lists and Maps
--------------


libUseful provides a double-linked list system. The core of this are the functions 'ListCreate', 'ListAddItem' 'ListAddNamedItem', 'ListGetNext', 'ListGetPrev', 'ListFindNamedItem' and 'ListDestroy'. These functions operate on 'ListNode' structures. A ListNode has three members of interest 'Item' which is a void * that points to things added to the list, 'Tag' which is the name of the item, and "ItemType", which can be used to put items of different types in a list.


Consider this example:

```
ListNode *Head, *Found;
struct thing *Something, SomethingElse, *AnotherThing, *Thing;

Head=ListCreate();
ListAddItem(Head, "Item1", Something);
ListAddItem(Head, "Item2", SomethingElse);
ListAddItem(Head, "Item3", AnotherThing);

Found=ListFindNamedItem(Head, "Item2");
if (Found) Thing=(struct thing *) Found->Item;

ListDestroy(Head, ThingDestroy);
```

First 'ListCreate' creates a list. All lists have a 'dummy' header node that holds information about the list, so there are no items in this list yet. We add three items (it doesn't matter for this example what they are or where they came from) giving each one a name ("Item1", "Item2" or "Item3") in the list. We then look up "Item3" by name. This returns a ListNode Object if the item is found, and NULL if it isn't. The item itself is pointed to by the 'Item' member of the ListNode Object. As the Item member of a ListNode is a void * and gives no information about what type of thing Item points to is, we must use a cast when extracting the item from a ListNode. We know, of course, that the thing pointed to by the 'Item' member of the "Item2" ListNode is 'SomethingElse', as we just put that in our list under that name. So 'Thing' not points to the same object in memory as 'SomethingElse' points to.

Finally, when we're done with our list, we call "ListDestroy" passing it a function to call to destroy each item in the list. This function will receive a void * pointing to the thing to destroy, it must cast it to the appropriate type, free up any members, then free it. So if we have a structure like:

```
typedef struct
{
char *Name;
char *Class;
char *Value;
} MyStruct;
```

and we put a bunch of these into a list, we would need a function like:

```
void MyStructDestroy(void *p_Item)
{
MyStruct *Item;

Item=(MyStruct *) p_Item;
Destroy(Item->Name);
Destroy(Item->Class);
Destroy(Item->Value);
free(Item);
}
```

Passing this function as the second argument of 'ListDestroy' would free all the items in the list along with the list structure itself.

In some cases you don't want to destroy the list, but just want to clear all the items out of it. For this use the 'ListClear' function, e.g.

```
ListClear(Head, MyStructDestroy);
```

If you want to Destroy or Clear a list, but don't want to free the actual items in it (perhaps because they also exist in another 'master' list somewhere, then you can just pass 'NULL' as the destructor function:

```
ListDestroy(Head, NULL);
```

'ListGetNext' and 'ListGetPrev' allow you to iterate back and forth through a list. If we have a list of items where we have to operate on every item in the list we can do this:

```
ListNode *Head, *Curr;

...  build list somehow ...

Curr=ListGetNext(Head);
while (Curr)
{
Item=(TItem *) Curr->Item;
Curr=ListGetNext(Curr);
}
```


If we want to remove an item from a list, we can use 'ListDeleteNode'. This just removes an item from the list, it doesnt' free/destroy it. Beware of making this mistake:

```
Curr=ListGetNext(Head);
while (Curr)
{
	if (strcmp(Curr->Tag, "Item2")==0) ListDeleteNode(Curr);
	Curr=ListGetNext(Curr);
}
```

'ListDeleteNode' unclips a ListNode from a list, and destroys it, so 'Curr' no longer points to anything, and cannot be used again with ListGetNext. The solution to this is:

```
Curr=ListGetNext(Head);
while (Curr)
{
	Next=ListGetNext(Curr);
	if (strcmp(Curr->Tag, "Item2")==0) ListDeleteNode(Curr);
	Curr=Next;
}
```

ListCreate can optionally be passed a 'Flags' argument:

```
	Head=ListCreate(LIST_FLAG_CACHE);
```

This is a bitmask of the values 'LIST_FLAG_CACHE', 'LIST_FLAG_SELFORG', and 'LIST_FLAG_ORDERED'. 'LIST_FLAG_CACHE' alters the behavior of 'ListFindNamedItem', keeping a cache of the last item found, so that it can quickly be returned if it's asked for again (this particularly pays off for maps, see below). 'LIST_FLAG_SELFORG' will cause items looked up with 'ListFindNamedItem' to 'bubble' up the list so that the most used items are near the top. 'LIST_FLAG_ORDERED' will cause items added to the list with 'ListAddNamedItem' to be placed in the appropriate place to maintain name order. 'LIST_FLAG_ORDERED' and 'LIST_FLAG_SELFORG' cannot be used together, as both require control over the order of the list items.


Maps are a special type of list that are suited to large numbers of named item that are looked up with 'ListFindNamedItem'. A map is actually made of many lists, or 'chains' and an algorithm is used to map any name to a particular chain. Maps are created with the 'MapCreate' function:


```
ListNode *Head;

Head=MapCreate(1000, LIST_FLAG_CACHE);
```

Notice that 'MapCreate' still returns a 'ListNode' object, as most of the differences between lists and maps are concealed by the implementation. The first argument of MapCreate specifies the number of chains in the map. The second one accepts the same flags as ListCreate (but isn't an optional argument). Splitting the list into a Map of 1000 chains not only means that lookups will have to search through one thousandth of the items, but if LIST_FLAG_CACHE is used, then each list will have it's own cache entry. Thus caching is far more effective for maps than for lists. A list cache only makes a difference if the same item gets asked for multiple times in a row, which is an unlikely event (but the cost of checking the cache is low). But a map cache can make a difference if an item in a chain is asked for twice for that chain, even if many items have been asked for from other chains between those two events. This is a much more likely.


The downside of maps is that there is a slight cost to calculating the chain a given item name maps to. This mostly effects adding items to the map, as with lists items are just added to the end (except ordered lists, where adding can be far more costly), but with maps the appropriate chain must be calcualted for each addition. However, if lookups are more common than additions to the list, then maps will outperform lists significantly.

Though maps are intended to be used with ListFindNamedItem, they can still be iterated through with ListGetNext and ListGetPrev.



Streams
-------

The STREAM Object and its related functions are the main way of accessing files and network connections in libUseful. Streams are opened with the function 'STREAMOpen', which accepts a URL and a 'Flags' argument, similar to fopen.

```
STREAM *S;

S=STREAMOpen("tcp:192.168.5.1:25","");
```

In addition to tcp: URLs STREAMOpen will accept udp:, bcast:, ssl:, tls:, ssh:, http:, https:, unix: unixstream:, unixdgram:, and fifo:. Files do not need a protocol prefixed and can just be opened like:

```
S=STREAMOpen("/tmp/myfile.txt","w");
```

'bcast:' streams are broadcast UDP. unix: is a synonym for unixstream, which opens a unix stream socket. ssl: and tls: URLs are equivalent, they are a TCP stream with openssl encryption. An 'ssh' stream will connect to an ssh server and open a shell there, and can thus be used to interact with shells on remote systems. An http: stream will perform an http GET. Those streams that require a username and password can be passed one with standard url format, like this:

```
S=STREAMOpen("http:user:password@host/file","r");
```

The 'Flags' argument contains a list of one-character flags that can have the following values:

```
c		create file if it doesn't exist
r		read only
w		write only
+		make read only or write only stream be read/write
a		append
m		mmap
s		secure
i		inheritable
A		append only (where supported by file system)
I		immutable (where supported by file system)
E   raise an error if this file can't be opened
F		follow (open symbolic links without raising an error)
t		create a temporary file using the path argument as a template for the tmpname function
z		compressed
```

the 'mmap' option opens a file as a shared memory-mapping, which makes it much faster for subsequent programs to open the file if another process already has it open, as the file acts as a form of shared memory. The 'secure' flag locks all memory associated with the stream so that it won't be swapped out to disk. The 'inheritable' flag sets a stream file descriptor to remain open across an exec (STREAMs are CLOEXEC by default). the 'follow' flag allows opening symbolic links without emitting a syslog warning. 

The 'create', 'read only' and 'write only' flags are not meaningful for network streams, which are always read/write. You can still include them though, they will just be ignored. Sometimes you might want to do this to make code clearer, as you can use them as a hint to the reader about what a stream is used for.

Many libUseful functions return or operate on STREAM objects. However, situations arise where one has a file descriptor rather than a STREAM. STREAMs can be created from a file descriptor using 'STREAMFromFD'


```
S=STREAMFromFD(fd);
```

In some cases two file descriptors are used for a connection. The main instance of this are terminal objects, which have one file descriptor for 'standard out' (output to the screen) and another for 'standard in' (input from a keyboard). In this case use 'STREAMFromDualFD'


```
S=STREAMFromDualFD(stdin_fd, stdout_fd);
```

A number of functions exist for reading from streams. Major ones are 'STREAMReadLine', which reads lines up to a linefeed ('\n') character. 'STREAMReadBytes' which functions like the 'read' function in the C standard library, and STREAMReadDocument.

STREAMReadLine returns NULL on reaching end of file, so can be used like:


```
STREAM *S;
char *Tempstr=NULL;

S=STREAMOpenFile("/tmp/myfile.txt", "r");
Tempstr=STREAMReadLine(Tempstr, S);
while (Tempstr)
{
printf("%s",Tempstr);
Tempstr=STREAMReadLine(Tempstr, S);
}
Destroy(Tempstr);
STREAMClose(S);
```

Notice that 'Destroy' can be used even if Tempstr shoud be NULL by the end of this function. Destroy string handles NULL, and this allows it to be used as a 'catch all'. Should, for instance, this function be modified to break out of the while look on encountering a certain line, the programmer won't need to remember to add a Destroy if they've already put one there as standard practice.


STREAMReadBytes needs a preallocated buffer, and works like 'read':


```
STREAM *In, *Out;
char *Tempstr=NULL;
int result;

In=STREAMOpen("http://www.test.com/testfile.html","r");
Out=STREAMOpen("/tmp/mydownload.html","wc");

Tempstr=SetStrLen(Tempstr, 4096);
result=STREAMReadBytes(In,Tempstr,4096);
while (result > 0)
{
STREAMWriteBytes(Out, Tempstr, result);
result=STREAMReadBytes(In,Tempstr,4096);
}

Destroy(Tempstr);
STREAMClose(In);
STREAMClose(Out);
```

STREAMReadDocument slurps an entire file into memory


```
STREAM *S;
char *Tempstr=NULL;

S=STREAMOpenFile("/tmp/myfile.txt", "r");
Tempstr=STREAMReadDocument(Tempstr, S);
printf("%s",Tempstr);

Destroy(Tempstr);
STREAMClose(S);
```


For further documentation of functions in libUseful please look in the .h files. 
