#include "includes.h"
#include "Expect.h"


//Values for 'flags' that are not visible to the user
//These must not clash with any visible values
#define DIALOG_DONE 67108864


void ExpectDialogAdd(ListNode *ExpectDialogs, char *Expect, char *Reply, int Flags)
{
    TExpectDialog *ExpectDialog;

    ExpectDialog=(TExpectDialog *) calloc(1,sizeof(TExpectDialog));
    ExpectDialog->Expect=CopyStr(ExpectDialog->Expect,Expect);
    ExpectDialog->Reply=CopyStr(ExpectDialog->Reply,Reply);
    ExpectDialog->Flags=Flags;

    ListAddItem(ExpectDialogs,ExpectDialog);
}

void ExpectDialogDestroy(void *p_Item)
{
    TExpectDialog *ExpectDialog;

    ExpectDialog=(TExpectDialog *) p_Item;
    DestroyString(ExpectDialog->Expect);
    DestroyString(ExpectDialog->Reply);
    free(ExpectDialog);
}

int ExpectDialogsDone(ListNode *ExpectDialogs)
{
ListNode *Curr;
TExpectDialog *ExpectDialog;

Curr=ListGetNext(ExpectDialogs);
while (Curr)
{
ExpectDialog=(TExpectDialog *) Curr->Item;
if (! (ExpectDialog->Flags & (DIALOG_DONE | DIALOG_OPTIONAL))) return(FALSE);
Curr=ListGetNext(Curr);
}

return(TRUE);
}


int STREAMExpectDialog(STREAM *S, ListNode *ExpectDialogs, int Timeout)
{
    int inchar;
    ListNode *Curr;
		int SavedTimeout;
    TExpectDialog *ExpectDialog;

		SavedTimeout=S->Timeout;
		if (Timeout > 0) S->Timeout=Timeout;

    inchar=STREAMReadChar(S);
    while (inchar !=EOF)
    {
				if ((inchar == STREAM_NODATA) && (Timeout > 0)) break;
        if (inchar > 0)
        {
            Curr=ListGetNext(ExpectDialogs);
            while (Curr)
            {
                ExpectDialog=(TExpectDialog *) Curr->Item;
                if (! (ExpectDialog->Flags & DIALOG_DONE))
                {
                    //if the current value does not equal where we are in the string
                    //we have to consider whether it is the first character in the string
                    if (ExpectDialog->Expect[ExpectDialog->Match]!=inchar) ExpectDialog->Match=0;

                    if (ExpectDialog->Expect[ExpectDialog->Match]==inchar)
                    {
                        ExpectDialog->Match++;
                        if (ExpectDialog->Expect[ExpectDialog->Match]=='\0')
                        {
                            ExpectDialog->Match=0;
                            ExpectDialog->Flags |= DIALOG_DONE;
                            if (ExpectDialog->Reply) 
														{
															//this is to allow programs that clear their buffers before reading to do that without throwing away our reply
															usleep(10000);
															STREAMWriteLine(ExpectDialog->Reply,S);
														}
                            if (ExpectDialog->Flags & DIALOG_END)  { S->Timeout=SavedTimeout; return(TRUE);  }
                            if (ExpectDialog->Flags & DIALOG_FAIL) { S->Timeout=SavedTimeout; return(FALSE); }
                        }
                    }

                    if (! (ExpectDialog->Flags & DIALOG_OPTIONAL)) break;
                }
                Curr=ListGetNext(Curr);
            }
        }
        inchar=STREAMReadChar(S);
    }

    S->Timeout=SavedTimeout;

		//if we got timeout rather than EOF, that implies the command didn't complete
		//so we only return TRUE if we got EOF and all our dialogs were processed
		if ((inchar == EOF) && ExpectDialogsDone(ExpectDialogs)) return(TRUE);
    return(FALSE);
}


int STREAMExpectAndReply(STREAM *S, const char *Expect, const char *Reply)
{

    if (STREAMReadToString(S, NULL, NULL, Expect))
    {
        if (Reply)
        {
            STREAMWriteLine(Reply,S);
            STREAMFlush(S);
        }
        return(TRUE);
    }

    return(FALSE);
}


int STREAMExpectSilence(STREAM *S, int wait)
{
    int inchar;
    int len=0, SavedTimeout;

    SavedTimeout=S->Timeout;
    S->Timeout=wait;
    inchar=STREAMReadChar(S);
    while (inchar  > 0) inchar=STREAMReadChar(S);

    S->Timeout=SavedTimeout;

    return(FALSE);
}
