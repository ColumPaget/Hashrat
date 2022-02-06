/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_PROCESS_H
#define LIBUSEFUL_PROCESS_H

#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>

//Various functions related to a process

#define PROC_DAEMON 8       //make process a daemon
#define PROC_SETSID 16      //create a new session for this process
#define PROC_CTRL_TTY 32    
#define PROC_CHROOT 64      //chroot this process
#define PROC_JAIL 128
#define PROC_SIGDEF 256     //set default signal mask for this process
#define PROC_CONTAINER 512  
#define PROC_CONTAINER_NET  1024
#define PROC_ISOCUBE  2048
#define PROC_NEWPGROUP  4096  //create new process group for this process

//these must be compatible with PROC_ defines
#define SPAWN_NOSHELL 8192
#define SPAWN_TRUST_COMMAND 16384
#define SPAWN_ARG0 32768

#define PROC_SETUP_FAIL 65536


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SIGNAL_HANDLER_FUNC)(int sig);
void LU_DefaultSignalHandler(int sig);

// make the current process a background process/service, with no connection to a terminal.
// Beware, this will change its pid. Returns the new pid or '0' on failure
pid_t demonize();

//singply try to close all file descriptors from 3 to 1024, leaving stdin, stdout and stderr alone
void CloseOpenFiles();

//switch user or group by id (call SwitchGID first, or you may not have permissions to switch user)
int SwitchUID(int uid);
int SwitchGID(int gid);

//switch user or group by name (call SwitchGgroup first, or you may not have permissions to switch user)
int SwitchUser(const char *User);
int SwitchGroup(const char *Group);

//returns a string pointing to a users home directory. DO NOT FREE THIS STRING. Take a copy of it, as it's
//an internal buffer and will change on the next call to this function
char *GetCurrUserHomeDir();

//write a file containing the current pid. The file can either be an absolute path to a file anywhere
//or a relative path that creates a file in /var/run. The file is locked with flock
//returns a file descriptor to the current file
int WritePidFile(const char *ProgName);

int CreateLockFile(const char *FilePath, int Timeout);

//pass argv from main to this function if you want to rewrite the process name that you see in 'ps'
void ProcessTitleCaptureBuffer(char **argv);

//set the process name that you see in 'ps'. Works like sprintf, with a format string and a variable number of
//arguments. ProcessTitleCaptureBuffer must be called first
void ProcessSetTitle(const char *FmtStr, ...);

// set 'fd' to be a processes controling tty
void ProcessSetControlTTY(int fd);

/*
ProcessApplyConfig()  changes aspects of a running process. This function is not normally used in C programming, and is instead either called from the Spawn or fork functions in SpawnCommands.c or is used when binding libUseful functionality to scripting languages that have limited types, and where structures cannot easily be used to pass data.

Values that can be passed in the 'Config' string of this function are:

user=<name>     run process as user 'name'
group=<name>    run process as group 'name'
dir=<path>      run process in directory 'path'

setsid          start a new session for the process
newpgroup       start a new process group for the process (pgid will be the same as the processes pid)

ctrl_tty        set controlling tty of process to be it's standard-in. Signals and tty output will then happen on that channel, rather than
                via the console the program is running on. So if your overall program is running on /dev/tty1, switch it to believe it's
                tty is really whatever is on stdin

ctty=<fd>       set controlling tty of process to be 'fd' (where fd is a file descriptor number). Same as ctrl_tty above, but using an
                arbitary file descriptor rather than stdin.

innull          redirect process stdin to /dev/null
outnull         redirect process stdout and stderr to /dev/null
errnull         redirect process stderr to /dev/null

sigdef          set all signal handlers to the default values (throw away any sighandlers set by parent process)
sigdefault      set all signal handlers to the default values (throw away any sighandlers set by parent process)

chroot=<path>   chroot process into <path>. This option happens before switching users, so that user info lookup, lockfile, pidfile,
                and chdir to directory specified with 'dir=<path>'  all happen within the chroot. Thus this is used when chrooting
                into a full unix filesystem

jail            jail the process. This does a chroot AFTER looking up the user id, creating lockfile, etc ,etc. This can be used to
                jail a process into an empty or private directory

ns=<path>       linux namespace to join. <path> is either a path to a namespace file, or a path to a directory (e.g. /proc/<pid>/ns )
                that contains namespace descriptor files

nice=value      'nice' value of new process
prio=value       scheduling priority of new process (equivalent to 0 - nice value)
priority=value   scheduling priority of new process (equivalent to 0 - nice value)
mem=value        resource limit for memory (data segment)
fsize=value      resource limit for filesize
files=value      resource limit for open files
coredumps=value  resource limit for max size of coredump files
procs=value      resource limit for max number of processes ON A PER USER BASIS.
*/

int ProcessApplyConfig(const char *Config);

#ifdef __cplusplus
}
#endif

#endif
