/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_ARRAY_H
#define LIBUSEFUL_ARRAY_H

typedef void (*ARRAY_ITEM_DESTROY_FUNC)(void *);

#define StringArrayAdd(a, s) ( (char **) ArrayAdd((void **) (a), (void *) (s)) )
#define StringArrayDestroy(a) ArrayDestroy((void **) (a), (Destroy))

void **ArrayAdd(void **Array, void *Item);
void ArrayDestroy(void **Array, ARRAY_ITEM_DESTROY_FUNC DestroyFunc);

#endif
