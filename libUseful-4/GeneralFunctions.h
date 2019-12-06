/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_GENERAL_H
#define LIBUSEFUL_GENERAL_H

#include <stdio.h>
#include "defines.h"
#include "includes.h"

/*
A number of general functions that don't fit anywhere else
*/

#ifdef __cplusplus
extern "C" {
#endif

//Destroy is defined in String.c 'cos of StrLen caching
//void Destroy(void *Obj);

//fill 'size' bytes pointed to by 'Str' with char 'fill'. 'Str' is treated as a volatile, which is intended to prevent
//the compiler from optimizing this function out. Use this function to blank memory holding sensitive information, as
//the compiler might decide that blanking memory before freeing it serves no purpose and will this optimize the
//function out. 'volatile' should prevent this.
void xmemset(volatile char *Str, char fill, off_t size);

int xsetenv(const char *Name, const char *Value);

//increment a char * by 'count' but DO NOT GO PAST A NULL CHARACTER. Returns number of bytes actually incremented
int ptr_incr(const char **ptr, int count);

//treat the first character pointed to by 'ptr' as a quote character. Traverse the string until a matching character is
//found, then return the character after that. This function ignores characters if they are quoted with a preceeding
//'\' character.
const char *traverse_quoted(const char *ptr);

//Add item to a comma seperated list. If the new item is not the first Item in the list, then a comma will be
//placed before it

//e.g.   CSV=CommaList(CSV, "this")
//       CSV=CommaList(CSV, "that")
//       printf("%s\n",CSV);    //this,that

char *CommaList(char *RetStr, const char *AddStr);

//return item at 'pos' in array WITHOUT GOING PAST A NULL. Returns NULL if item can't be reached.
void *ArrayGetItem(void *array[], int pos);


//Creates a bunch of random bytes using /dev/urandom if available, otherwise falling back to weaker methods
int GenerateRandomBytes(char **RetBuff, int ReqLen, int Encoding);

//get a hexidecimamlly encoded random string
char *GetRandomHexStr(char *RetBuff, int len);

//get a random string containing only the characters in 'AllowedChars'
char *GetRandomData(char *RetBuff, int len, char *AllowedChars);

//get a random string of alphanumeric characters
char *GetRandomAlphabetStr(char *RetBuff, int len);

#define SHELLSAFE_BLANK 1

char *MakeShellSafeString(char *RetStr, const char *String, int SafeLevel);

const char *ToSIUnit(double Value, int Base, int Precision);
#define ToIEC(Value, Precision) (ToSIUnit((Value), 1024, Precision))
#define ToMetric(Value, Precision) (ToSIUnit((Value), 1000, Precision))

//Convert to and from metric
double FromSIUnit(const char *Data, int BAse);
#define FromIEC(Value, Precision) (FromSIUnit((Value), 1024))
#define FromMetric(Value, Precision) (FromSIUnit((Value), 1000))


int fd_remap(int fd, int newfd);
int fd_remap_path(int fd, const char *Path, int Flags);

//lookup uid for User
int LookupUID(const char *User);

//lookup gid for Group
int LookupGID(const char *Group);

//lookup username from uid
const char *LookupUserName(uid_t uid);

//lookup groupname from uid
const char *LookupGroupName(gid_t gid);


//given a key generate a hash value using the fnv method. Then mod this value by NoOfItems and return result.
//This allows items to be mapped to values with a good spread, and is used internally by the 'Map' datastructure
unsigned int fnv_hash(unsigned const char *key, int NoOfItems);

#ifdef __cplusplus
}
#endif


#endif
