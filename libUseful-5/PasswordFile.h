/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

// These functions relate to a password file that stores passwords
// in the form:
// <user>:<pass type>:<salt>:<credential>
//
// if 'pass type' is blank or is 'plain' then the password is stored as-is in plaintext
// otherwise 'pass type' will be a hash type, like md5, sha256 etc.
// For hash types a 20 character salt is generated and prepended to the password,
// and the resulting string is then hashed before being stored


#ifndef LIBUSEFUL_PASSWORD_FILE_H
#define LIBUSEFUL_PASSWORD_FILE_H


#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif


// Add an entry to the password file, replacing any previous entries for that user. Because it has
// to remove previous entries, this function builds an new password file, and then atomically moves
// it into place to replace the existing one. It will not remove any duplicate entries for other
// users.
int PasswordFileAdd(const char *Path, const char *PassType, const char *User, const char *Password);

// Add an entry to the password file, not replacing previous entries, to previous passwords can still be
// used. This does not require rebuilding the file, and thus may be more efficient than PasswordFileAdd
int PasswordFileAppend(const char *Path, const char *PassType, const char *User, const char *Password);

//check a users password matches the one stored in password file at 'Path'
int PasswordFileCheck(const char *Path, const char *User, const char *Password);

#ifdef __cplusplus
}
#endif




#endif
