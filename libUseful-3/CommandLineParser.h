#ifndef LIBUSEFUL_CMDLINE_H
#define LIBUSEFUL_CMDLINE_H

#include "includes.h"

typedef struct
{
unsigned int argc;
unsigned int curr;
const char **argv;
} CMDLINE;


#ifdef __cplusplus
extern "C" {
#endif


void CommandLineParserInit(CMDLINE *CmdLine, int argc, char *argv[]);
CMDLINE *CommandLineParserCreate(int argc, char *argv[]);
const char *CommandLineCurr(CMDLINE *CmdLine);
const char *CommandLineNext(CMDLINE *CmdLine);

#ifdef __cplusplus
}
#endif


#endif
