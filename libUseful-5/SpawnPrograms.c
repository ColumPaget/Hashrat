#include "SpawnPrograms.h"
#include "Process.h"
#include "Log.h"
#include "Pty.h"
#include "SecureMem.h"
#include "Stream.h"
#include "String.h"
#include "Errors.h"
#include "FileSystem.h"
#include <sys/ioctl.h>
#include <sys/wait.h>

#define SPAWN_COMBINE_STDERR 1
#define SPAWN_STDOUT_NULL 2
#define SPAWN_STDERR_NULL 4

int SpawnParseConfig(const char *Config)
{
    const char *ptr;
    char *Token=NULL;
    int Flags=0;

    ptr=GetToken(Config," |,",&Token,GETTOKEN_MULTI_SEP);
    while (ptr)
    {
        if (strcasecmp(Token,"+stderr")==0) Flags |= SPAWN_COMBINE_STDERR;
        else if (strcasecmp(Token,"stderr2null")==0) Flags |= SPAWN_STDERR_NULL;
        else if (strcasecmp(Token,"stdout2null")==0) Flags |= SPAWN_STDOUT_NULL;
        else if (strcasecmp(Token,"errnull")==0) Flags |= SPAWN_STDERR_NULL;
        else if (strcasecmp(Token,"outnull")==0) Flags |= SPAWN_STDOUT_NULL | SPAWN_STDERR_NULL;

        ptr=GetToken(ptr," |,",&Token,GETTOKEN_MULTI_SEP);
    }

    DestroyString(Token);

    return(Flags);
}




//This is the function we call in the child process for 'SpawnCommand'
int BASIC_FUNC_EXEC_COMMAND(void *Command, int Flags)
{
    int result;
    char *Token=NULL, *FinalCommand=NULL, *ExecPath=NULL;
    char **argv;
    const char *ptr;
    int max_arg=100;
    int i;

    if (Flags & SPAWN_TRUST_COMMAND) FinalCommand=CopyStr(FinalCommand, (char *) Command);
    else FinalCommand=MakeShellSafeString(FinalCommand, (char *) Command, 0);

    StripTrailingWhitespace(FinalCommand);

    if (Flags & SPAWN_NOSHELL)
    {
        argv=(char **) calloc(max_arg+10,sizeof(char *));
        ptr=FinalCommand;
        ptr=GetToken(FinalCommand,"\\S",&Token,GETTOKEN_QUOTES);
        ExecPath=FindFileInPath(ExecPath,Token,getenv("PATH"));
        i=0;

        if (! (Flags & SPAWN_ARG0))
        {
            argv[0]=CopyStr(argv[0],ExecPath);
            i=1;
        }

        for (; i < max_arg; i++)
        {
            ptr=GetToken(ptr,"\\S",&Token,GETTOKEN_QUOTES);
            if (! ptr) break;
            argv[i]=CopyStr(argv[i],Token);
        }

        execv(ExecPath, argv);
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
        //we must handle creds store straight away, because it's memory is likely configured
        //with SMEM_NOFORK and thus the memory is invalid on fork
        CredsStoreOnFork();
        if (StrValid(Config))
        {
            //if any of the process configs we asked for failed, then quit
            if (ProcessApplyConfig(Config) & PROC_SETUP_FAIL) _exit(1);
        }
    }
    return(pid);
}


pid_t xforkio(int StdIn, int StdOut, int StdErr)
{
    pid_t pid;
    int fd;

    pid=xfork("");
    if (pid==0)
    {
        if (StdIn !=0)
        {
            if (StdIn == -1) fd_remap_path(0, "/dev/null", O_RDONLY);
            else fd_remap(0, StdIn);
        }

        if (StdOut !=1)
        {
            if (StdOut == -1) fd_remap_path(1, "/dev/null", O_WRONLY);
            else fd_remap(1, StdOut);
        }

        if (StdErr !=2)
        {
            if (StdErr == -1) fd_remap_path(2, "/dev/null", O_WRONLY);
            else fd_remap(2, StdErr);
        }
    }

    return(pid);
}





static void InternalSwitchProgram(const char *CommandLine, const char *Config)
{
    int Flags;

    Flags=ProcessApplyConfig(Config);
    if (! (Flags & PROC_SETUP_FAIL)) BASIC_FUNC_EXEC_COMMAND((void *) CommandLine, Flags);
}

void SwitchProgram(const char *CommandLine, const char *Config)
{
    int Flags;

    Flags=ProcessApplyConfig(Config);
    if (! (Flags & PROC_SETUP_FAIL)) BASIC_FUNC_EXEC_COMMAND((void *) CommandLine, SPAWN_NOSHELL|Flags);
}




pid_t SpawnWithIO(const char *CommandLine, const char *Config, int StdIn, int StdOut, int StdErr)
{
    pid_t pid;

    pid=xforkio(StdIn,StdOut,StdErr);
    if (pid==0)
    {
        InternalSwitchProgram(CommandLine, Config);
        _exit(pid);
    }

    return(pid);
}


//This creates an STDIO pipe, unless 'ToNull' is true
static int PipeSpawnCreateStdOutPipe(const char *Type, int channel[2], int ToNull)
{
int fd=-1, result;

channel[0]=-1;
channel[1]=-1;

//if we ask for this to be set to null, then we map leave fd set to -1
//which maps to /dev/null in xforkio
if (! ToNull) 
{
	result=pipe(channel);
  if (result==0) 
	{
	  if (strcmp(Type, "stdin")==0) fd=channel[0];
		else fd=channel[1];
	}
  else RaiseError(ERRFLAG_ERRNO, "PipeSpawnFunction", "Failed to create pipe for %s", Type);
}

return(fd);
}


/* This creates a child process that we can talk to using a couple of pipes*/
pid_t PipeSpawnFunction(int *infd, int *outfd, int *errfd, BASIC_FUNC Func, void *Data, const char *Config)
{
    pid_t pid;
    //default these to stdin, stdout and stderr and then override those later
    int c1=0, c2=1, c3=2;
    int channel1[2], channel2[2], channel3[2];
    int result;
    int Flags=0;

    Flags=SpawnParseConfig(Config);
    if (infd) c1=PipeSpawnCreateStdOutPipe("stdin", channel1, 0);
    if (outfd) c2=PipeSpawnCreateStdOutPipe("stdout", channel2, Flags & SPAWN_STDOUT_NULL);

    if (errfd) c3=PipeSpawnCreateStdOutPipe("stderr", channel3, Flags & SPAWN_STDERR_NULL);
    else if (Flags & SPAWN_COMBINE_STDERR) c3=c2;

    pid=xforkio(c1, c2, c3);
    if (pid==0)
    {
        /* we are the child, so we close the appropriate sites of the pipe for our connection */
        if (infd) close(channel1[1]);
        if (outfd) close(channel2[0]);
        if (errfd) close(channel3[0]);

        //if Func is NULL we effectively do a fork, rather than calling a function we just
        //continue exectution from where we were
        Flags=ProcessApplyConfig(Config);
        if (Func)
        {
            if (! (Flags & PROC_SETUP_FAIL)) Func(Data, Flags);
            exit(0);
        }
    }
    else // This is the parent process, not the spawned child
    {
        /* we close the appropriate halves of the link */
        if (infd)
        {
            close(channel1[0]);
            *infd=channel1[1];
						if (*infd == -1) *infd=open("/dev/null", O_RDWR);
        }
        if (outfd)
        {
            close(channel2[1]);
            *outfd=channel2[0];
						if (*outfd == -1) *outfd=open("/dev/null", O_RDWR);
        }

        if (errfd)
        {
            close(channel3[1]);
            //Yes, we can pass an integer value as errfd, even though errfd is an int *.
            //This is probably a bad idea, and will likely be changed in future releases
            *errfd=channel3[0];
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
    pid_t pid=-1, ConfigFlags=0;
    int tty, pty, i;

    if (PseudoTTYGrab(&pty, &tty, Flags))
    {
        pid=xforkio(tty, tty, tty);
        if (pid==0)
        {
            close(pty);

            ProcessSetControlTTY(tty);
            setsid();

            ///now that we've dupped it, we don't need to keep it open
            //as it will be open on stdin/stdout
            close(tty);

            ConfigFlags=ProcessApplyConfig(Config);

            //if Func is NULL we effectively do a fork, rather than calling a function we just
            //continue exectution from where we were
            if (Func)
            {
                if (! (ConfigFlags & PROC_SETUP_FAIL)) Func((char *) Data, ConfigFlags);
                _exit(0);
            }
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
        pid=PipeSpawnFunction(&to_fd, &from_fd, iptr, Func, Data, Config);
    }

    if (pid > 0)
    {
        S=STREAMFromDualFD(from_fd, to_fd);
        /*
        if (waitpid(pid, NULL, WNOHANG) < 1)
        //sleep to allow spawned function time to exit due to startup problems
        usleep(250);
        //use waitpid to check process has not exited, if so then spawn stream
        else fprintf(stderr, "ERROR: Subprocess exited: %s %s\n", strerror(errno), Data);
        */
    }

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
    char *Token=NULL, *ExecPath=NULL;
    const char *ptr;
    STREAM *S=NULL;
    int UseShell=TRUE;

    //if 'noshell' exists in Config, then we want to do more checking (i.e. exe to be launched must exist on disk)
    //otherwise this could be a shell command that doesn't exist on disk
    ptr=GetToken(Config, "\\S", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        if (CompareStr(Token, "noshell")==0) UseShell=FALSE;
        ptr=GetToken(ptr, "\\S", &Token, GETTOKEN_QUOTES);
    }

    GetToken(Command, "\\S", &Token, GETTOKEN_QUOTES);
    ExecPath=FindFileInPath(ExecPath,Token,getenv("PATH"));

    //take a copy of this as it's going to be passed to another process and
    Token=CopyStr(Token, Command);
    if (UseShell || StrValid(ExecPath)) S=STREAMSpawnFunction(BASIC_FUNC_EXEC_COMMAND, (void *) Token, Config);

    Destroy(ExecPath);
    Destroy(Token);

    return(S);
}



int STREAMSpawnCommandAndPty(const char *Command, const char *Config, STREAM **CmdS, STREAM **PtyS)
{
    int pty, tty;
    int result=FALSE;
    char *Tempstr=NULL, *Args=NULL, *Token=NULL;
    const char *ptr;

    *PtyS=NULL;
    *CmdS=NULL;

    if (PseudoTTYGrab(&pty, &tty, TTYFLAG_PTY))
    {
        //handle situation where Config might be null
        if (StrValid(Config)) Tempstr=CopyStr(Tempstr, Config);
        else Tempstr=CopyStr(Tempstr, "rw");

        Args=FormatStr(Args, "%s setsid ctty=%d nosig ", Tempstr, tty);
        ptr=GetToken(Config, "\\S", &Token, 0);
        while (ptr)
        {
            if (strcmp(Token, "ptystderr")==0)
            {
                Tempstr=FormatStr(Tempstr, "stderr=%d", tty);
                Args=CatStr(Args, Tempstr);
            }
            ptr=GetToken(ptr, "\\S", &Token, 0);
        }

        Tempstr=MCopyStr(Tempstr, "cmd:", Command, NULL);
        *CmdS=STREAMOpen(Tempstr, Args);
        if (*CmdS)
        {
            *PtyS=STREAMFromFD(pty);
            STREAMSetTimeout(*PtyS, 1000);
            result=TRUE;
        }
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Args);

    return(result);
}


int STREAMSpawnWaitExit(STREAM *S)
{
    char *Tempstr=NULL;
    pid_t pid;
    int result, status, exited=-1;

    STREAMCommit(S);
    pid=atoi(STREAMGetValue(S, "PeerPID"));
    STREAMSetTimeout(S, 10);
    Tempstr=SetStrLen(Tempstr, 4096);
    result=STREAMReadBytes(S, Tempstr, 4096);
    while (result != STREAM_CLOSED)
    {
        exited=waitpid(pid, &status, WNOHANG);
        if (exited == pid) break;
        result=STREAMReadBytes(S, Tempstr, 4096);
    }

    if (exited != pid) waitpid(pid, &status, 0);

    Destroy(Tempstr);
    return(status);
}
