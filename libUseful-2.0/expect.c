#include "includes.h"
#include "expect.h"


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



int STREAMExpectDialog(STREAM *S, ListNode *ExpectDialogs)
{
int len=0, inchar;
ListNode *Curr;
TExpectDialog *ExpectDialog;

inchar=STREAMReadChar(S);
while (inchar !=EOF)
{
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
          if (ExpectDialog->Reply) STREAMWriteLine(ExpectDialog->Reply,S);
          if (ExpectDialog->Flags & DIALOG_END) return(TRUE);
          if (ExpectDialog->Flags & DIALOG_FAIL) return(FALSE);
        }
      }

			if (! (ExpectDialog->Flags & DIALOG_OPTIONAL)) break;
			}
      Curr=ListGetNext(Curr);
    }
  }
inchar=STREAMReadChar(S);
}

return(FALSE);
}


int STREAMExpectAndReply(STREAM *S, char *Expect, char *Reply)
{
int match=0, len=0, inchar;

len=StrLen(Expect);
inchar=STREAMReadChar(S);
while (inchar !=EOF)
{
  if (inchar > 0)
  {
  //if the current value does not equal where we are in the string
  //we have to consider whether it is the first character in the string
  if (Expect[match]!=inchar) match=0;

  if (Expect[match]==inchar)
  {
    match++;
    if (match==len)
    {
      if (Reply) STREAMWriteLine(Reply,S);
      return(TRUE);
    }
  }

  }
inchar=STREAMReadChar(S);
}

return(FALSE);
}


int STREAMExpectSilence(STREAM *S, int wait)
{
int inchar;
char *Tempstr=NULL;
int len=0, Timeout;

Timeout=S->Timeout;
S->Timeout=wait;
inchar=STREAMReadChar(S);
while (inchar  > 0)
{
  Tempstr=AddCharToBuffer(Tempstr,len,inchar);
  len++;
  inchar=STREAMReadChar(S);
}

S->Timeout=Timeout;

DestroyString(Tempstr);

return(FALSE);
}
