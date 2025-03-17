#include "Process.h"
#include "Container.h"

#include "errno.h"
#include "includes.h"
#include <pwd.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <syslog.h>
#include <glob.h>
#include "Log.h"
#include "Time.h"
#include "FileSystem.h"
#include "UnitsOfMeasure.h"
#include "Users.h"
#include "Seccomp.h"

//needed for 'flock' used by CreatePidFile and CreateLockFile
#include <sys/file.h>

#ifdef HAVE_PRCTL
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>
#endif



/*This is code to change the command-line of a program as visible in ps */
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char **environ;
#endif

static char *TitleBuffer=NULL;
static int TitleLen=0;




#ifdef USE_CAPABILITIES
#ifdef HAVE_LIBCAP
#include <sys/capability.h>

int CapabilitySet(cap_t caps, cap_value_t Cap)
{

    cap_set_flag(caps, CAP_EFFECTIVE, 1, &Cap, CAP_SET);
    cap_set_flag(caps, CAP_PERMITTED, 1, &Cap, CAP_SET);
    cap_set_flag(caps, CAP_INHERITABLE, 1, &Cap, CAP_SET);

    return(TRUE);
}
#endif
#endif

int ProcessSetCapabilities(const char *CapNames)
{
#ifdef USE_CAPABILITIES
#ifdef HAVE_LIBCAP

    char *Token=NULL;
    const char *ptr;
    cap_t caps;
    uid_t uid;

    caps=cap_get_proc();
    cap_clear(caps);
    ptr=GetToken(CapNames, ",",&Token,0);
    while (ptr)
    {
        if (strcasecmp(Token, "net-bind")==0) CapabilitySet(caps,  CAP_NET_BIND_SERVICE);
        else if (strcasecmp(Token, "net-admin")==0) CapabilitySet(caps,  CAP_NET_ADMIN);
        else if (strcasecmp(Token, "net-raw")==0) CapabilitySet(caps,  CAP_NET_RAW);
        else if (strcasecmp(Token, "net-bcast")==0) CapabilitySet(caps,  CAP_NET_BROADCAST);
        else if (strcasecmp(Token, "setuid")==0) CapabilitySet(caps,  CAP_SETUID);
        else if (strcasecmp(Token, "setgid")==0) CapabilitySet(caps,  CAP_SETGID);
        else if (strcasecmp(Token, "fsetid")==0) CapabilitySet(caps,  CAP_FSETID);
        else if (strcasecmp(Token, "chown")==0) CapabilitySet(caps,  CAP_CHOWN);
        else if (strcasecmp(Token, "mknod")==0) CapabilitySet(caps,  CAP_MKNOD);
        else if (strcasecmp(Token, "kill")==0) CapabilitySet(caps,  CAP_KILL);
        else if (strcasecmp(Token, "chroot")==0) CapabilitySet(caps,  CAP_SYS_CHROOT);
        else if (strcasecmp(Token, "reboot")==0) CapabilitySet(caps,  CAP_SYS_BOOT);
        else if (strcasecmp(Token, "nice")==0) CapabilitySet(caps,  CAP_SYS_NICE);
        else if (strcasecmp(Token, "time")==0) CapabilitySet(caps,  CAP_SYS_TIME);
        else if (strcasecmp(Token, "sys-admin")==0) CapabilitySet(caps,  CAP_SYS_ADMIN);
        else if (strcasecmp(Token, "rawio")==0) CapabilitySet(caps,  CAP_SYS_RAWIO);
        else if (strcasecmp(Token, "file-access")==0) CapabilitySet(caps,  CAP_DAC_OVERRIDE);
        else if (strcasecmp(Token, "audit")==0)
        {
            CapabilitySet(caps,  CAP_AUDIT_CONTROL);
            CapabilitySet(caps,  CAP_AUDIT_READ);
            CapabilitySet(caps,  CAP_AUDIT_WRITE);
        }
        else if (strcasecmp(Token, "audit")==0)
        {
            CapabilitySet(caps,  CAP_AUDIT_CONTROL);
            CapabilitySet(caps,  CAP_AUDIT_READ);
            CapabilitySet(caps,  CAP_AUDIT_WRITE);
        }



        ptr=GetToken(ptr, ",",&Token,0);
    }

    cap_set_proc(caps);

    uid=getuid();
#ifdef HAVE_SETRESUID
    setresuid( uid, uid, uid);
#else
    setreuid( uid, uid);
#endif

    // once our capabilites are set, we can't be adding any
    // further privilidges
    ProcessNoNewPrivs();

    Destroy(Token);

#endif
#endif

    return(1);
}


//Set controlling tty to be fd.
//This means that CTRL-C, SIGWINCH etc is handled for the selected fd
//and not any other
void ProcessSetControlTTY(int fd)
{
// TIOCSCTTY doesn't seem to exist under macosx!
#ifdef TIOCSCTTY
    ioctl(fd,TIOCSCTTY,1);
#endif
}


//The command-line args that we've been passed (argv) will occupy a block of contiguous memory that
//contains these args and the environment strings. In order to change the command-line args we isolate
//this block of memory by iterating through all the strings in it, and making copies of them. The
//pointers in 'argv' and 'environ' are then redirected to these copies. Now we can overwrite the whole
//block of memory with our new command-line arguments.
void ProcessTitleCaptureBuffer(char **argv)
{
    char *end=NULL;
    int i;

    TitleBuffer=*argv;
    end=*argv;
    for (i=0; argv[i] !=NULL; i++)
    {
//if memory is contiguous, then 'end' should always wind up
//pointing to the next argv
        if (end==argv[i])
        {
            while (*end != '\0') end++;
            end++;
        }
    }

//we used up all argv, environ should follow it
    if (argv[i] ==NULL)
    {
        for (i=0; environ[i] !=NULL; i++)
            if (end==environ[i])
            {
                while (*end != '\0') end++;
                end++;
            }
    }

//now we replace argv and environ with copies
    for (i=0; argv[i] != NULL; i++) argv[i]=strdup(argv[i]);
    for (i=0; environ[i] != NULL; i++) environ[i]=strdup(environ[i]);

//These might point to argv[0], so make copies of these too
#ifdef __GNU_LIBRARY__
    extern char *program_invocation_name;
    extern char *program_invocation_short_name;

    program_invocation_name=strdup(program_invocation_name);
    program_invocation_short_name=strdup(program_invocation_short_name);
#endif


    TitleLen=end-TitleBuffer;
}


void ProcessSetTitle(const char *FmtStr, ...)
{
    va_list args;

    if (! TitleBuffer) return;
    memset(TitleBuffer,0,TitleLen);

    va_start(args,FmtStr);
    vsnprintf(TitleBuffer,TitleLen,FmtStr,args);
    va_end(args);
}



int CreateLockFile(const char *FilePath, int Timeout)
{
    int fd, result;

    SetTimeout(Timeout, NULL);
    fd=open(FilePath, O_CREAT | O_RDWR, 0600);
    if (fd <0)
    {
        RaiseError(ERRFLAG_ERRNO, "lockfile","failed to open file %s", FilePath);
        return(-1);
    }

    result=flock(fd,LOCK_EX);
    alarm(0);

    if (result==-1)
    {
        RaiseError(ERRFLAG_ERRNO, "lockfile","failed to lock file %s", FilePath);
        close(fd);
        return(-1);
    }
    return(fd);
}

int WritePidFile(const char *ProgName)
{
    char *Tempstr=NULL;
    int fd, len;


    if (*ProgName=='/') Tempstr=CopyStr(Tempstr, ProgName);
    else Tempstr=FormatStr(Tempstr,"/var/run/%s.pid",ProgName);

    fd=open(Tempstr,O_CREAT | O_WRONLY,0600);
    if (fd > -1)
    {
        fchmod(fd,0644);
        if (flock(fd,LOCK_EX|LOCK_NB) ==0)
        {
            if (ftruncate(fd,0) !=0) RaiseError(ERRFLAG_ERRNO, "WritePidFile", "Failed to truncate pid file %s.",Tempstr);
            Tempstr=FormatStr(Tempstr,"%d\n",getpid());
            len=StrLen(Tempstr);
            if (write(fd,Tempstr,len) != len) RaiseError(ERRFLAG_ERRNO, "WritePidFile", "Failed to write to pidfile.");
        }
        else
        {
            RaiseError(ERRFLAG_ERRNO, "WritePidFile", "Failed to lock pid file %s. Program already running?",Tempstr);
            close(fd);
            fd=-1;
        }
    }
    else RaiseError(ERRFLAG_ERRNO, "WritePidFile", "Failed to open pid file %s",Tempstr);

//Don't close 'fd'!

    Destroy(Tempstr);

    return(fd);
}




void CloseOpenFiles()
{
    int i;

    for (i=3; i < 1024; i++) close(i);
}




void LU_DefaultSignalHandler(int sig)
{

}






void ProcessSetRLimit(int Type, const char *Value)
{
    struct rlimit limit;
    rlim_t val;

    val=(rlim_t) FromMetric(Value, 0);
    limit.rlim_cur=val;
    limit.rlim_max=val;
    setrlimit(Type, &limit);

}


int ProcessResistPtrace()
{

#ifdef HAVE_PRCTL
//Turn OFF Dumpable flag. This prevents producing coredumps, but has the side-effect of preventing ptrace attach.
//We normally control coredumps via resources (RLIMIT_CORE) rather than this
#ifdef PR_SET_DUMPABLE
#include <sys/prctl.h>

//set, then check we have the set. This covers situations where the sat failed, but we've already
//set the value previously somehow
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    if (prctl(PR_GET_DUMPABLE, 0, 0, 0, 0) == 0) return(TRUE);

    RaiseError(ERRFLAG_ERRNO, "ProcessResistPtrace", "Failed to setup ptrace resistance");
#else
    RaiseError(0, "ProcessResistPtrace", "This platform doesn't seem to support the 'resist ptrace' (PR_SET_DUMPABLE) option");
#endif
    RaiseError(0, "ProcessResistPtrace", "This platform doesn't seem to support the 'resist ptrace' (PR_SET_DUMPABLE) option (no prctl)");
#endif

    return(FALSE);
}


int ProcessNoNewPrivs()
{
#ifdef HAVE_PRCTL
#ifdef PR_SET_NO_NEW_PRIVS
#include <sys/prctl.h>

//set, then check that the set worked. This correctly handles situations where we ask to set more than once
//as the second attempt may 'fail', but we already have the desired result
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    if (prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 1) return(TRUE);

    RaiseError(ERRFLAG_ERRNO, "ProcessNoNewPrivs", "Failed to set 'no new privs'");
#else
    RaiseError(0, "ProcessNoNewPrivs", "This platform doesn't seem to support the 'no new privs' option");
#endif
#else
    RaiseError(0, "ProcessNoNewPrivs", "This platform doesn't seem to support the 'no new privs' option (no prctl)");
#endif

    return(FALSE);
}



static int ProcessMemLockAdd()
{
    int result=FALSE;
    LibUsefulFlags |= LU_MLOCKALL;
#ifdef HAVE_MLOCKALL
    if (mlockall(MCL_CURRENT | MCL_FUTURE)) result=TRUE;
    else RaiseError(ERRFLAG_ERRNO, "ProcessMemLockAdd", "Failed to set 'mlockall'");
#else
    RaiseError(0, "ProcessMemLockAdd", "This platform doesn't seem to support 'mlockall'");
#endif
    LibUsefulSetupAtExit();

    return(result);
}


static int ProcessParseSecurity(const char *Config, char **SeccompSetup)
{
    char *Token=NULL;
    const char *ptr;
    int Flags=0, val;
    const char *Levels[]= {"minimal", "basic", "user", "guest", "client", "untrusted", "local", "constrained", "high", NULL};
    typedef enum {LU_SEC_MINIMAL, LU_SEC_BASIC, LU_SEC_USER, LU_SEC_GUEST, LU_SEC_CLIENT, LU_SEC_UNTRUSTED, LU_SEC_LOCAL, LU_SEC_CONSTRAINED, LU_SEC_HIGH} TSecLevel;

    ptr=GetToken(Config, " ", &Token, 0);
    while (ptr)
    {
        val=MatchTokenFromList(Token, Levels, 0);

        switch (val)
        {
        case LU_SEC_HIGH:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_kill=group:net ");
        //break; //fall through to LU_SEC_CONSTRAINED

        case LU_SEC_CONSTRAINED:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_allow=ioctl(termget);ioctl(termset) syscall_deny=group:net syscall_kill=group:exec;mprotect(exec);mmap(exec);ioctl;group:ptrace ");
        //break; //fall through to LU_SEC_LOCAL

        case LU_SEC_LOCAL:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_allow=socket(unix) syscall_deny=socket ");
        //break; //fall through to LU_SEC_UNTRUSTED

        case LU_SEC_UNTRUSTED:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_kill=group:chroot;group:keyring;group:ns;setsid;acct ");
        //break; //fall through to LU_SEC_CLENT

        case LU_SEC_CLIENT:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_deny=listen;accept ");
        //break; //fall through to LU_SEC_GUEST

        case LU_SEC_GUEST:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_deny=group:keyring ");
        //break; //fall through to LU_SEC_USER

        case LU_SEC_USER:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_deny=chown;chmod(exec) syscall_kill=group:sysadmin;bpf ");
        //break; //fall through to LU_SEC_BASIC

        case LU_SEC_BASIC:
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_deny=acct ");
        //break; //fall through to LU_SEC_MINIMAL

        case LU_SEC_MINIMAL:
            //sadly, things like wine use ptrace, so we'd rather deny it than kill them.
            *SeccompSetup=CatStr(*SeccompSetup, "syscall_deny=ptrace syscall_kill=group:kexec;uselib;userfaultfd;personality;perf_event_open;group:kern_mods;kexec_load;get_kernel_syms;lookup_dcookie;vm86;vm86old;mbind;move_pages ");

            Flags |= PROC_NO_NEW_PRIVS;
            break;

        default:
            if (strncmp(Token, "syscall_allow=", 14)==0) *SeccompSetup=MCatStr(*SeccompSetup, Token, " ", NULL);
            else if (strncmp(Token, "syscall_kill=", 13)==0) *SeccompSetup=MCatStr(*SeccompSetup, Token, " ", NULL);
            else if (strncmp(Token, "syscall_deny=", 13)==0) *SeccompSetup=MCatStr(*SeccompSetup, Token, " ", NULL);
            Flags |= PROC_NO_NEW_PRIVS;
            break;
        }
        ptr=GetToken(ptr, " ", &Token, 0);
    }

    Destroy(Token);

    return(Flags);
}




//do all things that we can do 'early' (i.e. before chroot and demonize)
static int ProcessApplyEarlyConfig(const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    int Flags=0;

    ptr=Config;
    while (isspace(*ptr)) ptr++;
    ptr=GetNameValuePair(ptr,"\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name,"nice")==0) setpriority(PRIO_PROCESS, 0, atoi(Value));
        else if (strcasecmp(Name,"prio")==0) setpriority(PRIO_PROCESS, 0, atoi(Value));
        else if (strcasecmp(Name,"priority")==0) setpriority(PRIO_PROCESS, 0, atoi(Value));
        else if (strcasecmp(Name,"openlog")==0) openlog(Value, LOG_PID, LOG_USER);
        else if (strcasecmp(Name,"sigdef")==0) Flags |= PROC_SIGDEF;
        else if (strcasecmp(Name,"sigdefault")==0) Flags |= PROC_SIGDEF;
        else if (strcasecmp(Name,"setsid")==0) Flags |= PROC_SETSID;
        else if (strcasecmp(Name,"newpgroup")==0) Flags |= PROC_NEWPGROUP;
        else if (strcasecmp(Name,"daemon")==0) Flags |= PROC_DAEMON;
        else if (strcasecmp(Name,"demon")==0) Flags |= PROC_DAEMON;
        else if (strcasecmp(Name,"ctrltty")==0) Flags |= PROC_CTRL_TTY;
        else if (strcasecmp(Name,"ctrl_tty")==0) Flags |= PROC_CTRL_TTY;
        else if (strcasecmp(Name,"strict")==0) Flags |= PROC_SETUP_STRICT;
        else if (strcasecmp(Name,"innull")==0)  fd_remap_path(0, "/dev/null", O_WRONLY);
        else if (strcasecmp(Name,"errnull")==0) fd_remap_path(2, "/dev/null", O_WRONLY);
        else if (strcasecmp(Name,"outnull")==0)
        {
            fd_remap_path(1, "/dev/null", O_WRONLY);
            fd_remap_path(2, "/dev/null", O_WRONLY);
        }
        else if (strcasecmp(Name,"stdin")==0)  fd_remap(0, atoi(Value));
        else if (strcasecmp(Name,"stdout")==0)  fd_remap(1, atoi(Value));
        else if (strcasecmp(Name,"stderr")==0)  fd_remap(2, atoi(Value));
        else if (strcasecmp(Name,"jail")==0) Flags |= PROC_JAIL;
        else if (strcasecmp(Name,"trust")==0) Flags |= SPAWN_TRUST_COMMAND;
        else if (strcasecmp(Name,"noshell")==0) Flags |= SPAWN_NOSHELL;
        else if (strcasecmp(Name,"arg0")==0) Flags |= SPAWN_ARG0;
        else if (strcasecmp(Name,"container")==0) Flags |= PROC_CONTAINER_FS;
        else if (strcasecmp(Name,"container+net")==0) Flags |= PROC_CONTAINER_FS | PROC_CONTAINER_NET;
        else if (strcasecmp(Name,"isocube")==0) Flags |= PROC_CONTAINER_FS;
        else if (strcasecmp(Name,"-net")==0) Flags |= PROC_CONTAINER_NET;
        else if (strcasecmp(Name,"nonet")==0) Flags |= PROC_CONTAINER_NET;
        else if (strcasecmp(Name,"nopid")==0) Flags |= PROC_CONTAINER_PID;
        else if (strcasecmp(Name,"-pid")==0) Flags |= PROC_CONTAINER_PID;
        else if (strcasecmp(Name,"ns")==0) Flags |= PROC_CONTAINER_FS;
        else if (strcasecmp(Name,"namespace")==0) Flags |= PROC_CONTAINER_FS;
        else if (strcasecmp(Name,"mlock")==0) ProcessMemLockAdd();
        else if (strcasecmp(Name,"memlock")==0) ProcessMemLockAdd();
        else if (strcasecmp(Name,"mem")==0) ProcessSetRLimit(RLIMIT_DATA, Value);
        else if (strcasecmp(Name,"mlockmax")==0) ProcessSetRLimit(RLIMIT_MEMLOCK, Value);
        else if (strcasecmp(Name,"fsize")==0) ProcessSetRLimit(RLIMIT_FSIZE, Value);
        else if (strcasecmp(Name,"files")==0) ProcessSetRLimit(RLIMIT_NOFILE, Value);
        else if (strcasecmp(Name,"coredumps")==0) ProcessSetRLimit(RLIMIT_CORE, Value);
        else if ( (strcasecmp(Name,"procs")==0) || (strcasecmp(Name,"nproc")==0) ) ProcessSetRLimit(RLIMIT_NPROC, Value);
        else if (strcasecmp(Name, "resist_ptrace")==0) LibUsefulFlags |= LU_RESIST_PTRACE;
        else if (strcasecmp(Name,"chroot")==0)
        {
            if ( StrValid(Value) && (chdir(Value) !=0 ) )
            {
                RaiseError(ERRFLAG_ERRNO, "ProcessApplyEarlyConfig", "failed to chdir to directory %s for chrooting", Value);
                Flags |= PROC_SETUP_FAIL;
            }
            else Flags |= PROC_CHROOT;
        }

        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }

    if (LibUsefulFlags & LU_RESIST_PTRACE)
    {
        if (! ProcessResistPtrace()) Flags |= PROC_SETUP_FAIL;
    }

    Destroy(Name);
    Destroy(Value);

    return(Flags);
}


//Apply config changes that are relevant AFTER chroot/daemonize
static int ProcessApplyLateConfig(int Flags, const char *Config)
{
    char *Name=NULL, *Value=NULL, *Capabilities=NULL, *SeccompDeny=NULL;
    const char *ptr;
    long uid=0, gid=0;
    int lockfd, ctty_fd=0;

//these are things that, if we've Chroot-ed, happen *within* the Chroot. But not within a Jail.
    ptr=GetNameValuePair(Config,"\\S","=",&Name,&Value);
    while (ptr)
    {
        if (strcasecmp(Name,"User")==0) uid=LookupUID(Value);
        else if (strcasecmp(Name,"Group")==0) gid=LookupGID(Value);
        else if (strcasecmp(Name,"UID")==0) uid=atoi(Value);
        else if (strcasecmp(Name,"GID")==0) gid=atoi(Value);
        else if ( (strcasecmp(Name,"Dir")==0) || (strcasecmp(Name, "chdir")==0) )
        {
            if (chdir(Value) !=0)
            {
                RaiseError(ERRFLAG_ERRNO, "ProcessApplyConfig", "failed to chdir to %s", Value);
                RaiseError(ERRFLAG_ERRNO, "ProcessApplyConfig", "too dangerous to continue, (possibly in wrong directory) exiting...", Value);
                exit(1);

                Flags |= PROC_SETUP_FAIL;
            }
        }
        else if (strcasecmp(Name,"PidFile")==0) WritePidFile(Value);
        else if (strcasecmp(Name,"LockFile")==0)
        {
            lockfd=CreateLockFile(Value, 0);
            if (lockfd==-1) _exit(1);
        }
        else if (strcasecmp(Name,"LockStdin")==0)
        {
            close(0);
            lockfd=CreateLockFile(Value, 0);
            if (lockfd==-1) _exit(1);
        }
        else if (strcasecmp(Name,"nosu")==0) Flags |= PROC_NO_NEW_PRIVS;
        else if (strcasecmp(Name,"nopriv")==0) Flags |= PROC_NO_NEW_PRIVS;
        else if (strcasecmp(Name,"noprivs")==0) Flags |= PROC_NO_NEW_PRIVS;
        else if (strcasecmp(Name,"capabilities")==0) Capabilities=CopyStr(Capabilities, Value);
        else if (strcasecmp(Name,"security")==0) Flags |= ProcessParseSecurity(Value, &SeccompDeny);
        else if (strcasecmp(Name,"ctty")==0)
        {
            ctty_fd=atoi(Value);
            Flags |= PROC_CTRL_TTY;
        }

        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }

    if (Flags & PROC_CONTAINER)
    {
        if (! ContainerApplyConfig(Config)) Flags |= PROC_SETUP_FAIL;
    }

    if (Flags & PROC_CTRL_TTY) ProcessSetControlTTY(ctty_fd);


//Always do group first, otherwise we'll lose ability to switch user/group
    if (gid > 0) SwitchGID(gid);
    if (uid > 0) SwitchUID(uid);

    if (LibUsefulFlags & LU_RESIST_PTRACE)
    {
        // do this again, as switching uid or gid can reset this
        if (! ProcessResistPtrace()) Flags |= PROC_SETUP_FAIL;
    }

//Must do this last! After parsing Config, and also after functions like
//SwitchUser that will need access to /etc/passwd
    if (Flags & PROC_JAIL)
    {
        if (chroot(".") == -1)
        {
            RaiseError(ERRFLAG_ERRNO, "ProcessApplyConfig", "failed to chroot to curr directory");
            Flags |= PROC_SETUP_FAIL;
        }
    }


    if (StrValid(Capabilities)) ProcessSetCapabilities(Capabilities);
    //if we set any capabilites, we will already have set 'NO_NEW_PRIVS'
    //so only consider the PROC_NO_NEW_PRIVS flag if we didn't use
    //capabilities
    else if (Flags & PROC_NO_NEW_PRIVS)
    {
        if (! ProcessNoNewPrivs()) Flags |= PROC_SETUP_FAIL;
        else if (LibUsefulDebugActive()) fprintf(stderr, "DEBUG: set 'PROC_NO_NEW_PRIVS', su/suid should not be possible now\n");


        //seccomp must come after PROC_NO_NEW_PRIVS
#ifdef USE_SECCOMP
        if (StrValid(SeccompDeny)) SeccompAddRules(SeccompDeny);
#endif
    }

    Destroy(Name);
    Destroy(Value);
    Destroy(Capabilities);
    Destroy(SeccompDeny);

    return(Flags);
}




int ProcessApplyConfig(const char *Config)
{
    int Flags=0, i;

//do all things that we can do 'early' (i.e. before chroot and demonize)
    Flags=ProcessApplyEarlyConfig(Config);

    if (Flags & PROC_SETUP_FAIL)
    {
        if (Flags & PROC_SETUP_STRICT) RaiseError(ERRFLAG_ABORT, "ProcessApplyConfig", "Early setup failed. Strict mode requested. Aborting.");
        return(Flags);
    }


//set all signal handlers to default
    if (Flags & PROC_SIGDEF)
    {
        for (i =0; i < NSIG; i++) signal(i,SIG_DFL);
    }


//if we're to run as a daemon service, then do so
//which will mean a new group, new sid and closing our tty.
//Otherwise setup these things for a process with a ttty
    if (Flags & PROC_DAEMON) demonize();
    else
    {
        if (Flags & PROC_SETSID) setsid();
        if (Flags & PROC_NEWPGROUP) setpgid(0, 0);
    }


// This allows us to chroot into a whole different unix directory tree, with its own
// password file etc
    if (Flags & PROC_CHROOT)
    {
        if (chroot(".") == -1)
        {
            RaiseError(ERRFLAG_ERRNO, "ProcessApplyConfig", "failed to chroot");
            Flags |= PROC_SETUP_FAIL;
            if (Flags & PROC_SETUP_STRICT) RaiseError(ERRFLAG_ABORT, "ProcessApplyConfig", "chroot failed. Strict mode requested. Aborting.");
        }
    }


    //Apply config changes that are relevant AFTER chroot/daemonize
    if (! (Flags & PROC_SETUP_FAIL)) Flags=ProcessApplyLateConfig(Flags, Config);
    if ( (Flags & PROC_SETUP_FAIL) && (Flags & PROC_SETUP_STRICT) ) RaiseError(ERRFLAG_ABORT, "ProcessApplyConfig", "Late setup failed. Strict mode requested. Aborting.");


    return(Flags);
}


// This function turns our process into a demon
// though this requires forks, we do not call CredsStoreOnFork as we want to take the Credentials Store with us.
pid_t demonize()
{
    int result, i=0;

    LogFileFlushAll(TRUE);

//Don't fork with context here, as a demonize involves two forks, so
//it's wasted work here.
    result=fork();
    if (result != 0) exit(0);

    /*we can only get to here if result= 0 i.e. we are the child process*/
    setsid();

    result=fork();
    if (result !=0) exit(0);
    umask(0);

    /* close stdin, stdout and std error, but only if they are a tty. In some  */
    /* situations (like working out of cron) we may not have been given in/out/err */
    /* and thus the first files we open will be 0,1,2. If we close them, we will have */
    /* closed files that we need! Alternatively, the user may have used shell redirection */
    /* to send output for a file, and I'm sure they don't want us to close that file */

    for (i=0; i < 3; i++)
    {
        if (isatty(i))
        {
            /* reopen to /dev/null so that any output gets thrown away */
            /* but the program still has somewhere to write to         */

            fd_remap_path(i, "/dev/null", O_RDWR);
        }
    }


    return(getpid());
}
