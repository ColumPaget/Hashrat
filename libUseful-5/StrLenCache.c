#include "StrLenCache.h"

typedef struct
{
    const char *Str;
    size_t len;
} TStrLenCacheEntry;

static int StrLenCacheSize=0;
static int StrLenCacheMinLen=100;
static TStrLenCacheEntry *StrLenCache=NULL;

void StrLenCacheInit(int Size, int MinStrLen)
{
    StrLenCache=(TStrLenCacheEntry *) calloc(Size, sizeof(TStrLenCacheEntry));
    StrLenCacheSize=Size;
    StrLenCacheMinLen=MinStrLen;
}


TStrLenCacheEntry *StrLenCacheFind(const char *Str)
{
    TStrLenCacheEntry *Entry;

    for (Entry=StrLenCache; Entry < StrLenCache + StrLenCacheSize; Entry++)
    {
        //do not try to use builtin-prefetch here, as we are not interested
        //in the strings that are pointed to, we are just comparing addresses
        if (Entry->Str == Str)
        {
            return(Entry);
        }
    }

    return(NULL);
}


void StrLenCacheDel(const char *Str)
{
    TStrLenCacheEntry *Entry;

    Entry=StrLenCacheFind(Str);
    if (Entry) Entry->Str=NULL;
}

void StrLenCacheUpdate(const char *Str, int incr)
{
    TStrLenCacheEntry *Entry;

    Entry=StrLenCacheFind(Str);
    if (Entry) Entry->len += incr;
}


void StrLenCacheAdd(const char *Str, size_t len)
{
    int i, emptyslot=-1;

    if (LibUsefulFlags & LU_STRLEN_NOCACHE) return;


    if (! StrLenCache) StrLenCacheInit(10, 100);

    //is string already in cache? We must check this even for short lengths!
    for (i=0; i < StrLenCacheSize; i++)
    {
        if (StrLenCache[i].Str == NULL) emptyslot=i;
        else if (StrLenCache[i].Str == Str)
        {
            //if the string has become short, then free its entry in the cache
            if (len < StrLenCacheMinLen) StrLenCache[i].Str=NULL;
            else StrLenCache[i].len=len;
            return;
        }
    }

//don't pollute cache with short strings that don't take long to look up
//strlen caching has been seen to give a benefit with very large strings,
//but modern processors with built-in strlen functions are proabably faster
//for short strings. The magic number seems to be about 100 chars.
    if (len > StrLenCacheMinLen)
    {
        //if we get here than string isn't in cache and we add it
        if (emptyslot == -1) emptyslot=rand() % StrLenCacheSize;

        StrLenCache[emptyslot].Str=Str;
        StrLenCache[emptyslot].len=len;
    }
}


int StrLenFromCache(const char *Str)
{
    TStrLenCacheEntry *Entry;

    if (! StrValid(Str)) return(0);

    if (! (LibUsefulFlags & LU_STRLEN_NOCACHE))
    {
        Entry=StrLenCacheFind(Str);
        if (Entry) return(Entry->len);
    }

//okay, nothing worked, fall back to good old strlen
    return(strlen(Str));
}

