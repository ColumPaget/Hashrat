#include "Socket.h"
#include "ConnectionChain.h"
#include "URL.h"
#include "UnixSocket.h"
#include "FileSystem.h"
#include "IPAddress.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>


#ifdef linux
#include <linux/netfilter_ipv4.h>
#endif



static void SocketParseConfigFlags(const char *Config, TSockSettings *Settings)
{
    const char *ptr;

    for (ptr=Config; *ptr !='\0'; ptr++)
    {
        if (isspace(*ptr))
        {
            ptr++;
            break;
        }

        switch (*ptr)
        {
        case 'n':
            Settings->Flags |= CONNECT_NONBLOCK;
            break;
        case 'E':
            Settings->Flags |= CONNECT_ERROR;
            break;
        case 'k':
            Settings->Flags |= SOCK_NOKEEPALIVE;
            break;
        case 'A':
            Settings->Flags |= SOCK_TLS_AUTO;
            break;
        case 'B':
            Settings->Flags |= SOCK_BROADCAST;
            break;
#ifdef TCP_FASTOPEN
        case 'F':
            Settings->Flags |= SOCK_TCP_FASTOPEN;
            break;
#endif
        case 'R':
            Settings->Flags |= SOCK_DONTROUTE;
            break;
        case 'P':
            Settings->Flags |= SOCK_REUSEPORT;
            break;
        case 'N':
            Settings->Flags |= SOCK_TCP_NODELAY;
            break;
        }
    }

}



int SocketParseConfig(const char *Config, TSockSettings *Settings)
{
    const char *ptr;
    char *Name=NULL, *Value=NULL;

    Settings->Flags=0;
    Settings->QueueLen=0;
    Settings->Perms=-1;
    ptr=LibUsefulGetValue("TCP:Keepalives");
    if ( StrValid(ptr) &&  (! strtobool(ptr)) ) Settings->Flags |= SOCK_NOKEEPALIVE;


    if (! StrValid(Config)) return(0);

    ptr=GetNameValuePair(Config, "\\S", "=", &Name, &Value);

    //if first item has no value, assume it is a list of setting flags
    //rather than a name=value pair setting
    if (! StrValid(Value))
    {
        SocketParseConfigFlags(Name, Settings);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    while (ptr)
    {
        if (strcasecmp(Name, "listen")==0) Settings->QueueLen=atoi(Value);
        else if (strcasecmp(Name, "ttl")==0) Settings->TTL=atoi(Value);
        else if (strcasecmp(Name, "tos")==0) Settings->ToS=atoi(Value);
        else if (strcasecmp(Name, "mark")==0) Settings->Mark=atoi(Value);
        else if (strcasecmp(Name, "mode")==0) Settings->Perms=FileSystemParsePermissions(Value);
        else if (strcasecmp(Name, "perms")==0) Settings->Perms=FileSystemParsePermissions(Value);
        else if (strcasecmp(Name, "permissions")==0) Settings->Perms=FileSystemParsePermissions(Value);
        else if (strcasecmp(Name,"keepalive")==0)
        {
            if (StrLen(Value) && (strncasecmp(Value, "n",1)==0)) Settings->Flags |= SOCK_NOKEEPALIVE;
        }
        else if (strcasecmp(Name,"timeout")==0)
        {
            Settings->Timeout=atoi(Value);
        }
        //else STREAMSetValue(S, Name, Value);

        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);

    return(Settings->Flags);
}



int IsSockConnected(int sock)
{
    struct sockaddr_in sa;
    socklen_t salen;
    int result;

    if (sock==-1) return(FALSE);
    salen=sizeof(sa);
    result=getpeername(sock,(struct sockaddr *) &sa, &salen);
    if (result==0) return(TRUE);
    if (errno==ENOTCONN) return(SOCK_CONNECTING);
    return(FALSE);
}


int SockSetOpt(int sock, int Opt, const char *Name, int OnOrOff)
{
    int val=OnOrOff, level;

    if (strncmp(Name, "TCP_", 4)==0) level=IPPROTO_TCP;
    else level=SOL_SOCKET;

    if (setsockopt(sock, level, Opt, &val, sizeof(int)) != 0)
    {
        RaiseError(ERRFLAG_ERRNO, "Failed to setsockopt '%s'",Name);
        return(FALSE);
    }
    return(TRUE);
}


//Socket options wrapped in ifdef statements to handle systems that lack certain options
void SockSetOptions(int sock, int SetFlags, int UnsetFlags)
{
#ifdef SO_BROADCAST
    if (SetFlags & SOCK_BROADCAST) SockSetOpt(sock, SO_BROADCAST, "SOCK_BROADCAST", 1);
#endif

#ifdef SO_DONTROUTE
    if (SetFlags & SOCK_DONTROUTE) SockSetOpt(sock, SO_DONTROUTE, "SOCK_DONTROUTE", 1);
#endif

#ifdef SO_REUSEPORT
    if (SetFlags & SOCK_REUSEPORT) SockSetOpt(sock, SO_REUSEPORT, "SOCK_REUSEPORT", 1);
#endif

#ifdef SO_PASSCRED
    if (SetFlags & SOCK_PEERCREDS) SockSetOpt(sock, SO_PASSCRED,  "SOCK_PEERCREDS", 1);
#endif

#ifdef TCP_NODELAY
    if (SetFlags & SOCK_TCP_NODELAY) SockSetOpt(sock, TCP_NODELAY, "TCP_NODELAY", 1);
#endif

#ifdef TCP_FASTOPEN
    if (SetFlags & SOCK_TCP_FASTOPEN) SockSetOpt(sock, TCP_FASTOPEN, "TCP_FASTOPEN", 1);
#endif


#ifdef SO_KEEPALIVE
    //Default is KEEPALIVE ON
    //SOCK_NOKEEPALIVE unsets the default, so looks opposite to all the others
    SockSetOpt(sock, SO_KEEPALIVE, "SO_KEEPALIVE", 1);
#endif


    if (SetFlags & CONNECT_NONBLOCK) fcntl(sock,F_SETFL,O_NONBLOCK);


#ifdef SO_BROADCAST
    if (UnsetFlags & SOCK_BROADCAST) SockSetOpt(sock, SO_BROADCAST, "SOCK_BROADCAST", 0);
#endif

#ifdef SO_DONTROUTE
    if (UnsetFlags & SOCK_DONTROUTE) SockSetOpt(sock, SO_DONTROUTE, "SOCK_DONTROUTE", 0);
#endif

#ifdef SO_REUSEPORT
    if (UnsetFlags & SOCK_REUSEPORT) SockSetOpt(sock, SO_REUSEPORT, "SOCK_REUSEPORT", 0);
#endif

#ifdef SO_PASSCRED
    if (UnsetFlags & SOCK_PEERCREDS) SockSetOpt(sock, SO_PASSCRED, "SOCK_PASSCRED", 0);
#endif

#ifdef TCP_NODELAY
    if (UnsetFlags & SOCK_TCP_NODELAY) SockSetOpt(sock, TCP_NODELAY, "TCP_NODELAY", 0);
#endif

#ifdef TCP_FASTOPEN
    if (UnsetFlags & SOCK_TCP_FASTOPEN) SockSetOpt(sock, TCP_FASTOPEN, "TCP_FASTOPEN", 0);
#endif

#ifdef SO_KEEPALIVE
    //SOCK_NOKEEPALIVE unsets the default, so looks opposite to all the others
    if (SetFlags & SOCK_NOKEEPALIVE) SockSetOpt(sock, SO_KEEPALIVE, "SOCK_NOKEEPALIVE", 0);
#endif

}






int IP6SockAddrCreate(struct sockaddr **ret_sa, const char *Addr, int Port)
{
    struct sockaddr_in6 *sa6;
    char *Token=NULL, *wptr;
    const char *ptr;
    socklen_t salen;


    sa6=(struct sockaddr_in6 *) calloc(1,sizeof(struct sockaddr_in6));
    if (StrValid(Addr))
    {
        if (IsIP4Address(Addr))
        {
            Token=MCopyStr(Token,"::ffff:",Addr,NULL);
            ptr=Token;
        }
        else
        {
            ptr=GetToken(Addr, "%",&Token,0);
            if (StrValid(ptr)) sa6->sin6_scope_id=if_nametoindex(ptr);

            ptr=Token;
            if (*ptr == '[')
            {
                ptr++;
                wptr=strchr(ptr,']');
                if (wptr) *wptr='\0';
            }
        }

        inet_pton(AF_INET6, ptr, &(sa6->sin6_addr));
    }
    else sa6->sin6_addr=in6addr_any;
    sa6->sin6_port=htons(Port);
    sa6->sin6_family=AF_INET6;

    *ret_sa=(struct sockaddr *) sa6;
    salen=sizeof(struct sockaddr_in6);


    DestroyString(Token);

    return(salen);
}


int IP4SockAddrCreate(struct sockaddr **ret_sa, const char *Addr, int Port)
{
    struct sockaddr_in *sa4;
    socklen_t salen;

    sa4=(struct sockaddr_in *) calloc(1,sizeof(struct sockaddr_in));
    if (! StrValid(Addr)) sa4->sin_addr.s_addr=INADDR_ANY;
    else sa4->sin_addr.s_addr=StrtoIP(Addr);

    sa4->sin_port=htons(Port);
    sa4->sin_family=AF_INET;
    *ret_sa=(struct sockaddr *) sa4;
    salen=sizeof(struct sockaddr_in);
    return(salen);
}


int SockAddrCreate(struct sockaddr **ret_sa, const char *Host, int Port)
{
    const char *p_Addr="";
    socklen_t salen;

    if (StrValid(Host))
    {
        if (! IsIPAddress(Host)) p_Addr=LookupHostIP(Host);
        else p_Addr=Host;
    }

#ifdef USE_INET6
    if (IsIP6Address(p_Addr)) return(IP6SockAddrCreate(ret_sa, p_Addr, Port));
#endif

    return(IP4SockAddrCreate(ret_sa, p_Addr, Port));
}




int BindSock(int Type, const char *Address, int Port, int Flags)
{
    int result;
    struct sockaddr *sa;
    socklen_t salen;
    int fd;

    salen=SockAddrCreate(&sa, Address, Port);
    if (salen==0) return(-1);
    if (Flags & BIND_RAW)
    {
        if (Type==SOCK_STREAM) fd=socket(sa->sa_family, SOCK_RAW, IPPROTO_TCP);
        else if (Type==SOCK_DGRAM) fd=socket(sa->sa_family, SOCK_RAW, IPPROTO_UDP);
    }
    else fd=socket(sa->sa_family, Type, 0);

    //REUSEADDR and REUSEPORT must be set BEFORE bind
    result=1;
#ifdef SO_REUSEADDR
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &result, sizeof(result));
#endif

#ifdef SO_REUSEPORT
    if (Flags & BIND_REUSEPORT) setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &result, sizeof(result));
#endif

    result=bind(fd, sa, salen);
    free(sa);

    if (result !=0)
    {
        close(fd);
        return(-1);
    }

    //No reason to pass server/listen fdets across an exec
    if (Flags & BIND_CLOEXEC) fcntl(fd, F_SETFD, FD_CLOEXEC);

    if (Flags & BIND_LISTEN) result=listen(fd,10);


    return(fd);
}


int GetHostARP(const char *IP, char **Device, char **MAC)
{
    char *Tempstr=NULL, *Token=NULL;
    int result=FALSE;
    const char *ptr;
    FILE *F;

    Tempstr=SetStrLen(Tempstr, 255);
    F=fopen("/proc/net/arp","r");
    if (F)
    {
        *Device=CopyStr(*Device,"remote");
        *MAC=CopyStr(*MAC,"remote");
        //Read Title Line
        fgets(Tempstr,255,F);

        while (fgets(Tempstr,255,F))
        {
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr," ",&Token,0);
            if (strcmp(Token,IP)==0)
            {
                while (isspace(*ptr)) ptr++;
                ptr=GetToken(ptr," ",&Token,0);

                while (isspace(*ptr)) ptr++;
                ptr=GetToken(ptr," ",&Token,0);

                while (isspace(*ptr)) ptr++;
                ptr=GetToken(ptr," ",MAC,0);
                strlwr(*MAC);

                while (isspace(*ptr)) ptr++;
                ptr=GetToken(ptr," ",&Token,0);

                while (isspace(*ptr)) ptr++;
                ptr=GetToken(ptr," ",Device,0);

                result=TRUE;
            }
        }
        fclose(F);
    }

    DestroyString(Tempstr);
    DestroyString(Token);

    return(result);
}


/* This is a bit of kernel magic to decide where the client was trying */
/* to connect to before it got transparently proxied */
int GetSockDestination(int sock, char **Host, int *Port)
{
    int salen;
    struct sockaddr_storage sa;
    char *Tempstr=NULL;
    int result=FALSE;

#ifdef SO_ORIGINAL_DST
    salen=sizeof(struct sockaddr_in);

    if (getsockopt(sock, SOL_IP, SO_ORIGINAL_DST, (char *) &sa, &salen) ==0)
    {
        *Host=SetStrLen(*Host,NI_MAXHOST);
        Tempstr=SetStrLen(Tempstr,NI_MAXSERV);
        salen=sizeof(struct sockaddr_storage);
        getnameinfo((struct sockaddr *) &sa, salen, *Host, NI_MAXHOST, Tempstr, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);
        *Port=atoi(Tempstr);
    }

    result=TRUE;
#endif

    DestroyString(Tempstr);

    return(result);
}


char *GetInterfaceDetails(char *RetStr, const char *Interface)
{
    int fd, result;
    struct ifreq ifr;
    char *Tempstr=NULL;

    RetStr=CopyStr(RetStr, "");
    if (! StrValid(Interface)) return(RetStr);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) return(RetStr);

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, Interface, IFNAMSIZ-1);

    result=ioctl(fd, SIOCGIFADDR, &ifr);
    if (result > -1) RetStr=MCopyStr(RetStr, "ip4address=", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), " ", NULL);

    result=ioctl(fd, SIOCGIFBRDADDR, &ifr);
    RetStr=MCatStr(RetStr, "ip4broadcast=", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr), " ", NULL);

    result=ioctl(fd, SIOCGIFNETMASK, &ifr);
    RetStr=MCatStr(RetStr, "ip4netmask=", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr), " ", NULL);

    result=ioctl(fd, SIOCGIFDSTADDR, &ifr);
    RetStr=MCatStr(RetStr, "ip4destaddr=", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_dstaddr)->sin_addr), " ", NULL);

    result=ioctl(fd, SIOCGIFMTU, &ifr);
    Tempstr=FormatStr(Tempstr, "%d", ifr.ifr_mtu);
    RetStr=MCatStr(RetStr, "ip4destaddr=", Tempstr, " ", NULL);

    close(fd);

    Destroy(Tempstr);
    return(RetStr);
}



const char *GetInterfaceIP(const char *Interface)
{
    int fd, result;
    struct ifreq ifr;

    if (! StrValid(Interface)) return("");
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) return("");

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, Interface, IFNAMSIZ-1);
    result=ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    if (result==-1) return("");

    return(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}


#ifdef __GNU_LIBRARY__


#include <netinet/ip_icmp.h>

/*--------------------------------------------------------------------*/
/*--- checksum - standard 1s complement checksum                   ---*/
/*--------------------------------------------------------------------*/
static unsigned short ip_checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}


int ICMPSend(int sock, const char *Host, int Type, int Code, int TTL, char *Data, int len)
{
    struct icmphdr *ICMPHead;
    char *Tempstr=NULL;
    int packet_len;
    static int seq=0;
    struct sockaddr_in sa;
    int result;

    result=TTL;
    if (setsockopt(sock, SOL_IP, IP_TTL, &result, sizeof(int)) != 0) return(0);

    packet_len=len+sizeof(struct icmphdr);
    Tempstr=(char *) calloc(1, packet_len+1);
    ICMPHead=(struct icmphdr *) Tempstr;
    ICMPHead->type=ICMP_ECHO;
    ICMPHead->un.echo.id=getpid();
    ICMPHead->un.echo.sequence=seq++;
    if ((len > 0) && Data) memcpy(Tempstr + sizeof(struct icmphdr), Data, len);
    ICMPHead->checksum=ip_checksum(Tempstr, packet_len);

    memset(&sa, 0, sizeof(sa));
    sa.sin_family=AF_INET;
    sa.sin_port=0;
    sa.sin_addr.s_addr=StrtoIP(Host);
    result=sendto(sock, Tempstr, packet_len, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr));

    DestroyString(Tempstr);

    return(result);
}

#endif




int UDPOpen(const char *Addr, int Port, int Flags)
{
    int fd;

    fd=BindSock(SOCK_DGRAM, Addr, Port, 0);
    if (fd > -1) SockSetOptions(fd, Flags, 0);
    return(fd);
}


int UDPRecv(int sock,  char *Buffer, int len, char **Addr, int *Port)
{
    char *Tempstr=NULL;
    struct sockaddr_in sa;
    socklen_t salen;
    int result;
    int fd;

    salen=sizeof(sa);
    result=recvfrom(sock, Buffer, len,0, (struct sockaddr *) &sa, &salen);
    if (result > -1)
    {
        *Addr=SetStrLen(*Addr,NI_MAXHOST);
        Tempstr=SetStrLen(Tempstr,NI_MAXSERV);
        getnameinfo((struct sockaddr *) &sa, salen, *Addr, NI_MAXHOST, Tempstr, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);

        *Port=atoi(Tempstr);
    }

    DestroyString(Tempstr);

    return(result);
}



int UDPSend(int sock, const char *Host, int Port, char *Data, int len)
{
    struct sockaddr *sa=NULL;
    socklen_t salen;
    int result;

    salen=SockAddrCreate(&sa, Host, Port);
    if (salen==0) return(-1);

    result=sendto(sock, Data, len, 0, sa, salen);
    free(sa);
    return(result);
}



int STREAMSendDgram(STREAM *S, const char *Host, int Port, char *Data, int len)
{
    return(UDPSend(S->out_fd, Host, Port, Data, len));
}


STREAM *STREAMFromSock(int sock, int Type, const char *Peer, const char *DestIP, int DestPort)
{
    STREAM *S;
    char *Tempstr=NULL;

    S=STREAMFromFD(sock);
    if (! S) return(S);

    S->Type=Type;
    if (StrValid(Peer))
    {
        if (strncmp(Peer,"::ffff:",7)==0) STREAMSetValue(S,"PeerIP",Peer+7);
        else STREAMSetValue(S,"PeerIP",Peer);
    }

    if (StrValid(DestIP))
    {
        if (strncmp(DestIP,"::ffff:",7)==0) STREAMSetValue(S,"DestIP",DestIP+7);
        else STREAMSetValue(S,"DestIP",DestIP);
        if (DestPort > 0)
        {
            Tempstr=FormatStr(Tempstr, "%d",DestPort);
            STREAMSetValue(S,"DestPort",Tempstr);
        }
    }

    DestroyString(Tempstr);

    return(S);
}




void IP6AddresssFromSA(struct sockaddr_storage *sa, char **ReturnAddr, int *ReturnPort)
{
    int salen;
    char *Tempstr=NULL, *PortStr=NULL;
    const char *ptr;

    salen=sizeof(struct sockaddr_storage);

    Tempstr=SetStrLen(Tempstr,NI_MAXHOST);
    PortStr=SetStrLen(PortStr,NI_MAXSERV);
    getnameinfo((struct sockaddr *) sa, salen, Tempstr, NI_MAXHOST, PortStr, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);
    if (ReturnPort) *ReturnPort=atoi(PortStr);

    if (ReturnAddr)
    {
        if ((strncmp(Tempstr,"::ffff:",7)==0) && strchr(Tempstr,'.')) ptr=Tempstr+7;
        else ptr=Tempstr;
        *ReturnAddr=CopyStr(*ReturnAddr, ptr);
    }

    DestroyString(Tempstr);
    DestroyString(PortStr);
}



#ifdef USE_INET6
int GetSockDetails(int sock, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort)
{
    socklen_t salen;
    int result;
    struct sockaddr_storage sa;

    if (LocalPort) *LocalPort=0;
    if (RemotePort) *RemotePort=0;
    if (LocalAddress) *LocalAddress=CopyStr(*LocalAddress,"");
    if (RemoteAddress) *RemoteAddress=CopyStr(*RemoteAddress,"");

    salen=sizeof(struct sockaddr_storage);
    if (getsockname(sock, (struct sockaddr *) &sa, &salen) != 0)
    {
        RaiseError(ERRFLAG_ERRNO, "GetSockDetails", "failed to get local endpoint");
        return(FALSE);
    }

    IP6AddresssFromSA(&sa, LocalAddress, LocalPort);
    //Set Address to be the same as control sock, as it might not be INADDR_ANY
    if (getpeername(sock, (struct sockaddr *) &sa, &salen) == 0) IP6AddresssFromSA(&sa, RemoteAddress, RemotePort);

    return(TRUE);
}

#else
int GetSockDetails(int sock, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort)
{
    socklen_t salen;
    int result;
    struct sockaddr_in sa;

    if (LocalPort) *LocalPort=0;
    if (RemotePort) *RemotePort=0;
    if (LocalAddress) *LocalAddress=CopyStr(*LocalAddress,"");
    if (RemoteAddress) *RemoteAddress=CopyStr(*RemoteAddress,"");

    salen=sizeof(struct sockaddr_in);
    result=getsockname(sock, (struct sockaddr *) &sa, &salen);

    if (result==0)
    {
        if (LocalAddress) *LocalAddress=CopyStr(*LocalAddress,IPtoStr(sa.sin_addr.s_addr));
        if (LocalPort) *LocalPort=ntohs(sa.sin_port);

        //Set Address to be the same as control sock, as it might not be INADDR_ANY
        result=getpeername(sock, (struct sockaddr *) &sa, &salen);

        if (result==0)
        {
            if (RemoteAddress) *RemoteAddress=CopyStr(*RemoteAddress,IPtoStr(sa.sin_addr.s_addr));
            if (RemotePort) *RemotePort=ntohs(sa.sin_port);
        }

        //We've got the local sock, so lets still call it a success
        result=0;
    }

    if (result==0) return(TRUE);
    return(FALSE);
}

#endif



/* Users will probably only use this function if they want to reconnect   */
/* a broken connection, or reuse a socket for multiple connections, hence */
/* the name... */
int IPReconnect(int sock, const char *Host, int Port, int Flags)
{
    socklen_t salen;
    int result;
    struct sockaddr *sa;


    salen=SockAddrCreate(&sa, Host, Port);
    if (salen==0) return(-1);

    SockSetOptions(sock, Flags, 0);

    result=connect(sock,(struct sockaddr *) sa, salen);
    if (result==0) result=TRUE;

    if ((result==-1) && (Flags & CONNECT_NONBLOCK) && (errno == EINPROGRESS)) result=FALSE;

    if (result==-1)
    {
        //if ( (Flags & CONNECT_ERROR) || (! LibUsefulGetBool("Error:IgnoreIP")) ) RaiseError(ERRFLAG_ERRNO, "IPConnect", "1: failed to connect to %s:%d", Host, Port);
        if (Flags & CONNECT_ERROR) RaiseError(ERRFLAG_ERRNO, "IPConnect", "failed to connect to %s:%d", Host, Port);
        else RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "IPConnect", "failed to connect to %s:%d", Host, Port);
    }

    free(sa);
    return(result);
}


int NetConnectWithSettings(const char *Proto, const char *LocalHost, const char *Host, int Port, TSockSettings *Settings)
{
    const char *p_LocalHost=LocalHost;
    int sock, result;

    if ((! StrValid(p_LocalHost)) && IsIP6Address(Host)) p_LocalHost="::";
    if ((strcasecmp(Proto,"udp")==0) || (strcasecmp(Proto,"bcast")==0))
    {
        sock=BindSock(SOCK_DGRAM, p_LocalHost, 0, 0);
        if (strcasecmp(Proto,"bcast")==0) Settings->Flags |= SOCK_BROADCAST;
    }
    else sock=BindSock(SOCK_STREAM, p_LocalHost, 0, 0);

    if (Settings->TTL > 0) setsockopt(sock, IPPROTO_IP, IP_TTL, &(Settings->TTL), sizeof(int));
    if (Settings->ToS > 0) setsockopt(sock, IPPROTO_IP, IP_TOS, &(Settings->ToS), sizeof(int));

#ifdef SO_MARK
    if (Settings->Mark > 0) setsockopt(sock, SOL_SOCKET, SO_MARK, &(Settings->Mark), sizeof(int));
#endif

    result=IPReconnect(sock, Host, Port, Settings->Flags);
    if (result==-1)
    {
        close(sock);
        return(-1);
    }

    return(sock);
}



int NetConnectWithAttributes(const char *Proto, const char *LocalHost, const char *Host, int Port, const char *Config)
{
    TSockSettings Settings;

    memset(&Settings, 0, sizeof(TSockSettings));
    SocketParseConfig(Config, &Settings);

    return(NetConnectWithSettings(Proto, LocalHost, Host, Port, &Settings));
}



int TCPConnect(const char *Host, int Port, const char *Config)
{
    return(NetConnectWithAttributes("tcp", "", Host, Port, Config));
}


const char *GetRemoteIP(int sock)
{
    struct sockaddr_in sa;
    socklen_t salen;
    int result;

    salen=sizeof(struct sockaddr_in);
    result=getpeername(sock,(struct sockaddr *) &sa, &salen);
    if  (result==-1)
    {
        if (errno==ENOTSOCK)  return("127.0.0.1");
        else return("0.0.0.0");
    }

    return((char *) inet_ntoa(sa.sin_addr));
}



int STREAMIsConnected(STREAM *S)
{
    int result=FALSE;

    if (! S) return(FALSE);
    result=IsSockConnected(S->in_fd);
    if (result==TRUE)
    {
        if (S->State & SS_CONNECTING)
        {
            S->State |= SS_CONNECTED;
            S->State &= (~SS_CONNECTING);
        }
    }
    if ((result==SOCK_CONNECTING) && (! (S->State & SS_CONNECTING))) result=FALSE;
    return(result);
}



int STREAMDoPostConnect(STREAM *S, int Flags)
{
    int result=FALSE;
    const char *ptr;

    if (! S) return(FALSE);

    if (S->in_fd > -1)
    {
//We will have been non-blocking during connection, but if we don't
//really want the stream to be non blocking, we unset that here
        if (! (Flags & CONNECT_NONBLOCK))  STREAMSetFlags(S, 0, SF_NONBLOCK);

        STREAMSetFlushType(S,FLUSH_LINE,0,0);
        if (Flags & CONNECT_SSL)
        {
            STREAMSetFlags(S, 0, SF_NONBLOCK);
            DoSSLClientNegotiation(S, Flags);
            //  	if (Flags & CONNECT_NONBLOCK) STREAMSetFlags(S, SF_NONBLOCK, 0);
        }

        ptr=GetRemoteIP(S->in_fd);
        if (ptr)
        {
            if (strncmp(ptr,"::ffff:",7)==0) ptr+=7;
            STREAMSetValue(S,"PeerIP",ptr);
        }

        result=TRUE;
        S->State |=SS_CONNECTED;
    }
    else
    {
        if ( (Flags & CONNECT_ERROR) || (! LibUsefulGetBool("Error:IgnoreIP")) ) RaiseError(ERRFLAG_ERRNO, "IPConnect", "failed to connect to %s", S->Path);
        else RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "IPConnect", "failed to connect to %s", S->Path);
    }



    return(result);
}

extern char *GlobalConnectionChain;



static int STREAMWaitConnect(STREAM *S)
{
    struct timeval tv;

    if ((S->in_fd > -1) && (S->Timeout > 0) )
    {
        //timeout is expressed in centisecs, so multiply by 10 to give millsecs then convert to a tv
        MillisecsToTV(S->Timeout * 10, &tv);

        if (FDSelect(S->in_fd, SELECT_WRITE, &tv) != SELECT_WRITE) return(FALSE);
    }

    return(STREAMIsConnected(S));
}



int STREAMNetConnect(STREAM *S, const char *Proto, const char *Host, int Port, const char *Config)
{
    int result=FALSE;
    TSockSettings Settings;

    memset(&Settings, 0, sizeof(TSockSettings));
    SocketParseConfig(Config, &Settings);


    if (StrValid(Host))
    {
        //Flags are handled in this function
        S->in_fd=NetConnectWithSettings(Proto, STREAMGetValue(S, "LocalAddress"), Host, Port, &Settings);
        S->Timeout=Settings.Timeout;
        S->out_fd=S->in_fd;
        if (S->in_fd > -1) result=TRUE;
    }


    if (result==TRUE)
    {
        if (strcasecmp(Proto, "ssl")==0) S->Type=STREAM_TYPE_SSL;
        else if (strcasecmp(Proto, "tls")==0) S->Type=STREAM_TYPE_SSL;
        else if (strcasecmp(Proto, "udp")==0) S->Type=STREAM_TYPE_UDP;
        else if (strcasecmp(Proto, "bcast")==0) S->Type=STREAM_TYPE_UDP;
        else S->Type=STREAM_TYPE_TCP;

        //if (S->Timeout > 0) S->Flags |= SF_NONBLOCK;

        if (S->Type==STREAM_TYPE_SSL) S->Flags |= CONNECT_SSL;
        if (S->Flags & SF_NONBLOCK)
        {
            S->State |=SS_CONNECTING;
            S->Flags |=SF_NONBLOCK;
        }

        if (STREAMWaitConnect(S)) result=STREAMDoPostConnect(S, S->Flags);
    }

    return(result);
}




int STREAMConnect(STREAM *S, const char *URL, const char *Config)
{
    int result=FALSE;
    char *Proto=NULL, *Host=NULL, *Token=NULL, *Path=NULL;
    char *Name=NULL, *Value=NULL;
    TSockSettings Settings;
    const char *ptr, *p_val;
    int Flags=0, fd;
    int Port=0;


//if URL is a comma-seperated list then it's a list of 'connection hops' through proxies
    if (StrValid(GlobalConnectionChain))
    {
        Value=MCopyStr(Value,GlobalConnectionChain,"|",URL, NULL);
        result=STREAMProcessConnectHops(S, Value);
    }
    else if (strchr(URL, '|')) result=STREAMProcessConnectHops(S, URL);
    else
    {
        ParseURL(URL, &Proto, &Host, &Token, NULL, NULL, &Path, NULL);
        if (StrValid(Token)) Port=strtoul(Token, 0, 10);

        STREAMSetFlushType(S,FLUSH_LINE,0,0);
        if (strcasecmp(Proto,"fifo")==0)
        {
            mknod(Host, S_IFIFO|0666, 0);
            S->in_fd=open(Host, O_RDWR);
            S->out_fd=S->in_fd;
        }
        else if (strcasecmp(Proto,"unix")==0)  result=STREAMConnectUnixSocket(S, URL+5, SOCK_STREAM); //whole of URL is path
        else if (strcasecmp(Proto,"unixstream")==0)  result=STREAMConnectUnixSocket(S, URL+11, SOCK_STREAM); //whole of URL is path
        else if (strcasecmp(Proto,"unixdgram")==0) result=STREAMConnectUnixSocket(S, URL+10, SOCK_DGRAM); //whole of URL is path
        else result=STREAMNetConnect(S, Proto, Host, Port, Config);
    }

    DestroyString(Token);
    DestroyString(Proto);
    DestroyString(Host);
    DestroyString(Path);
    DestroyString(Name);
    DestroyString(Value);



    return(result);
}

