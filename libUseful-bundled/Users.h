/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_USERS_H
#define LIBUSEFUL_USERS_H

#include "defines.h"
#include "includes.h"

/*
A number of general functions that don't fit anywhere else
*/

#ifdef __cplusplus
extern "C" {
#endif



//lookup uid for User
int LookupUID(const char *User);

//lookup gid for Group
int LookupGID(const char *Group);

//lookup username from uid
const char *LookupUserName(uid_t uid);

//lookup groupname from gid
const char *LookupGroupName(gid_t gid);


//switch user or group by id (call SwitchGID first, or you may not have permissions to switch user)
int SwitchUID(int uid);
int SwitchGID(int gid);

//switch user or group by name (call SwitchGgroup first, or you may not have permissions to switch user)
int SwitchUser(const char *User);
int SwitchGroup(const char *Group);

//returns a string pointing to a users home directory. DO NOT FREE THIS STRING. Take a copy of it, as it's
//an internal buffer and will change on the next call to this function
const char *GetUserHomeDir(const char *User);


//returns a string pointing to the CURRENT USER'S home directory.
// DO NOT FREE THIS STRING. Take a copy of it, as it's
//an internal buffer and will change on the next call to this function
const char *GetCurrUserHomeDir();


#ifdef __cplusplus
}
#endif


#endif


