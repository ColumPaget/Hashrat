#include "Server.h"
#include "URL.h"
#include "IPAddress.h"
#include "UnixSocket.h"
#include "HttpServer.h"
#include "WebSocket.h"
#include "StreamAuth.h"

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


static int TCPServerNew(const char *Host, int Port, int Flags, TSockSettings *Settings)
{
    int fd;

    fd=IPServerNew(SOCK_STREAM, Host, Port, Flags);
    if (Settings->QueueLen > 0)
    {
        listen(fd, Settings->QueueLen);
#ifdef TCP_FASTOPEN
        if (Flags & SOCK_TCP_FASTOPEN) SockSetOpt(fd, TCP_FASTOPEN, "TCP_FASTOPEN", Settings->QueueLen);
#endif
    }

    return(fd);
}



static void STREAMServerParseConfig(STREAM *S, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strncasecmp(Name, "SSL:", 4)==0) STREAMSetValue(S, Name, Value);
        else if (strcasecmp(Name, "Authentication")==0) STREAMSetValue(S, "Authenticator", Value);
        else if (strcasecmp(Name, "Auth")==0) STREAMSetValue(S, "Authenticator", Value);

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Value);
    Destroy(Name);
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
    case 'h':
        if (strcmp(Proto,"http")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_HTTP_SERVER;
        }
        else if (strcmp(Proto,"https")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_HTTP_SERVER;
            Flags |= SF_TLS;
        }
        break;


    case 's':
        if (strcmp(Proto,"ssl")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_TCP_SERVER;
            Flags |= SF_TLS;
        }
        break;

    case 't':
        if (strcmp(Proto,"tcp")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_TCP_SERVER;
        }
        else if (strcmp(Proto,"tls")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_TCP_SERVER;
            Flags |= SF_TLS;
        }
        else if (strcmp(Proto,"tproxy")==0)
        {
#ifdef SOCK_TPROXY
            fd=IPServerNew(SOCK_TPROXY, Host, Port, Flags);
            Type=STREAM_TYPE_TPROXY;
#endif
        }
        break;

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

    case 'w':
        if (strcmp(Proto, "ws")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_WS_SERVER;
        }
        else if (strcmp(Proto, "wss")==0)
        {
            fd=TCPServerNew(Host, Port, Flags, &Settings);
            Type=STREAM_TYPE_WS_SERVER;
            Flags |= SF_TLS;
        }
        break;
    }


    S=STREAMFromSock(fd, Type, NULL, Host, Port);
    if (S)
    {
        S->Path=CopyStr(S->Path, URL);
        if (Flags & SOCK_TLS_AUTO) S->Flags |= SF_TLS_AUTO;
        else if (Flags & SF_TLS) S->Flags |= SF_TLS;
        STREAMServerParseConfig(S, Config);
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

    case STREAM_TYPE_HTTP_SERVER:
        fd=IPServerAccept(Serv->in_fd, &Tempstr);
        GetSockDetails(fd, &DestIP, &DestPort, NULL, NULL);
        type=STREAM_TYPE_HTTP_ACCEPT;
        break;

    case STREAM_TYPE_WS_SERVER:
        fd=IPServerAccept(Serv->in_fd, &Tempstr);
        GetSockDetails(fd, &DestIP, &DestPort, NULL, NULL);
        type=STREAM_TYPE_WS_ACCEPT;
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
    if (S)
    {
        CopyVars(S->Values, Serv->Values);

        //things that we have to do post-accept for each type of socket
        switch (type)
        {
        case STREAM_TYPE_TCP_ACCEPT:
            //if TLS autodetection enabled, perform it now
            if ((Serv->Flags & SF_TLS_AUTO) && OpenSSLAutoDetect(S)) DoSSLServerNegotiation(S, LU_SSL_VERIFY_PEER);
            else if (Serv->Flags & SF_TLS) DoSSLServerNegotiation(S, LU_SSL_VERIFY_PEER);

            // for tcp and tls/ssl, if STREAMAuth fails, we disconnect
            if (! STREAMAuth(S))
            {
                STREAMClose(S);
                S=NULL;
            }
            break;

        case STREAM_TYPE_HTTP_ACCEPT:
            if ((Serv->Flags & SF_TLS_AUTO) && OpenSSLAutoDetect(S)) DoSSLServerNegotiation(S, LU_SSL_VERIFY_PEER);
            else if (Serv->Flags & SF_TLS) DoSSLServerNegotiation(S, LU_SSL_VERIFY_PEER);

            //HttpServer handles STREAMAuth internally
            HTTPServerAccept(S);
            break;

        case STREAM_TYPE_WS_ACCEPT:
            if ((Serv->Flags & SF_TLS_AUTO) && OpenSSLAutoDetect(S)) DoSSLServerNegotiation(S, LU_SSL_VERIFY_PEER);
            else if (Serv->Flags & SF_TLS) DoSSLServerNegotiation(S, LU_SSL_VERIFY_PEER);

            //Websocket handles STREAMAuth internally
            WebSocketAccept(S);
            break;
        }
    }

    DestroyString(Tempstr);
    DestroyString(DestIP);
    return(S);
}


