#include "SpawnPrograms.h"
#include "libsettings.h"
#include "pty.h"
#include "file.h"
#include "string.h"
#include <sys/ioctl.h>

char *MakeShellSafeString(char *RetStr, const char *String, int SafeLevel)
{
char *Tempstr=NULL;
char *BadChars=";|$`";

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


pid_t ForkWithContext()
{
char *ptr;
pid_t pid;

LogFileFlushAll(TRUE);
pid=fork();
if (pid==0)
{
	ptr=LibUsefulGetValue("FORK:Dir");
	if (StrLen(ptr)) chdir(ptr);
	ptr=LibUsefulGetValue("FORK:User");
	if (StrLen(ptr)) SwitchUser(ptr);
	ptr=LibUsefulGetValue("FORK:Group");
	if (StrLen(ptr)) SwitchGroup(ptr);
}
return(pid);
}


/* This function turns our process into a demon */
int demonize()
{
int result, i=0;

LogFileFlushAll(TRUE);
//Don't fork with context here, as a demonize involves two forks, so
//it's wasted work here.
result=fork();
if (result != 0) exit(0);

/*we can only get to here if result= 0 i.e. we are the child process*/
setsid();

result=ForkWithContext();
if (result !=0) exit(0);
umask(0);

/* close stdin, stdout and std error, but only if they are a tty. In some  */
/* situations (like working out of cron) we may not have been given in/out/err */
/* and thus the first files we open will be 0,1,2. If we close them, we will have */
/* closed files that we need! Alternatively, the user may have used shell redirection */
/* to send output for a file, and I'm sure they don't want us to close that file */

//for (i=0; i < 3; i++)
{
	if (isatty(i)) 
	{
		close(i);
		/* reopen to /dev/null so that any output gets thrown away */
		/* but the program still has somewhere to write to         */
		open("/dev/null",O_RDWR);
	}
}


return(1);
}



int ForkWithIO(int StdIn, int StdOut, int StdErr)
{
pid_t pid;
int fd;

pid=ForkWithContext();
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
else
{
	fd=open("/dev/null",O_WRONLY);
	dup(fd);
	close(fd);
}

return(pid);
}



pid_t SpawnWithIO(char *CommandLine, int StdIn, int StdOut, int StdErr)
{
pid_t pid;
int fd, i;

pid=ForkWithIO(StdIn,StdOut,StdErr);
if (pid==0)
{
SwitchProgram(CommandLine);
_exit(pid);
}

return(pid);
}


int Spawn(char *ProgName)
{
return(SpawnWithIO(ProgName, 0,1,2));
}


/* This creates a child process that we can talk to using a couple of pipes*/
pid_t PipeSpawnFunction(int *infd,int  *outfd,int  *errfd, BASIC_FUNC Func, void *Data )
{
pid_t pid;
int channel1[2], channel2[2], channel3[2], DevNull=-1;
int count;

if (infd) pipe(channel1);
if (outfd) pipe(channel2);
if (errfd) pipe(channel3);

pid=ForkWithContext();
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
	if (errfd==COMMS_COMBINE_STDERR) dup(channel2[1]);
	else dup(channel3[1]);
}
else dup(DevNull);

Func(Data);
exit(0);
}
else 
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
	*errfd=channel3[0];
}

}

return(pid);
}


int BASIC_FUNC_EXEC_COMMAND(void *Data)
{
return(execl("/bin/sh","/bin/sh","-c",(char *) Data,NULL));
}


pid_t PipeSpawn(int *infd,int  *outfd,int  *errfd, const char *Command)
{
return(PipeSpawnFunction(infd,outfd,errfd, BASIC_FUNC_EXEC_COMMAND, (void *) Command));
}




pid_t PseudoTTYSpawnFunction(int *ret_pty, BASIC_FUNC Func, void *Data, int TTYFlags)
{
pid_t pid;
int tty, pty, i;
STREAM *S;
char *Tempstr=NULL;

if (GrabPseudoTTY(&pty, &tty, TTYFlags))
{
pid=ForkWithContext();
if (pid==0)
{
for (i=0; i < 4; i++) close(i);
close(pty);

setsid();
dup(tty);
dup(tty);
dup(tty);
ioctl(tty,TIOCSCTTY,0);
Func((char *) Data);
_exit(0);
}

close(tty);
}

*ret_pty=pty;
return(pid);
}


pid_t PseudoTTYSpawn(int *pty, const char *Command, int TTYFlags)
{
return(PseudoTTYSpawnFunction(pty, BASIC_FUNC_EXEC_COMMAND, (void *) Command,TTYFlags));
}


STREAM *STREAMSpawnCommand(const char *Command, int Flags)
{
int to_fd, from_fd;
pid_t pid;
STREAM *S=NULL;
char *Tempstr=NULL;


if (Flags & SPAWN_TRUST_COMMAND) Tempstr=CopyStr(Tempstr,Command);
else Tempstr=MakeShellSafeString(Tempstr, Command, 0);

if (Flags & COMMS_BY_PTY)
{
	pid=PseudoTTYSpawn(&to_fd,Tempstr,Flags);
	if (pid > 0) S=STREAMFromFD(to_fd);
}
else 
{
	if (Flags & COMMS_COMBINE_STDERR)
	{
		pid=PipeSpawn(&to_fd, &from_fd, COMMS_COMBINE_STDERR, Tempstr);
	}
	else pid=PipeSpawn(&to_fd, &from_fd, NULL, Tempstr);
	if (pid > 0) S=STREAMFromDualFD(from_fd, to_fd);
}

if (S)
{
	STREAMSetFlushType(S,FLUSH_LINE,0);
	Tempstr=FormatStr(Tempstr,"%d",pid);
	STREAMSetValue(S,"PeerPID",Tempstr);
}

DestroyString(Tempstr);
return(S);
}
