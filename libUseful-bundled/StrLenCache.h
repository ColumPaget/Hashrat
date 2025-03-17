/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_STRLEN_CACHE_H
#define LIBUSEFUL_STRLEN_CACHE_H

#include "includes.h"

/* 
Libuseful has an internal caching system for strlen results, as functions like CopyStr use
strlen a lot. For large strings (basically manipulating entire documents in memory) this can
result in very dramatic speedups. However, for short strings it can be slightly slower than
libc strlen. The magic number seems to come at about 100 characters long. 

Hence 'StrLen' (defined in String.h) does not use this cache. StrLenFromCache is the version
of StrLen that does use the cache. Furthermore short strings are not added to the cache, as
the benefit of caching them is too small to warrant evicting longer strings from the cache.

All this means that libUseful string functions like CopyStr trade off being slightly slower
than strlen for short strings, against being much better than strlen for large strings. In
normal use these differences aren't noticable, but become so when dealing with large strings.

The cacheing system can be switched on and off using:

LibUsefulSetValue("StrLenCache", "Y");  // turn caching on
LibUsefulSetValue("StrLenCache", "N");  // turn caching off

*/



#ifdef __cplusplus
extern "C" {
#endif


//thse are used internally, you'll not normally use any of these functions
int StrLenFromCache(const char *Str);
void StrLenCacheDel(const char *Str);
void StrLenCacheUpdate(const char *Str, int incr);
void StrLenCacheAdd(const char *Str, size_t len);

#ifdef __cplusplus
}
#endif



#endif

