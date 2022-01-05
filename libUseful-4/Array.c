#include "includes.h"
#include "Array.h"

void **ArrayAdd(void **Array, void *Item)
{
    void *ptr;
    int count=0;

    if (! Array) Array=calloc(10, sizeof(void *));
    else
    {
        for (ptr=*Array; ptr !=NULL; ptr++)
        {
            count++;
        }
    }

    Array=realloc(Array, (count+10) * sizeof(void *));
    Array[count]=Item;
    Array[count+1]=NULL;
    return(Array);
}


void ArrayDestroy(void **Array, ARRAY_ITEM_DESTROY_FUNC DestroyFunc)
{
    void **ptr;

    for (ptr=Array; *ptr != NULL; ptr++) DestroyFunc(*ptr);
    if (Array != NULL) free(Array);
}

