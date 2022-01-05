#include "Errors.h"
#include "includes.h"
#include <errno.h>

static ListNode *Errors=NULL;

void ErrorDestroy(void *p_Err)
{
    TError *Err;

    Err=(TError *) p_Err;
    DestroyString(Err->where);
    DestroyString(Err->file);
    DestroyString(Err->msg);
    free(Err);
}


void ErrorsClear()
{
    ListDestroy(Errors, ErrorDestroy);
}

ListNode *ErrorsGet()
{
    return(Errors);
}


static bool ErrorOutputWanted(int flags)
{
    bool result=TRUE;

//if stderr isn't a tty then don't print, unless ERRFLAG_NOTTY is set
//(this is used to disable this check when we consider syslog debugging too)
    if (
        (! (flags & ERRFLAG_NOTTY)) &&
        (! isatty(2))
    )
    {
        if (! LibUsefulGetBool("Error:notty")) result=FALSE;
    }

//if this error is a debug error, then don't print unless either LIBUSEFUL_DEBUG environment variable is set
//or the libUseful:Debug internal value is set
    if (flags & ERRFLAG_DEBUG)
    {
        result=FALSE;
        if (StrValid(getenv("LIBUSEFUL_DEBUG"))) result=TRUE;
        if (LibUsefulGetBool("libUseful:Debug")) result=TRUE;
    }

    return(result);
}


void InternalRaiseError(int flags, const char *where, const char *file, int line, const char *FmtStr, ...)
{
    char *Tempstr=NULL;
    va_list args;
    const char *ptr="";
    int errno_save;
    ListNode *Curr, *Next;;
    uint64_t Now;
    TError *Err;

    if (flags & ERRFLAG_CLEAR) ErrorsClear();

    errno_save=errno;
    va_start(args,FmtStr);
    Tempstr=VFormatStr(Tempstr,FmtStr,args);
    va_end(args);

    if (! Errors) Errors=ListCreate();
    if (flags & ERRFLAG_ERRNO) ptr=strerror(errno_save);

    //first consider if we want to print out a message on stderr
    if (ErrorOutputWanted(flags))
    {
        if (! LibUsefulGetBool("Error:Silent")) fprintf(stderr,"DEBUG: %s %s:%d %s. %s\n", where, file, line, Tempstr, ptr);
    }

    //now consider if we want to syslog an error message. Pass ERRFLAG_NOTTY as it doesn't matter if stderr is not a
    //tty for this case
    if (ErrorOutputWanted(flags | ERRFLAG_NOTTY))
    {
        if (LibUsefulGetBool("Error:Syslog")) syslog(LOG_ERR,"DEBUG: %s %s:%d %s. %s", where, file, line, Tempstr, ptr);
    }

    //if this Error is just debugging, then never add it to the list of errors
    if (! (flags & ERRFLAG_DEBUG))
    {
        Now=GetTime(TIME_MILLISECS);
        Err=(TError *) calloc(1,sizeof(TError));
        Err->errval=errno_save;
        Err->when=Now;
        Err->where=CopyStr(Err->where, where);
        Err->file=CopyStr(Err->file, file);
        Err->line=line;
        Err->msg=CopyStr(Err->msg, Tempstr);

        Curr=ListInsertItem(Errors,  Err);
        Curr=ListGetNext(Curr);
        while (Curr)
        {
            Next=ListGetNext(Curr);
            Err=(TError *) Curr->Item;
            if ((Now - Err->when) > 2000)
            {
                ListDeleteNode(Curr);
                ErrorDestroy(Err);
            }
            Curr=Next;
        }
    }


    DestroyString(Tempstr);
}



