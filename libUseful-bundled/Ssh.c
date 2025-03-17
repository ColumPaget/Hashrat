#include "Expect.h"
#include "Ssh.h"
#include "Pty.h"
#include "SpawnPrograms.h"


#define SSH_CANON_PTY 1  //communicate to ssh program over a canonical pty (honor ctrl-d, ctrl-c etc)
#define SSH_NO_ESCAPE 2  //disable the 'escape character'
#define SSH_COMPRESS  4  //use -C to compress ssh traffic



static int SSHParseFlags(const char *Token)
{
    const char *ptr;
    int Flags=0;

    for (ptr=Token; *ptr != '\0'; ptr++)
    {
        switch (*ptr)
        {
        case 'a':
            Flags |= STREAM_APPEND;
        case 'r':
            Flags |= SF_RDONLY;
        case 'w':
            Flags |= SF_WRONLY;
        case 'l':
            Flags |= SF_LIST;
        }
    }

    return(Flags);
}


static int SSHParseConfig(const char *Config, char **BindAddress, char **ConfigFile)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    int Flags=0;

    ptr=GetToken(Config, "\\S", &Value, 0);

    //if first item is a name=value, then there's no flags, so all of Config is name=value pairs
    if (! strchr(Value, '=')) Flags=SSHParseFlags(Value);
    else ptr=Config;


    ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        //we could add options here like 'bind address'
        //or specify encryption/key algorithms, but
        //unfortunately that will mean rethinking SSHConnect
        //as it has no way of passing such arguments at current


        if ( (strcasecmp(Name, "bind")==0)  && BindAddress) *BindAddress=CopyStr(*BindAddress, Value);
        if ( (strcasecmp(Name, "config")==0)  && ConfigFile) *ConfigFile=CopyStr(*ConfigFile, Value);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);

    return(Flags);
}


STREAM *SSHConnect(const char *Host, int Port, const char *User, const char *Pass, const char *Command, const char *Config)
{
    ListNode *Dialog;
    char *Tempstr=NULL, *KeyFile=NULL, *Token=NULL, *RemoteCmd=NULL, *TTYConfigs=NULL, *ConfigFile=NULL, *BindAddress=NULL;
    const char *ptr;
    STREAM *S;
    int IsTunnel=FALSE;
    int Flags, SshFlags=0;

    Flags=SSHParseConfig(Config, &BindAddress, &ConfigFile);


    //translate stream flags into specific SSH configurations
    if (Flags & SF_COMPRESSED) SshFlags |= SSH_COMPRESS;

    if (Flags & SF_WRONLY) SshFlags |= SSH_CANON_PTY;
    else if (Flags & STREAM_APPEND) SshFlags |= SSH_CANON_PTY | SSH_NO_ESCAPE;
    else if (Flags & SF_LIST) SshFlags |= SSH_CANON_PTY | SSH_NO_ESCAPE;



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

    if (SshFlags & SSH_NO_ESCAPE) Tempstr=MCatStr(Tempstr, "-e none ", NULL);
    if (SshFlags & SSH_COMPRESS) Tempstr=MCatStr(Tempstr, "-C ", NULL);

    ptr=GetToken(Command, "\\S", &Token, 0);
    while (ptr)
    {
        if (CompareStr(Token,"none")==0) Tempstr=CatStr(Tempstr, "-N ");
        else if (strncmp(Token, "stdin:",6)==0) Tempstr=MCatStr(Tempstr,"-W ", Token+6, NULL);
        else if (strncmp(Token, "jump:",5)==0) Tempstr=MCatStr(Tempstr,"-J ", Token+5, NULL);
        else if (strncmp(Token, "tunnel:",7)==0)
        {
            Tempstr=MCatStr(Tempstr,"-oExitOnForwardFailure=yes -L ", Token+7, NULL);
            IsTunnel=TRUE;
        }
        else if (strncmp(Token, "proxy:",6)==0)
        {
            Tempstr=MCatStr(Tempstr,"-oExitOnForwardFailure=yes -D ", Token+6, NULL);
            IsTunnel=TRUE;
        }
        else RemoteCmd=MCatStr(RemoteCmd, Token, " ", NULL);

        ptr=GetToken(ptr, "\\S", &Token, 0);
    }

    if (StrValid(BindAddress)) Tempstr=MCatStr(Tempstr, "-b \"", BindAddress, "\" ", NULL);
    if (StrValid(ConfigFile)) Tempstr=MCatStr(Tempstr, "-E \"", ConfigFile, "\" ", NULL);

    if (StrValid(RemoteCmd)) Tempstr=MCatStr(Tempstr, " \"", RemoteCmd, "\" ", NULL);


    //Setup configuration of the connection to the 'ssh' command
    //This is done over a psuedo-terminal (pty)
    //periodically something causes me to remove the 'pty' settings
    //but then password auth is broken.
    //this comment is so I'm aware of that the next time I think of removing 'pty'
    TTYConfigs=CopyStr(TTYConfigs, "pty crlf stderr2null nosig");

    //if we are writing to a file on the remote server then we need some way
    //to tell it 'end of file'. We can't just close the connection, as we
    //may not have sent all the data. For this one situation we use canonical
    //pty settings, so we can use the 'cntrl-d' control character
    if (SshFlags & SSH_CANON_PTY) TTYConfigs=CatStr(TTYConfigs, " canon");

    TTYConfigs=CatStr(TTYConfigs, " noshell setsid");

    S=STREAMSpawnCommand(Tempstr, TTYConfigs);
    if (S)
    {
        S->Path=MCopyStr(S->Path, "ssh:", Host, NULL);
        S->Type=STREAM_TYPE_SSH;
        if (StrValid(User) && (! StrValid(KeyFile)))
        {
            Dialog=ListCreate();
            ExpectDialogAdd(Dialog, "Are you sure you want to continue connecting (yes/no)?", "yes\n", DIALOG_OPTIONAL);
            ExpectDialogAdd(Dialog, "Permission denied", "", DIALOG_OPTIONAL | DIALOG_FAIL);
            Tempstr=MCopyStr(Tempstr,User,"\n",NULL);
            ExpectDialogAdd(Dialog, "ogin:", Tempstr, DIALOG_OPTIONAL);
            Tempstr=MCopyStr(Tempstr,Pass,"\n",NULL);
            ExpectDialogAdd(Dialog, "assword:", Tempstr, DIALOG_END);
            STREAMExpectDialog(S, Dialog, 0);
            ListDestroy(Dialog,ExpectDialogDestroy);
        }

        //if we've started up an ssh to do tunneling via '-L <port>:<ip>:<port>' then we need to
        //mark it's pid to be killed when this connection closes, otherwise we'll leak ssh procs.
        if (IsTunnel)
        {
            STREAMSetValue(S, "HelperPID", STREAMGetValue(S, "PeerPID"));
        }
    }

    DestroyString(Tempstr);
    DestroyString(KeyFile);
    DestroyString(TTYConfigs);
    DestroyString(RemoteCmd);
    DestroyString(ConfigFile);
    DestroyString(BindAddress);
    DestroyString(Token);

    return(S);
}



//Open an ssh stream in read, write or execute mode, depending on read flags
//This is called from STREAMOpen, where the 'r', 'w' or 'x' in the 2nd arg
//get translated to:
//'r' -> SF_RDONLY (read a file from remote server)
//'w' -> SF_WRONLY (write a file to remote server)
//'a' -> SF_APPEND (append to a file to remote server)
//'l' list files on remote server
//'x' or anything else runs a command on the remote server

STREAM *SSHOpen(const char *Host, int Port, const char *User, const char *Pass, const char *iPath, const char *Config)
{
    char *Tempstr=NULL, *Path=NULL;
    const char *ptr;
    int Flags;
    STREAM *S;

    Flags=SSHParseConfig(Config, NULL, NULL);

    if (iPath)
    {
        ptr=iPath;
        if (*ptr=='/') ptr++;
        else if (strncmp(ptr, "/./", 3)==0) ptr+=3;
        else if (strncmp(ptr, "/~/", 3)==0) ptr+=3;
    }
    else ptr="";

    //if SF_RDONLY is set, then we treat this as a 'file get',
    // we cannot do both read and write to a file over ssh
    if (Flags & SF_RDONLY)
    {
        Tempstr=QuoteCharsInStr(Tempstr, ptr, "    ()");
        Path=MCopyStr(Path, "cat ", Tempstr, NULL);
    }
    else if (Flags & SF_WRONLY)
    {
        Tempstr=QuoteCharsInStr(Tempstr, ptr, "    ()");
        Path=MCopyStr(Path, "cat - > ", Tempstr, NULL);
    }
    else if (Flags & STREAM_APPEND)
    {
        Tempstr=QuoteCharsInStr(Tempstr, ptr, "    ()");
        Path=MCopyStr(Path, "cat - >> ", Tempstr, NULL);
    }
    else if (Flags & SF_LIST)
    {
        Tempstr=QuoteCharsInStr(Tempstr, ptr, "    ()");
        Path=MCopyStr(Path, "ls -d ", Tempstr, NULL);
    }
    else Path=CopyStr(Path, ptr);

    S=SSHConnect(Host, Port, User, Pass, Path, Config);

    Destroy(Tempstr);
    Destroy(Path);

    return(S);
}


void SSHClose(STREAM *S)
{
//cntrl-d
    const char endchar=4;
    char *Tempstr=NULL;

    if (S->Flags & SF_WRONLY)
    {
//send cntrl-d
        STREAMWriteBytes(S, &endchar, 1);
        STREAMFlush(S);
        Tempstr=STREAMReadDocument(Tempstr, S);
    }

    Destroy(Tempstr);
}
