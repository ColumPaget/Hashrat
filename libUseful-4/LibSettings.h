/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SETTINGS
#define LIBUSEFUL_SETTINGS

#include "Vars.h"

/*
These functions provide an interface for setting variables that are used by libUseful itself
or that are default values used by functions.

Possible variables are:

LibUseful:Version      This is set automatically when any of these functions are first called
LibUseful:BuildTime    This is set automatically when any of these functions are first called.

Error:Debug            If this is set to 'yes', 'y' or 'true' (case insensitive) then RaiseError
                       will print out debug statements.

Error:Silent           If this is set to 'yes', 'y' or 'true' (case insensitive) then RaiseError
                       won't print anything to stderr

Error:Syslog           If this is set to 'yes', 'y' or 'true' (case insensitive) then RaiseError
                       will send messages to the system logger via syslog

Error:IgnoreIP         If this is set to 'yes', 'y' or 'true' (case insensitive) then errors will
                       not be raised when network connections fail. This can be overriden on a
                       per-connection basis by passing the 'E' flag to STREAMOpen or the
                       CONNECT_ERROR flag to lower level functions

SwitchUserAllowFail    The SwitchUser function normally kills our process if it fails. Setting
                       this to 'yes', 'y' or 'true' (case insensitive) will allow the program to
                       continue even if switching UID fails

SwitchGroupAllowFail   The SwitchGroup function normally kills our process if it fails. Setting
                       this to 'yes', 'y' or 'true' will allow the program to continue even if
                       switching GID fails


HTTP:Debug             set this to 'true' or 'y' (case insensitive) to get debugging printed to
                       stderr for all HTTP connections

HTTP:NoCookies         set this to 'true' or 'y' (case insensitive) to disable cookie suppport
                       on all HTTP connections

HTTP:NoCompression     set this to 'true' or 'y' (case insensitive) to disable compression
                       on all HTTP connections

HTTP:NoRedirect        set this to 'true' or 'y' (case insensitive) to disable internal handling
                       of redirects on all HTTP connections

HTTP:NoCache           set this to 'true' or 'y' (case insensitive) to prevent servers sending
                       cached copies of pages all HTTP connections

HTTP:UserAgent         set default UserAgent value to be used by HTTP connections

SSL:Library            openssl library that libUseful is linked against. This is set on first use
                       of ssl connections or call to 'SSLAvailable' and has the format:

												 name:version:build date:cflags

                       currently openssl is the only supported library


SSL:Level              minimum SSL/TLS version to use on SSL connections. possible values are

                         ssl    ssl version 3
                         tls    tls version 1
                         tls1.1 tls version 1.1
												 tls1.2 tls version 1.2

                       currently 'tls' is the default


SSL:PermittedCiphers   openssl-format list of permitted ciphers (cipher names separated by :)


SSL:CertFile          set default certificate file to be offered by server-side ssl/tls connections
SSL:KeyFile           set default key file to be offered by server-side ssl/tls connections
SSL:VerifyCertDir     set directory full of C.A. certificates to be used in certificate verification
SSL:VerifyCertFile    set path to file containing a concatanated list of C.A. certificates to be
                       used in certificate verification


Net:Timeout           set default timeout for all network connections
TCP:keep_alives       use tcp keepalives for all network connections

Unicode:NamesFile     path to file that maps names->unicode code points
*/




#define  LU_ATEXIT_REGISTERED 1
#define  LU_CONTAINER         2

extern int LibUsefulFlags;

#ifdef __cplusplus
extern "C" {
#endif


//Get List of values
ListNode *LibUsefulValuesGetHead();

//set get values
void LibUsefulSetValue(const char *Name, const char *Value);
const char *LibUsefulGetValue(const char *Name);

//get a value as a boolean. This treats a value set to 'yes' 'y' or 'true' as true regardless
//of case (so uppercase or mixed-case works too). Any other value is false
int LibUsefulGetBool(const char *Name);

int LibUsefulGetInteger(const char *Name);

//this function gets called at exit to do certain cleaning up. It's no concern of the user.
//nothing to see here, move along
void LibUsefulAtExit();

#ifdef __cplusplus
};
#endif


#endif
