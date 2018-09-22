#include "SpawnPrograms.h"
#include "Process.h"
#include "Log.h"
#include "Pty.h"
#include "Stream.h"
#include "String.h"
#include "Errors.h"
#include "FileSystem.h"
#include <sys/ioctl.h>


//This Function eliminates characters from a string that can be used to trivially achieve code-exec via the shell
char *MakeShellSafeString(char *RetStr, const char *String, int SafeLevel)
{
    char *Tempstr=NULL;
    char *BadChars=";|&`";

    if (SafeLevel==SHELLSAFE_BLANK)
    {
        Tempstr=CopyStr(RetStr,String);
        strmrep(Tempstr,BadChars,' ');
    }
    else Tempstr=QuoteCharsInStr(RetStr,String,BadChars);

    if (strcmp(Tempstr,String) !=0)
    {
        //if (EventCallback) EventCallback(String);
    }
    return(Tempstr);
}


//This is the function we call in the child process for 'SpawnCommand'
int BASIC_FUNC_EXEC_COMMAND(void *Command, int Flags)
{
    int result;
    char *Token=NULL, *FinalCommand=NULL, *ExecPath=NULL;
    char **argv;
    const char *ptr;
    int i;

    if (Flags & SPAWN_TRUST_COMMAND) FinalCommand=CopyStr(FinalCommand, (char *) Command);
    else FinalCommand=MakeShellSafeString(FinalCommand, (char *) Command, 0);

    StripTrailingWhitespace(FinalCommand);
    if (Flags & SPAWN_NOSHELL)
    {
        argv=(char **) calloc(101,sizeof(char *));
        ptr=FinalCommand;
        ptr=GetToken(FinalCommand,"\\S",&Token,GETTOKEN_QUOTES);
        ExecPath=FindFileInPath(ExecPath,Token,getenv("PATH"));
        i=0;

        if (! (Flags & SPAWN_ARG0))
        {
            argv[0]=CopyStr(argv[0],ExecPath);
            i=1;
        }

        for (; i < 100; i++)
        {
            ptr=GetToken(ptr,"\\S",&Token,GETTOKEN_QUOTES);
            if (! ptr) break;
            argv[i]=CopyStr(argv[i],Token);
        }

        execv(ExecPath,argv);
    }
    else result=execl("/bin/sh","/bin/sh","-c",(char *) Command,NULL);

    RaiseError(ERRFLAG_ERRNO, "Spawn", "Failed to execute '%s'",Command);
//We'll never get to here unless something fails!
    DestroyString(FinalCommand);
    DestroyString(ExecPath);
    DestroyString(Token);

    return(result);
}



pid_t xfork(const char *Config)
{
    pid_t pid;

    LogFileFlushAll(TRUE);
    pid=fork();
    if (pid==-1) RaiseError(ERRFLAG_ERRNO, "fork", "");
    if (pid==0)
    {
        ProcessApplyConfig(Config);
    }
    return(pid);
}


int xforkio(int StdIn, int StdOut, int StdErr)
{
    pid_t pid;
    int fd;

    pid=xfork("");
    if (pid==0)
    {
        if (StdIn > -1)
        {
            if (StdIn !=0)
            {
                close(0);
                dup(StdIn);
            }
        }
        else
        {
            fd=open("/dev/null",O_RDONLY);
            dup(fd);
            close(fd);
        }

        if (StdOut > -1)
        {
            if (StdOut !=1)
            {
                close(1);
                dup(StdOut);
            }
        }
        else
        {
            fd=open("/dev/null",O_WRONLY);
            dup(fd);
            close(fd);
        }

        if (StdErr > -1)
        {
            if (StdErr !=2)
            {
                close(2);
                dup(StdErr);
            }
        }
    }


    return(pid);
}





void SwitchProgram(const char *CommandLine, const char *Config)
{
    int Flags;

    Flags=ProcessApplyConfig(Config);
    BASIC_FUNC_EXEC_COMMAND((void *) CommandLine, SPAWN_NOSHELL|Flags);
}




pid_t SpawnWithIO(const char *CommandLine, const char *Config, int StdIn, int StdOut, int StdErr)
{
    pid_t pid;

    pid=xforkio(StdIn,StdOut,StdErr);
    if (pid==0)
    {
        SwitchProgram(CommandLine, Config);
        _exit(pid);
    }

    return(pid);
}


int Spawn(const char *ProgName, const char *Config)
{
    int pid;

    pid=xforkio(0,1,2);
    if (pid==0)
    {
        SwitchProgram(ProgName, Config);
        _exit(pid);
    }
    return(pid);
}


/* This creates a child process that we can talk to using a couple of pipes*/
pid_t PipeSpawnFunction(int *infd, int *outfd, int *errfd, BASIC_FUNC Func, void *Data, const char *Config)
{
    pid_t pid;
    int channel1[2], channel2[2], channel3[2], DevNull=-1;
    int Flags;

    if (infd) pipe(channel1);
    if (outfd) pipe(channel2);
    if (errfd) pipe(channel3);

    pid=xfork("");
    if (pid==0)
    {
        /* we are the child */
        if (infd) close(channel1[1]);
        else if (DevNull==-1) DevNull=open("/dev/null",O_RDWR);
        if (outfd) close(channel2[0]);
        else if (DevNull==-1) DevNull=open("/dev/null",O_RDWR);
        if (errfd) close(channel3[0]);
        else if (DevNull==-1) DevNull=open("/dev/null",O_RDWR);

        /*close stdin, stdout and stderr*/
        close(0);
        close(1);
        close(2);
        /*channel 1 is going to be our stdin, so we close the writing side of it*/
        if (infd) dup(channel1[0]);
        else dup(DevNull);
        /* channel 2 is stdout */
        if (outfd) dup(channel2[1]);
        else dup(DevNull);
        /* channel 3 is stderr */
        if (errfd)
        {
            //Yes, we can pass an integer value as errfd, even though it's an int *.
            //This is probably a bad idea, and will likely be changed in future releases
            //if (errfd==(int *) COMMS_COMBINE_STDERR) dup(channel2[1]);
            //else
            dup(channel3[1]);
        }
        else dup(DevNull);


        Flags=ProcessApplyConfig(Config);
        Func(Data, Flags);
        exit(0);
    }
    else // This is the parent process, not the spawned child
    {
        /* we close the appropriate halves of the link */
        if (infd)
        {
            close(channel1[0]);
            *infd=channel1[1];
        }
        if (outfd)
        {
            close(channel2[1]);
            *outfd=channel2[0];
        }
        if (errfd)
        {
            close(channel3[1]);
            //Yes, we can pass an integer value as errfd, even though errfd is an int *.
            //This is probably a bad idea, and will likely be changed in future releases
            //if (errfd != (int *) COMMS_COMBINE_STDERR) *errfd=channel3[0];
        }
    }

    return(pid);
}



pid_t PipeSpawn(int *infd,int  *outfd,int  *errfd, const char *Command, const char *Config)
{
    return(PipeSpawnFunction(infd,outfd,errfd, BASIC_FUNC_EXEC_COMMAND, (void *) Command, Config));
}




pid_t PseudoTTYSpawnFunction(int *ret_pty, BASIC_FUNC Func, void *Data, int Flags, const char *Config)
{
    pid_t pid=-1, ConfigFlags;
    int tty, pty, i;

    if (PseudoTTYGrab(&pty, &tty, Flags))
    {
        pid=xfork("");
        if (pid==0)
        {
            for (i=0; i < 3; i++) close(i);
            close(pty);

            setsid();
            ioctl(tty,TIOCSCTTY,0);

            dup(tty);
            dup(tty);
            dup(tty);

            ///now that we've dupped it, we don't need to keep it open
            //as it will be open on stdin/stdout
            close(tty);

            ConfigFlags=ProcessApplyConfig(Config);
            Func((char *) Data, ConfigFlags);
            _exit(0);
        }

        close(tty);
    }

    *ret_pty=pty;
    return(pid);
}


pid_t PseudoTTYSpawn(int *pty, const char *Command, const char *Config)
{
    return(PseudoTTYSpawnFunction(pty, BASIC_FUNC_EXEC_COMMAND, (void *) Command, TTYParseConfig(Config, NULL), Config));
}





STREAM *STREAMSpawnFunction(BASIC_FUNC Func, void *Data, const char *Config)
{
    int to_fd, from_fd, *iptr;
    pid_t pid=0;
    STREAM *S=NULL;
    char *Tempstr=NULL;
    int Flags=0;

    Flags=TTYParseConfig(Config, NULL);
    if (Flags & TTYFLAG_PTY)
    {
        pid=PseudoTTYSpawnFunction(&to_fd, Func, Data, Flags, Config);
        from_fd=to_fd;
    }
    else
    {
        iptr=NULL;
        //if (Flags & COMMS_COMBINE_STDERR) iptr=(int *) COMMS_COMBINE_STDERR;
        pid=PipeSpawnFunction(&to_fd, &from_fd, iptr, Func, Data, Config);
    }

    if (pid > 0) S=STREAMFromDualFD(from_fd, to_fd);
    if (S)
    {
        STREAMSetFlushType(S,FLUSH_LINE,0,0);
        Tempstr=FormatStr(Tempstr,"%d",pid);
        STREAMSetValue(S,"PeerPID",Tempstr);
        S->Type=STREAM_TYPE_PIPE;
    }

    DestroyString(Tempstr);
    return(S);
}


STREAM *STREAMSpawnCommand(const char *Command, const char *Config)
{
    return(STREAMSpawnFunction(BASIC_FUNC_EXEC_COMMAND, (void *) Command, Config));
}
