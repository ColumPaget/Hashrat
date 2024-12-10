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

#define PROC_DAEMON            8     // make process a daemon
#define PROC_SETSID           16     // create a new session for this process
#define PROC_CTRL_TTY         32     // set stdin to be a controlling tty
#define PROC_CHROOT           64     // chroot this process before switching user etc. Used to chroot into a full unix system.
#define PROC_NEWPGROUP       128     // create new process group for this process
#define PROC_SIGDEF          256     // set default signal mask for this process
#define PROC_CONTAINER_FS    512  
#define PROC_CONTAINER_NET  1024     // unshare network namespace for this process
#define PROC_CONTAINER_PID  2048     // unshare pids namespace

//these must be compatible with PROC_ defines
#define SPAWN_NOSHELL        8192    // run the command directly using exec, not from a shell using system
#define SPAWN_TRUST_COMMAND 16384    // don't strip unsafe chars from command
#define SPAWN_ARG0          32768    // deduce arg[0] from the command name

#define PROC_SETUP_FAIL     65536    // internal flag if anyting goes wrong, can trigger PROC_SETUP_STRICT
#define PROC_SETUP_STRICT  131072    // if anything goes wrong in ProcessApplyConfig, then abort program
#define PROC_NO_NEW_PRIVS  262144    // do not allow privilege escalation via setuid or other such methods

#define PROC_JAIL          524288    // chroot after everything setup. This jails a process in a directory, not the same as PROC_CHROOT
#define PROC_ISOCUBE      1048576    // chroot into a tmpfs filesystem. Any files process writes will be lost when it exits


#define PROC_CONTAINER (PROC_CONTAINER_FS | PROC_CONTAINER_NET | PROC_CONTAINER_PID)

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SIGNAL_HANDLER_FUNC)(int sig);
void LU_DefaultSignalHandler(int sig);

// make the current process a background process/service, with no connection to a terminal.
// Beware, this will change its pid. Returns the new pid or '0' on failure
pid_t demonize();

//try to close all file descriptors from 3 to 1024, leaving stdin, stdout and stderr alone
void CloseOpenFiles();

//write a file containing the current pid. The file can either be an absolute path to a file anywhere
//or a relative path that creates a file in /var/run. The file is locked with flock
//returns a file descriptor to the current file
int WritePidFile(const char *ProgName);

//create/use a lockfile at 'FilePath'. If file is locked already then wait 'Timeout' for it to be free
int CreateLockFile(const char *FilePath, int Timeout);

//pass argv from main to this function if you want to rewrite the process name that you see in 'ps'
void ProcessTitleCaptureBuffer(char **argv);

//set the process name that you see in 'ps'. Works like sprintf, with a format string and a variable number of
//arguments. ProcessTitleCaptureBuffer must be called first
void ProcessSetTitle(const char *FmtStr, ...);

// set 'fd' to be a processes controling tty
void ProcessSetControlTTY(int fd);

//set process to not be traceable with ptrace/strace, and also unable to make coredumps
int ProcessResistPtrace();

//set 'no new privs' to process cannot switch user/priviledges by any means (no su, sudo or setuid)
int ProcessNoNewPrivs();


/*
ProcessApplyConfig()  changes aspects of a running process. This function is not normally used in C programming, and is instead either called from the Spawn or fork functions in SpawnCommands.c or is used when binding libUseful functionality to scripting languages that have limited types, and where structures cannot easily be used to pass data.

Values that can be passed in the 'Config' string of this function are:

user=<name>     run process as user 'name'
group=<name>    run process as group 'name'
uid=<uid>       run process as user number 'uid'
gid=<gid>       run process as group number 'gid'
dir=<path>      run process in directory 'path'

setsid          start a new session for the process
newpgroup       start a new process group for the process (pgid will be the same as the processes pid)

ctrltty
ctrl_tty        set controlling tty of process to be it's standard-in. Signals and tty output will then happen on that channel, rather than via the console the program is running on. So if your overall program is running on /dev/tty1, switch it to believe it's tty is really whatever is on stdin

ctty=<fd>       set controlling tty of process to be 'fd' (where fd is a file descriptor number). 
                Same as ctrl_tty above, but using an arbitary file descriptor rather than stdin.

innull          redirect process stdin to /dev/null
outnull         redirect process stdout and stderr to /dev/null
errnull         redirect process stderr to /dev/null

stdin=<fd>      redirect process stdin to file descriptor <fd>
stdout=<fd>     redirect process stdout to file descriptor <fd>
stderr=<fd>     redirect process stderr to file descriptor <fd>

sigdef          set all signal handlers to the default values (throw away any sighandlers set by parent process)
sigdefault      set all signal handlers to the default values (throw away any sighandlers set by parent process)

demon
daemon          'daemonize' the current process (fork it into the background, close stdin/stdout/stderr, etc)

chroot=<path>   chroot process into <path>. This option happens before switching users, so that user info lookup, lockfile, pidfile, and chdir to directory specified with 'dir=<path>'  all happen within the chroot. Thus this is used when chrooting into a full unix filesystem

jail            jail the process. This does a chroot AFTER looking up the user id, creating lockfile, etc ,etc. This can be used to jail a process into an empty or private directory

strict          abort process if chdir, chroot or jail fails

namespace=<path>
ns=<path>       linux namespace to join. <path> is either a path to a namespace file, or a path to a directory (e.g. /proc/<pid>/ns )
                that contains namespace descriptor files

nosu            set 'prctl(PR_NO_NEW_PRIVS)' to prevent privesc via su/sudo/setuid
nopriv          set 'prctl(PR_NO_NEW_PRIVS)' to prevent privesc via su/sudo/setuid
noprivs         set 'prctl(PR_NO_NEW_PRIVS)' to prevent privesc via su/sudo/setuid
nice=<value>      'nice' value of new process
prio=<value>       scheduling priority of new process (equivalent to 0 - nice value)
priority=<value>   scheduling priority of new process (equivalent to 0 - nice value)
mem=<value>        resource limit for memory (data segment)
mlockmax=<value>   resource limit for locked memory
fsize=<value>      resource limit for filesize
files=<value>      resource limit for open files
coredumps=<value>  resource limit for max size of coredump files
procs=<value>      resource limit for max number of processes ON A PER USER BASIS.
nproc=<value>      resource limit for max number of processes ON A PER USER BASIS.
resist_ptrace      set prctrl(PR_NONE_DUMPABLE) to prevent ptracing of the process. This also prevents coredumps totally.
openlog=<name>     set 'ident' of future syslog messages to 'name'. Also adds 'LOG_PID' and sets facility to 'LOG_USER' (see "man openlog" for more details).
mlock              lock all current and future pages in memory so they don't swap out
memlock            lock all current and future pages in memory so they don't swap out
pidfile=<path>     create pidfile for this process at 'path'
lockfile=<path>    create lockfile at 'path'

security=<level>   set security levels for seccomp. These are intended to mostly kill processess that are trying to use suspicious/dangerous or inappropriate syscalls.
									 Any level includes the 'nosu' setting as seccomp requires setting 'prctrl(PR_NO_NEW_PRIVS)
                   Levels are 'minimal', 'basic', 'user', 'untrusted', 'constrained' and 'high'. Each level includes the level below it, so 'untrsted' gives you everything in 'minimal', 'basic' and 'user'.

                   minimal: disable ptrace and kill apps that try to use: personality, uselib, userfaultfd, perf_event_open, kexec_load, get_kernel_syms, lookup_dcookie, vm86, vm86old, mbind, move_pages, nfsservctl, and anything involving kernel modules
									 basic: everything in 'minimal' but also disable the 'acct' syscall
									 user: everything in 'basic' but also kill processes that try to use bpf, or any 'sysadmin' calls: settimeofday, clocksettime, clockadjtime, quotactl, reboot, swapon, swapoff, mount, umount, umount2, mknod, quotactl
                   untrusted: everyting in 'user' but kill apps that try to use: chroot, access the keyring, unshare or change namespaces group:ns or all acct
                   constrained: everything in 'untrusted' but kill apps that try to use: socket/network syscalls, exec syscalls, mprotect, ioctl or ptrace
									 high: kill apps that try to use networking
*/


int ProcessApplyConfig(const char *Config);

#ifdef __cplusplus
}
#endif

#endif
