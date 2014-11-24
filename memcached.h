#ifndef HASHRAT_MEMCACHED_H
#define HASHRAT_MEMCACHED_H

#include "common.h"

STREAM *MemcachedConnect(const char *Server);
int MemcachedSet(const char *Key, int TTL, const char *Value);
char *MemcachedGet(char *RetStr, const char *Key);

#endif

