#include "CGI.h"

char *CGIReadDocument(char *RetStr, STREAM *S)
{
    const char *ptr;

    ptr=getenv("CONTENT_LENGTH");
    if (StrValid(ptr))
    {
        S->Size=atoi(ptr);
    }

    RetStr=STREAMReadDocument(RetStr, S);
    return(RetStr);
}
