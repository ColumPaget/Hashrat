#include "Expect.h"
#include "Ssh.h"
#include "Pty.h"
#include "SpawnPrograms.h"

STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command)
{
    ListNode *Dialog;
    char *Tempstr=NULL, *KeyFile=NULL, *Token=NULL, *RemoteCmd=NULL;
		const char *ptr;
    STREAM *S;
    int val, i;


//If we are using the .ssh/config connection-config system then there won't be a username, and 'Host' will
//actually be a configuration name that names a config section with all details
    if (StrValid(User)) Tempstr=MCopyStr(Tempstr,"ssh -2 -T ",User,"@",Host, " ", NULL );
    else Tempstr=MCopyStr(Tempstr,"ssh -2 -T ",Host, " ", NULL );

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

		ptr=GetToken(Command, "\\S", &Token, 0);
		while (ptr)
    {
        if (strcmp(Token,"none")==0) Tempstr=CatStr(Tempstr, "-N ");
        else if (strncmp(Token, "tunnel:",7)==0) Tempstr=MCatStr(Tempstr,"-N -L ", Token+7, NULL);
        else if (strncmp(Token, "stdin:",6)==0) Tempstr=MCatStr(Tempstr,"-W ", Token+6, NULL);
        else if (strncmp(Token, "jump:",5)==0) Tempstr=MCatStr(Tempstr,"-J ", Token+5, NULL);
        else RemoteCmd=MCatStr(RemoteCmd, Token, " ", NULL);

				ptr=GetToken(ptr, "\\S", &Token, 0);
    }

    if (StrValid(RemoteCmd)) Tempstr=MCatStr(Tempstr, " \"", RemoteCmd, "\" ", NULL);
    Tempstr=CatStr(Tempstr, " 2> /dev/null");

//Never use TTYFLAG_CANON here

//periodically something causes me to remove pty from here, but then password auth is broken
//this comment is so I'm aware of that the next time I think of removing 'pty'
    S=STREAMSpawnCommand(Tempstr,"pty,crlf,ignsig,+stderr");
    if (S)
    {
        if (StrValid(User) && (! StrValid(KeyFile)))
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
    }


    DestroyString(Tempstr);
    DestroyString(KeyFile);
    DestroyString(RemoteCmd);
    DestroyString(Token);

    return(S);
}
