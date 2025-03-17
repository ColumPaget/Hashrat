/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_IPADDR_H
#define LIBUSEFUL_IPADDR_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

//these functions return TRUE if a string appears to be an IP4 or IP6 address
int IsIP4Address(const char *Str);
int IsIP6Address(const char *Str);

//returns TRUE for either IP6 or IP4
int IsIPAddress(const char *Str);

//lookup the primary IP address of a hostname
const char *LookupHostIP(const char *Host);

//lookup a list of addresses for a hostname
ListNode *LookupHostIPList(const char *Host);

//reverse lookup hostname for an IP
const char *IPStrToHostName(const char *IP);

//convert an integer representation of IP4 address to a string
const char *IPtoStr(unsigned long IP);

//convert a string representation of IP4 address to an integer representation
unsigned long StrtoIP(const char *IPStr);

//new functions that use inet_pton and inet_ntop
void StrtoIP6(const char *Str, struct in6_addr *dest);

char *IP4toStr(char *RetStr, unsigned long IP);

char *IP6toStr(char *RetStr, struct in6_addr *IP);


#ifdef __cplusplus
}
#endif



#endif
