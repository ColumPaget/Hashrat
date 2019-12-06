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

    if (flags & ERRFLAG_DEBUG)
    {
        ptr=getenv("LIBUSEFUL_DEBUG");
        if (StrEnd(ptr))
        {
            if (LibUsefulGetBool("libUseful:Debug")) ptr="y";
        }

        if (StrValid(ptr))
        {
            if (! LibUsefulGetBool("Error:Silent")) fprintf(stderr,"DEBUG: %s %s:%d %s. %s\n", where, file, line, Tempstr, ptr);
            if (LibUsefulGetBool("Error:Syslog")) syslog(LOG_ERR,"DEBUG: %s %s:%d %s. %s", where, file, line, Tempstr, ptr);
        }
    }
    else
    {
        Now=GetTime(TIME_MILLISECS);

        if (! LibUsefulGetBool("Error:Silent")) fprintf(stderr,"ERROR: %s %s:%d %s. %s\n", where, file, line, Tempstr, ptr);
        if (LibUsefulGetBool("Error:Syslog")) syslog(LOG_ERR,"ERROR: %s %s:%d %s. %s", where, file, line, Tempstr, ptr);


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



