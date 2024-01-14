/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_ARRAY_H
#define LIBUSEFUL_ARRAY_H

/*
These functions create growable arrays of objects

*/



#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ARRAY_ITEM_DESTROY_FUNC)(void *);

#define StringArrayAdd(a, s) ( (char **) ArrayAdd((void **) (a), (void *) (s)) )
#define StringArrayDestroy(a) ArrayDestroy((void **) (a), (Destroy))

void **ArrayAdd(void **Array, void *Item);

//return item at 'pos' in array WITHOUT GOING PAST A NULL. Returns NULL if item can't be reached.
void *ArrayGetItem(void *array[], int pos);

void ArrayDestroy(void **Array, ARRAY_ITEM_DESTROY_FUNC DestroyFunc);


#ifdef __cplusplus
}
#endif



#endif

