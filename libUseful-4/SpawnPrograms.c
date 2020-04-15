#include "SpawnPrograms.h"
#include "Process.h"
#include "Log.h"
#include "Pty.h"
#include "Stream.h"
#include "String.h"
#include "Errors.h"
#include "FileSystem.h"
#include <sys/ioctl.h>

#define SPAWN_COMBINE_STDERR 1

int SpawnParseConfig(const char *Config)
{
    const char *ptr;
    char *Token=NULL;
    int Flags=0;

    ptr=GetToken(Config," |,",&Token,GETTOKEN_MULTI_SEP);
    while (ptr)
    {
        if (strcasecmp(Token,"+stderr")==0) Flags |= SPAWN_COMBINE_STDERR;

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
        ProcessApplyConfig(Config);
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





void InternalSwitchProgram(const char *CommandLine, const char *Config)
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



/* This creates a child process that we can talk to using a couple of pipes*/
pid_t PipeSpawnFunction(int *infd, int *outfd, int *errfd, BASIC_FUNC Func, void *Data, const char *Config)
{
    pid_t pid;
		//default these to stdin, stdout and stderr and then override those later
		int c1=0, c2=1, c3=2;
    int channel1[2], channel2[2], channel3[2], DevNull=-1;
    int Flags;

		
		Flags=SpawnParseConfig(Config);
    if (infd) 
		{
			pipe(channel1);
			//this is a read channel, so pipe[0]
			c1=channel1[0];
		}

    if (outfd) 
		{
			pipe(channel2);
			//this is a write channel, so pipe[1]
			c2=channel2[1];
		}

    if (errfd) 
		{
			pipe(channel3);
			//this is a write channel, so pipe[1]
			c3=channel3[1];
		}
		else if (Flags & SPAWN_COMBINE_STDERR) c3=c2;


    pid=xforkio(c1, c2, c3);
    if (pid==0)
    {
        /* we are the child */
        if (infd) close(channel1[1]);
        else if (DevNull==-1) DevNull=open("/dev/null",O_RDWR);

        if (outfd) close(channel2[0]);
        else if (DevNull==-1) DevNull=open("/dev/null",O_RDWR);

        if (errfd) close(channel3[0]);
        else if (DevNull==-1) DevNull=open("/dev/null",O_RDWR);

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
    pid_t pid=-1, ConfigFlags;
    int tty, pty, i;

    if (PseudoTTYGrab(&pty, &tty, Flags))
    {
        pid=xforkio(tty, tty, tty);
        if (pid==0)
        {
            close(pty);

						//doesn't seem to exist under macosx!
						#ifdef TIOCSCTTY
            ioctl(tty,TIOCSCTTY,0);
						#endif
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
