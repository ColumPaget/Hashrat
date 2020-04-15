#include "UnixSocket.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "Errors.h"

// crazyshit function because different implementations treat
// sun_path differently, so we have to derive a safe length for it!
void sun_set_path(struct sockaddr_un *sa, const char *Path)
{
    int len, val;
    char *ptr1, *ptr2;
    off_t pos;

    memset(sa,0,sizeof(struct sockaddr_un));
    sa->sun_family=AF_UNIX;
    ptr1=sa->sun_path;
    ptr2=(char *) sa; //sa is already a pointer, so we don't need &

    len=sizeof(struct sockaddr_un) - (ptr1-ptr2);

    if (len > 0)
    {
        len--; //force room for terminating \0

        //all hail strlcpy, makes life easier
#ifdef strlcpy
        strlcpy(sa->sun_path,Path,len);

        //else grrrr
#else
        val=StrLen(Path);
        if (val < len) len=val;
        strncpy(sa->sun_path,Path,len);
        sa->sun_path[len]='\0';
#endif

    }

}


int OpenUnixSocket(const char *Path, int Type)
{
    int sock;
    struct sockaddr_un sa;
    int val;

    if (Type==0) Type=SOCK_STREAM;

    sock=socket(PF_UNIX, Type, FALSE);
    if (sock==-1) return(-1);

    sun_set_path(&sa, Path);
    val=sizeof(sa);
    if (connect(sock,(struct sockaddr *) &sa,val)==0) return(sock);

    close(sock);
    return(-1);
}



int STREAMConnectUnixSocket(STREAM *S, const char *Path, int ConType)
{

    S->in_fd=OpenUnixSocket(Path, ConType);
    if (S->in_fd==-1) return(FALSE);
    S->out_fd=S->in_fd;
    S->Type=ConType;

    return(TRUE);
}



int UnixServerInit(int Type, const char *Path)
{
    int sock;
    struct sockaddr_un sa;
    socklen_t salen;
    int result;

//if (Type==0) Type=SOCK_STREAM;
    sock=socket(PF_UNIX, Type, 0);

    if (sock <0)
    {
        if (result != 0) RaiseError(ERRFLAG_ERRNO, "UnixServerInit","failed to create a unix socket.");
        return(-1);
    }

//No reason to pass server/listen sockets across an exec
    fcntl(sock, F_SETFD, FD_CLOEXEC);

    unlink(Path);
    sun_set_path(&sa, Path);
    salen=sizeof(struct sockaddr_un);
    result=bind(sock,(struct sockaddr *) &sa, salen);

    if (result != 0) RaiseError(ERRFLAG_ERRNO, "UnixServerInit","failed to bind unix sock to %s.",Path);
    else if (Type==SOCK_STREAM)
    {
        result=listen(sock,10);
        if (result != 0) RaiseError(ERRFLAG_ERRNO, "UnixServerInit","failed to 'listen' on unix sock %s.",Path);
    }

    if (result==0) return(sock);
    else
    {
        close(sock);
        return(-1);
    }
}



int UnixServerAccept(int ServerSock)
{
    struct sockaddr_un sa;
    socklen_t salen;
    int sock;

    salen=sizeof(sa);
    sock=accept(ServerSock,(struct sockaddr *) &sa,&salen);
    return(sock);
}



