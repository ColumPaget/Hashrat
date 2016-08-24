#include "expect.h"
#include "ssh.h"
#include "pty.h"
#include "SpawnPrograms.h"

STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command)
{
ListNode *Dialog;
char *Tempstr=NULL, *KeyFile=NULL, *Token=NULL;
STREAM *S;
int val, i;

Tempstr=MCopyStr(Tempstr,"ssh -2 -T ",User,"@",Host, " ", NULL );

if (Port > 0)
{
	Token=FormatStr(Token," -p %d ",Port);
	Tempstr=CatStr(Tempstr,Token);
}

if (strncmp(Pass,"keyfile:",8)==0) 
{
	KeyFile=CopyStr(KeyFile, Pass+8);
	Tempstr=MCatStr(Tempstr,"-i ",KeyFile," ",NULL);
}

if (StrLen(Command))
{
	if (strcmp(Command,"none")==0) Tempstr=CatStr(Tempstr, "-N ");
	else Tempstr=MCatStr(Tempstr, "\"", Command, "\" ", NULL);
}
Tempstr=CatStr(Tempstr, " 2> /dev/null");

//Never use TTYFLAG_CANON here
S=STREAMSpawnCommand(Tempstr,"","", COMMS_BY_PTY|TTYFLAG_CRLF|TTYFLAG_IGNSIG);

printf("SSC: %d [%s]\n",S,Tempstr);

if (StrLen(KeyFile)==0)
{
  Dialog=ListCreate();
  ExpectDialogAdd(Dialog, "Are you sure you want to continue connecting (yes/no)?", "yes\n", DIALOG_OPTIONAL);
  ExpectDialogAdd(Dialog, "Permission denied", "", DIALOG_OPTIONAL | DIALOG_FAIL);
  Tempstr=MCopyStr(Tempstr,User,"\n",NULL);
  ExpectDialogAdd(Dialog, "ogin:", Tempstr, DIALOG_OPTIONAL);
  Tempstr=MCopyStr(Tempstr,Pass,"\n",NULL);
  ExpectDialogAdd(Dialog, "assword:", Tempstr, DIALOG_END);
  STREAMExpectDialog(S, Dialog);
  ListDestroy(Dialog,ExpectDialogDestroy);
}

/*
STREAMSetTimeout(S,100);

Tempstr=FormatStr(Tempstr,"okay %d %d\n",getpid(),time(NULL));
STREAMWriteString("echo ",S);
STREAMWriteLine(Tempstr,S);
STREAMFlush(S);

StripTrailingWhitespace(Tempstr);
for (i=0; i < 3; i++)
{
Token=STREAMReadLine(Token,S);
StripTrailingWhitespace(Token);

printf("SRL: [%s]\n",Token);
if ( StrLen(Token) && (strcmp(Tempstr,Token) ==0) ) break;
}

if (i==3)
{
  printf("Mismatch! [%s] [%s]\n",Token,Tempstr);
  STREAMClose(S);
  S=NULL;
}
*/

DestroyString(Tempstr);
DestroyString(KeyFile);
DestroyString(Token);

return(S);
}
