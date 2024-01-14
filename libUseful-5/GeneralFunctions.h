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

//Destroy an allocated object. Will not crash if passed NULL. 
void Destroy(void *Obj);

//reverse bytes within a uint32, There doesn't seem to be a POSIX function for this,
//hence this one.
uint32_t reverse_uint32(uint32_t Input);

//take a string of up to 8 '1' and '0' characters and convert them to an integer value
uint8_t parse_bcd_byte(const char *In);

char *encode_bcd_bytes(char *RetStr, unsigned const char *Bytes, int Len);

//fill 'size' bytes pointed to by 'Str' with char 'fill'. 'Str' is treated as a volatile, which is intended to prevent
//the compiler from optimizing this function out. Use this function to blank memory holding sensitive information, as
//the compiler might decide that blanking memory before freeing it serves no purpose and will this optimize the
//function out. 'volatile' should prevent this.
void xmemset(volatile char *Str, char fill, off_t size);

//setenv that returns TRUE or FALSE rather than returning 0 on success
int xsetenv(const char *Name, const char *Value);

//increment a char * by 'count' but DO NOT GO PAST A NULL CHARACTER. Returns number of bytes actually incremented
int ptr_incr(const char **ptr, int count);


const char *traverse_until(const char *ptr, char terminator);

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


#define SHELLSAFE_BLANK 1

char *MakeShellSafeString(char *RetStr, const char *String, int SafeLevel);

//remap one fd to another. e.g. change stdin, stdout or stderr to point to a different fd
int fd_remap(int fd, int newfd);

//open a file, and remap it to fd ONLY if it opened successfully
int fd_remap_path(int fd, const char *Path, int Flags);


//given a key generate a hash value using the fnv method. Then mod this value by NoOfItems and return result.
//This allows items to be mapped to values with a good spread, and is used internally by the 'Map' datastructure
unsigned int fnv_hash(unsigned const char *key, int NoOfItems);

#ifdef __cplusplus
}
#endif


#endif
