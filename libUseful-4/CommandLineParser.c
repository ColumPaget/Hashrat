#include "CommandLineParser.h"

void CommandLineParserInit(CMDLINE *CmdLine, int argc, char *argv[])
{
    if (argc < 0) CmdLine->argc=0;
    if (! argv) CmdLine->argc=0;
    else CmdLine->argc=argc;
    CmdLine->argv=(const char **) argv;
}


CMDLINE *CommandLineParserCreate(int argc, char *argv[])
{
    CMDLINE *CmdLine;

    CmdLine=(CMDLINE *) calloc(1,sizeof(CMDLINE));
    CommandLineParserInit(CmdLine, argc, argv);

    return(CmdLine);
}

const char *CommandLineCurr(CMDLINE *CmdLine)
{
    const char *ptr=NULL;

    if (CmdLine->curr < CmdLine->argc)
    {
        ptr=CmdLine->argv[CmdLine->curr];
    }

    return(ptr);
}



const char *CommandLineNext(CMDLINE *CmdLine)
{
    const char *ptr=NULL;

    if (CmdLine->curr < CmdLine->argc)
    {
        CmdLine->curr++;
        ptr=CmdLine->argv[CmdLine->curr];
    }

    return(ptr);
}

const char *CommandLineFirst(CMDLINE *CmdLine)
{
    CmdLine->curr=0;
    return(CommandLineNext(CmdLine));
}


const char *CommandLinePeek(CMDLINE *CmdLine)
{
    const char *ptr=NULL;

    if ((CmdLine->curr+1) < CmdLine->argc)
    {
        ptr=CmdLine->argv[CmdLine->curr+1];
    }

    return(ptr);
}

