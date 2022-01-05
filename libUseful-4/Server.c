#include "Server.h"
#include "UnixSocket.h"
#include "URL.h"
#include "IPAddress.h"

int IPServerNew(int iType, const char *Address, int Port, int Flags)
{
    int sock, val, Type;
    int BindFlags=0;
    const char *p_Addr=NULL, *ptr;

//if IP6 not compiled in then throw error if one is passed
#ifndef USE_INET6
    if (IsIP6Address(Address)) return(-1);
#endif

    if (! IsIPAddress(Address)) p_Addr=GetInterfaceIP(Address);
    else p_Addr=Address;

    switch (iType)
    {
    case SOCK_TPROXY:
#ifdef IP_TRANSPARENT
        Type=SOCK_STREAM;
#else
        RaiseError(0, "IPServerInit","TPROXY/IPTRANSPARENT not available on this machine/platform");
        return(-1);
#endif
        break;

    case 0:
    case SOCK_STREAM:
        Type=SOCK_STREAM;
        break;

    default:
        Type=iType;
        break;
    }

    BindFlags=BIND_CLOEXEC | BIND_LISTEN;
    if (Flags & SOCK_REUSEPORT) BindFlags |= BIND_REUSEPORT;
    sock=BindSock(Type, p_Addr, Port, BindFlags);

    if (sock > -1) SockSetOptions(sock, Flags, 0);

#ifdef IP_TRANSPARENT
    if (iType==SOCK_TPROXY)
    {
        val=1;
        if (setsockopt(sock, IPPROTO_IP, IP_TRANSPARENT, &val, sizeof(int)) !=0)
        {
            RaiseError(ERRFLAG_ERRNO, "IPServerInit","set socket to TPROXY/IPTRANSPARENT failed.");
            close(sock);
            sock=-1;
        }
    }
#endif


    return(sock);
}



int IPServerInit(int iType, const char *Address, int Port)
{
    return(IPServerNew(iType, Address, Port, 0));
}


int IPServerAccept(int ServerSock, char **Addr)
{
    struct sockaddr_storage sa;
    socklen_t salen;
    int sock;

    salen=sizeof(sa);
    sock=accept(ServerSock,(struct sockaddr *) &sa, &salen);
    if (Addr && (sock > -1))
    {
        *Addr=SetStrLen(*Addr,NI_MAXHOST);
        getnameinfo((struct sockaddr *) &sa, salen, *Addr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    }
    return(sock);
}




STREAM *STREAMServerNew(const char *URL, const char *Config)
{
    char *Proto=NULL, *Host=NULL, *Token=NULL;
    int fd=-1, Port=0, Type, Flags=0;
    TSockSettings Settings;
    STREAM *S=NULL;

    ParseURL(URL, &Proto, &Host, &Token,NULL, NULL,NULL,NULL);
    if (StrValid(Token)) Port=atoi(Token);

    Flags=SocketParseConfig(Config, &Settings);

    switch (*Proto)
    {
    case 'u':
        if (strcmp(Proto,"udp")==0)
        {
            fd=IPServerNew(SOCK_DGRAM, Host, Port, Flags);
            Type=STREAM_TYPE_UDP;
        }
        else if (strcmp(Proto,"unix")==0)
        {
            fd=UnixServerInit(SOCK_STREAM, URL+5);
            Type=STREAM_TYPE_UNIX_SERVER;
            if (Settings.Perms > -1) chmod(URL+5, Settings.Perms);
        }
        else if (strcmp(Proto,"unixdgram")==0)
        {
            fd=UnixServerInit(SOCK_DGRAM, URL+10);
            Type=STREAM_TYPE_UNIX_DGRAM;
            if (Settings.Perms > -1) chmod(URL+10, Settings.Perms);
        }
        break;

    case 't':
        if (strcmp(Proto,"tcp")==0)
        {
            fd=IPServerNew(SOCK_STREAM, Host, Port, Flags);
            Type=STREAM_TYPE_TCP_SERVER;
            if (Settings.QueueLen > 0)
            {
                listen(fd, Settings.QueueLen);
#ifdef TCP_FASTOPEN
                if (Flags & SOCK_TCP_FASTOPEN) SockSetOpt(fd, TCP_FASTOPEN, "TCP_FASTOPEN", Settings.QueueLen);
#endif
            }
        }
        else if (strcmp(Proto,"tproxy")==0)
        {
#ifdef SOCK_TPROXY
            fd=IPServerNew(SOCK_TPROXY, Host, Port, Flags);
            Type=STREAM_TYPE_TPROXY;
#endif
        }
        break;
    }

    S=STREAMFromSock(fd, Type, NULL, Host, Port);
    if (S)
    {
        S->Path=CopyStr(S->Path, URL);
        if (Flags & SOCK_TLS_AUTO) S->Flags |= SF_TLS_AUTO;
    }

    DestroyString(Proto);
    DestroyString(Host);
    DestroyString(Token);

    return(S);
}


STREAM *STREAMServerInit(const char *URL)
{
    return(STREAMServerNew(URL, ""));
}


STREAM *STREAMServerAccept(STREAM *Serv)
{
    char *Tempstr=NULL, *DestIP=NULL;
    STREAM *S=NULL;
    int fd=-1, type=0, DestPort=0, result;

    if (! Serv) return(NULL);

    switch (Serv->Type)
    {
    case STREAM_TYPE_UNIX_SERVER:
        fd=UnixServerAccept(Serv->in_fd);
        type=STREAM_TYPE_UNIX_ACCEPT;
        break;

    case STREAM_TYPE_TCP_SERVER:
        fd=IPServerAccept(Serv->in_fd, &Tempstr);
        GetSockDetails(fd, &DestIP, &DestPort, NULL, NULL);
        type=STREAM_TYPE_TCP_ACCEPT;
        break;

    case STREAM_TYPE_TPROXY:
        fd=IPServerAccept(Serv->in_fd, &Tempstr);
        GetSockDestination(fd, &DestIP, &DestPort);
        type=STREAM_TYPE_TCP_ACCEPT;
        break;

    default:
        return(Serv);
        break;
    }

    S=STREAMFromSock(fd, type, Tempstr, DestIP, DestPort);
    if (type==STREAM_TYPE_TCP_ACCEPT)
    {
        //if TLS autodetection enabled, perform it now
        if ((Serv->Flags & SF_TLS_AUTO) && OpenSSLAutoDetect(S)) DoSSLServerNegotiation(S, 0);
    }

    DestroyString(Tempstr);
    DestroyString(DestIP);
    return(S);
}


