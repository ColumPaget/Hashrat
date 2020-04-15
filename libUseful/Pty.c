#define _XOPEN_SOURCE
#define _GNU_SOURCE

#include "includes.h"
#include "Stream.h"
#include "GeneralFunctions.h"
#include "String.h"
#include "Pty.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>


static ListNode *TTYAttribs=NULL;


int TTYHangUp(int tty)
{
    struct termios tty_data, oldtty_data;

    if (! isatty(tty)) return(FALSE);

    tcgetattr(tty,&oldtty_data);
    tcgetattr(tty,&tty_data);
    cfsetispeed(&tty_data,B0);
    cfsetospeed(&tty_data,B0);

    tcsetattr(tty,TCSANOW,&tty_data);
    sleep(2);
    tcsetattr(tty,TCSANOW,&oldtty_data);

    return(TRUE);
}

int TTYReset(int tty)
{
    struct termios *tty_data;
    char *Tempstr=NULL;
    ListNode *Curr;

    if (! isatty(tty)) return(FALSE);
    Tempstr=FormatStr(Tempstr,"%d",tty);
    Curr=ListFindNamedItem(TTYAttribs, Tempstr);
    if (Curr)
    {
        tty_data=(struct termios *) Curr->Item;
        tcsetattr(tty,TCSANOW,tty_data);
        ListDeleteNode(Curr);
        free(tty_data);
    }

    DestroyString(Tempstr);

    return(TRUE);
}


void PTYSetGeometry(int pty, int wid, int high)
{
    struct winsize w;

    w.ws_col=wid;
    w.ws_row=high;
    ioctl(pty, TIOCSWINSZ, &w);
}


void TTYConfig(int tty, int LineSpeed, int Flags)
{
    struct termios tty_data, *old_tty_data;
    int val;
    char *Tempstr=NULL;
    ListNode *Curr;

    Tempstr=FormatStr(Tempstr,"%d",tty);
    if (! TTYAttribs) TTYAttribs=ListCreate();
    Curr=ListFindNamedItem(TTYAttribs,Tempstr);

    if (! Curr) old_tty_data=(struct termios *) calloc(1,sizeof(struct termios));
    else old_tty_data=(struct termios *) Curr->Item;

    if (Flags & TTYFLAG_SAVE) ListAddNamedItem(TTYAttribs,Tempstr,old_tty_data);
    tcgetattr(tty,old_tty_data);

    memset(&tty_data,0,sizeof(struct termios));

//ignore break characters and parity errors
    tty_data.c_iflag=IGNBRK | IGNPAR;


    if (! (Flags & TTYFLAG_CRLF_KEEP))
    {
        //translate carriage-return to newline
        if (Flags & TTYFLAG_IN_CRLF) tty_data.c_iflag |= ICRNL;
        else tty_data.c_iflag &= ~ICRNL;

        //translate newline to carriage return
        if (Flags & TTYFLAG_IN_LFCR)
        {
            tty_data.c_iflag |= INLCR;
        }
        else tty_data.c_iflag &= ~INLCR;

        //postprocess and translate newline to cr-nl
        if (Flags & TTYFLAG_OUT_CRLF)
        {
            tty_data.c_oflag |= ONLCR | OPOST;
        }
    }

    tty_data.c_cflag=CREAD | CS8 | HUPCL | CLOCAL;


    tty_data.c_cc[VERASE]=old_tty_data->c_cc[VERASE];
    if (Flags & TTYFLAG_SOFTWARE_FLOW)
    {
        tty_data.c_iflag |= IXON | IXOFF;
        tty_data.c_cc[VSTART]=old_tty_data->c_cc[VSTART];
        tty_data.c_cc[VSTOP]=old_tty_data->c_cc[VSTOP];
    }

#ifdef CRTSCTS
    if (Flags & TTYFLAG_HARDWARE_FLOW) tty_data.c_cflag |=CRTSCTS;
#endif

// 'local' input flags
    tty_data.c_lflag=0;

    if (! (Flags & TTYFLAG_IGNSIG))
    {
        tty_data.c_lflag=ISIG;
        tty_data.c_cc[VQUIT]=old_tty_data->c_cc[VQUIT];
        tty_data.c_cc[VSUSP]=old_tty_data->c_cc[VSUSP];
        tty_data.c_cc[VINTR]=old_tty_data->c_cc[VINTR];
    }
    if (Flags & TTYFLAG_ECHO) tty_data.c_lflag |= ECHO;

    if (Flags & TTYFLAG_CANON)
    {
        tty_data.c_lflag|= ICANON;
        tty_data.c_cc[VEOF]=old_tty_data->c_cc[VEOF];
        tty_data.c_cc[VEOL]=old_tty_data->c_cc[VEOL];
        tty_data.c_cc[VKILL]=old_tty_data->c_cc[VKILL];
        tty_data.c_cc[VERASE]=old_tty_data->c_cc[VERASE];
    }
    else
    {
        tty_data.c_cc[VMIN]=1;
        tty_data.c_cc[VTIME]=0;
    }

//Higher line speeds protected with #ifdef because not all
//operating systems seem to have them
    if (LineSpeed > 0)
    {
        switch (LineSpeed)
        {
        case 2400:
            val=B2400;
            break;
        case 4800:
            val=B4800;
            break;
        case 9600:
            val=B9600;
            break;
        case 19200:
            val=B19200;
            break;
        case 38400:
            val=B38400;
            break;
#ifdef B57600
        case 57600:
            val=B57600;
            break;
#endif

#ifdef B115200
        case 115200:
            val=B115200;
            break;
#endif

#ifdef B230400
        case 230400:
            val=B230400;
            break;
#endif

#ifdef B460800
        case 460800:
            val=B460800;
            break;
#endif

#ifdef B500000
        case 500000:
            val=B500000;
            break;
#endif

#ifdef B1000000
        case 10000000:
            val=B1000000;
            break;
#endif

#ifdef B1152000
        case 1152000:
            val=B1152000;
            break;
#endif

#ifdef B2000000
        case 2000000:
            val=B2000000;
            break;
#endif

#ifdef B4000000
        case 4000000:
            val=B4000000;
            break;
#endif

        default:
            val=B38400;
            break;
        }
        cfsetispeed(&tty_data,val);
        cfsetospeed(&tty_data,val);
    }

    tcflush(tty,TCIFLUSH);
    tcsetattr(tty,TCSANOW,&tty_data);

    DestroyString(Tempstr);
}




int TTYOpen(const char *devname, int LineSpeed, int Flags)
{
    int tty, flags=O_RDWR | O_NOCTTY;


    if (Flags & TTYFLAG_NONBLOCK)
    {
        //O_NDELAY should be used only if nothing else available
        //as O_NONBLOCK is more 'posixy'
#ifdef O_NDELAY
        flags |= O_NDELAY;
#endif

#ifdef O_NONBLOCK
        flags |= O_NONBLOCK;
#endif
    }

    tty=open(devname, flags);

    if ( tty <0)
    {
        RaiseError(ERRFLAG_ERRNO, "tty", "failed to open %s", devname);
        return(-1);
    }

    TTYConfig(tty, LineSpeed, Flags);
    return(tty);
}




int TTYParseConfig(const char *Config, int *Speed)
{
    const char *ptr;
    char *Token=NULL;
    int Flags=0;

    ptr=GetToken(Config," |,",&Token,GETTOKEN_MULTI_SEP);
    while (ptr)
    {
        if (strcasecmp(Token,"pty")==0) Flags |= TTYFLAG_PTY;
        else if (strcasecmp(Token,"canon")==0) Flags |= TTYFLAG_CANON;
        else if (strcasecmp(Token,"echo")==0) Flags |= TTYFLAG_ECHO;
        else if (strcasecmp(Token,"xon")==0) Flags |= TTYFLAG_SOFTWARE_FLOW;
        else if (strcasecmp(Token,"sw")==0) Flags |= TTYFLAG_SOFTWARE_FLOW;
        else if (strcasecmp(Token,"hw")==0) Flags |= TTYFLAG_HARDWARE_FLOW;
        else if (strcasecmp(Token,"nb")==0) Flags |= TTYFLAG_NONBLOCK;
        else if (strcasecmp(Token,"nonblock")==0) Flags |= TTYFLAG_NONBLOCK;
        else if (strcasecmp(Token,"ilfcr")==0) Flags |= TTYFLAG_IN_LFCR;
        else if (strcasecmp(Token,"icrlf")==0) Flags |= TTYFLAG_IN_CRLF;
        else if (strcasecmp(Token,"ocrlf")==0) Flags |= TTYFLAG_OUT_CRLF;
        else if (strcasecmp(Token,"nosig")==0) Flags |= TTYFLAG_IGNSIG;
        else if (strcasecmp(Token,"save")==0) Flags |= TTYFLAG_SAVE;
        else if (isnum(Token) && Speed) *Speed=atoi(Token);

        ptr=GetToken(ptr," |,",&Token,GETTOKEN_MULTI_SEP);
    }

    DestroyString(Token);

    return(Flags);
}


int TTYConfigOpen(const char *Dev, const char *Config)
{
    int Flags, Speed=0;

    Flags=TTYParseConfig(Config, &Speed);
    return(TTYOpen(Dev, Speed, Flags));
}




int PseudoTTYGrabUnix98(int *master, int *slave, int TermFlags)
{
    char *Tempstr=NULL;

//first try unix98 style
    *master=open("/dev/ptmx",O_RDWR);
    if (*master > -1)
    {
        if (grantpt(*master)==-1) RaiseError(ERRFLAG_ERRNO, "pty", "grantpt failed");
        if (unlockpt(*master)==-1) RaiseError(ERRFLAG_ERRNO, "pty", "unlockpt failed");
        Tempstr=SetStrLen(Tempstr,100);
				memset(Tempstr, 0, 100);

#ifdef HAVE_PTSNAME_R
        if (ptsname_r(*master,Tempstr,100) != 0)
#endif

        Tempstr=CopyStr(Tempstr, ptsname(*master));
        if (StrValid(Tempstr))
        {
            if ( (*slave=open(Tempstr,O_RDWR)) >-1)
            {
                if (TermFlags !=0) TTYConfig(*slave,0,TermFlags);
                DestroyString(Tempstr);
                return(TRUE);
            }
            else RaiseError(ERRFLAG_ERRNO, "pty", "failed to open %s", Tempstr);
        }
        close(*master);
    }
    else
    {
        RaiseError(ERRFLAG_ERRNO, "pty", "failed to open /dev/ptmx");
    }

Destroy(Tempstr);
return(FALSE);
}




int PseudoTTYGrabBSD(int *master, int *slave, int TermFlags)
{
    char c1,c2;
    char *Tempstr=NULL;

//if unix98 fails, try old BSD style

    for (c1='p'; c1 < 's'; c1++)
    {
        for (c2='5'; c2 <='9'; c2++)
        {
            Tempstr=FormatStr(Tempstr,"/dev/pty%c%c",c1,c2);
            if ( (*master=open(Tempstr,O_RDWR)) >-1)
            {
                Tempstr=FormatStr(Tempstr,"/dev/tty%c%c",c1,c2);
                if ( (*slave=TTYOpen(Tempstr,0,TermFlags)) >-1)
                {
                    DestroyString(Tempstr);
                    return(TRUE);
                }
                else close(*master);
            }

        }

    }

    RaiseError(0, "pty", "failed to grab pseudo tty");
    DestroyString(Tempstr);
    return(FALSE);
}


int PseudoTTYGrab(int *master, int *slave, int TermFlags)
{
if (PseudoTTYGrabUnix98(master, slave, TermFlags)) return(TRUE);
if (PseudoTTYGrabBSD(master, slave, TermFlags)) return(TRUE);

return(FALSE);
}


