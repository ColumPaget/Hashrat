/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_CMDLINE_H
#define LIBUSEFUL_CMDLINE_H

#include "includes.h"

// handles parsing a command line, and safely handling iterating through
// the list of items. Thus, if there is an item that expects to have an
// argument, but the argument isn't there, CommandLineNext will just return ""

typedef struct
{
unsigned int argc;
unsigned int curr;
const char **argv;
} CMDLINE;


#ifdef __cplusplus
extern "C" {
#endif

//initiallize a CMDLINE object. Only needed if using a CMDLINE as a static object, rather
//than dynamically allocated with CommandLineParserCreate
void CommandLineParserInit(CMDLINE *CmdLine, int argc, char *argv[]);

//dynamically create a CMDLINE object. Either this or CommandLineParserInit are used, not both
CMDLINE *CommandLineParserCreate(int argc, char *argv[]);

//get current command-line argument
const char *CommandLineCurr(CMDLINE *CmdLine);

//get next command-line argument, advancing argument pointer
const char *CommandLineNext(CMDLINE *CmdLine);

//get next command-line argument, WITHOUT advancing argument pointer
const char *CommandLinePeek(CMDLINE *CmdLine);

//get first command-line argument
const char *CommandLineFirst(CMDLINE *CmdLine);

#ifdef __cplusplus
}
#endif


#endif
