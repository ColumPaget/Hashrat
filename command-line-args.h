
#ifndef HASHRAT_COMMANDLINE_H 
#define HASHRAT_COMMANDLINE_H 

#include "common.h"

HashratCtx *CommandLineParseArgs(int argc,char *argv[]);
void CommandLinePrintUsage();

#endif
