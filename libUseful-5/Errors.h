/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_ERRORS_H
#define LIBUSEFUL_ERRORS_H

#include <stdint.h>
#include "List.h"

#define ERRFLAG_ERRNO 1
#define ERRFLAG_DEBUG 2
#define ERRFLAG_CLEAR 4
#define ERRFLAG_NOTTY 8


/*
The libUseful error system builds a list of errors as the occur. This way, if multiple errors happen within
a function (e.g. multiple files fail to open) none of these errors are 'lost' when the next one happens. Errors
leave the queue when they are more than 2 seconds old, and can also be cleared by calling 'ErrorsClear'. They
can be accessed by calling 'ErrorsGet' which returns a list of TError objects. Normal use is that a function
returns a failure value, and the code then calls 'ErrorsGet' and steps through the errors, displaying them
to the user or whatever.

Errors are injected into the list with RaiseError like this:

	RaiseError(ERRFLAG_ERRNO, "ServiceConnect", "host=%s port=%d",Host, Port);

	the first argument, 'flags' can take a bitmask of of the above defined ERRFLAG_ values

	the second argument 'where' is a user defined string specifying where this error occurred, or what was being
  done when it occurred

	the third argument is a printf-style format string, and arguments after that are handled printf-style

	RaiseError grabs information about which sourcecode file, and which line in that file, it was called at.
	It adds a 'TError' object to the errors list with all the information in. It prints an error message to stderr,
	depending on some conditions mentioned below.

	If the flag ERRFLAG_ERRNO is passed then the 'errval' member of TError will contain the errno value that was in
	the system when RaiseError was called. Also, if RaiseError prints out an error, then the string description of
  errno, as obtained from strerror will be appended to the output message.

	By default RaiseError will not print out to stderr if stderr is not a tty, in order to prevent sending errors
	up pipes and sockets to applications that aren't expecting them. If you want raise error to print even when
	stderr is not a tty then either pass the ERRFLAG_NOTTY value, like this

	RaiseError(ERRFLAG_ERRNO|ERRFLAG_NOTTY, "ServiceConnect", "host=%s port=%d",Host, Port);

	or, if you don't want to have to do this every time you call RaiseError, then you can achieve the same by
	setting the libUseful internal boolean value 'Error:notty', like this:

	ListUsefulSetValue("Error:notty","Y")

	If the flag ERRFLAG_DEBUG is set then RaiseError won't print anything out by default. So this line:

	RaiseError(ERRFLAG_DEBUG, "ServiceConnect", "host=%s port=%d",Host, Port);

	Wouldn't produce any output normally.  This allows you to have debugging lines that can be switched on as 
	needed. There's two ways to activate debugging. Firstly there's the environment variable 'LIBUSEFUL_DEBUG'. 
	When this environment variable exists and is non-empty  then RaiseError calls will print out, even if they're
	passed ERRFLAG_DEBUG. Secondly there's the libuseful internal value 'Error:Debug'. When this has been set to 
	'true' (or 'y' or 'yes') with LibUsefulSetValue, then again all debugging lines will print out.

	If ERRFLAG_DEBUG is passed then nothing is EVER added to the ErrorList, the 'error' is just a debugging line
	and it's assumed that the program continues normally.

	If the libUseful internal value 'Error:Silent' is set true/yes/y with ListUsefulSetValue then RaiseError never
  prints errors to stderr, regardless of any other config.

	If  the value 'Error:Syslog' is set true/yes/y then error and debug messages are sent to the system logger via syslog
*/


typedef struct
{
    uint64_t when;
    char *where;
    char *file;
    char *msg;
    int line;
    int errval;
} TError;


#ifdef __cplusplus
extern "C" {
#endif


#define RaiseError(flags, where, fmt, ...) InternalRaiseError(flags, where, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

//clears the list of errors
void ErrorsClear();

//returns a list of TError objects
ListNode *ErrorsGet();


//this function isn't supposed to be used by you! You never saw this!
void InternalRaiseError(int flags, const char *where, const char *file, int line, const char *FmtStr, ...);


#ifdef __cplusplus
}
#endif


#endif
