#include "ConnectionChain.h"
#include "URL.h"
#include "SpawnPrograms.h"
#include "Expect.h"
#include "SecureMem.h"
#include "Errors.h"
#include "Stream.h"
#include "Ssh.h"
#include "IPAddress.h"

typedef enum {CONNECT_HOP_NONE, CONNECT_HOP_TCP, CONNECT_HOP_HTTPTUNNEL, CONNECT_HOP_SSH, CONNECT_HOP_SSHTUNNEL, CONNECT_HOP_SSHPROXY, CONNECT_HOP_SOCKS4, CONNECT_HOP_SOCKS5, CONNECT_HOP_SHELL_CMD, CONNECT_HOP_TELNET} THopTypes;
static const char *HopTypes[]= {"none","tcp","https","ssh","sshtunnel","sshproxy","socks4","socks5","shell","telnet",NULL};

char *GlobalConnectionChain=NULL;
ListNode *ProxyHelpers=NULL;

int SetGlobalConnectionChain(const char *Chain)
{
    char *Token=NULL, *Type=NULL;
    const char *ptr;
    int result=TRUE, count=0;

    ptr=GetToken(Chain, "|", &Token, 0);
    while (ptr)
    {
        GetToken(Token,":",&Type,0);
        if (MatchTokenFromList(Type, HopTypes, 0)==-1) result=FALSE;
        count++;
        ptr=GetToken(ptr, "|", &Token, 0);
    }

    GlobalConnectionChain=CopyStr(GlobalConnectionChain, Chain);

    DestroyString(Token);
    DestroyString(Type);
    return(result);
}


//if a proxy helper was launched by the current program (check using getpid) then
//this is called by 'atexit' on exit, to shutdown that helper
void ConnectionHopCloseAll()
{
    ListNode *Curr;
    STREAM *S;
    pid_t pid;

    Curr=ListGetNext(ProxyHelpers);
    while (Curr)
    {
        S=(STREAM *) Curr->Item;
        pid=atoi(STREAMGetValue(S, "LU:LauncherPID"));
        if (pid==getpid()) STREAMClose(S);
        Curr=ListGetNext(Curr);
    }
}


int ConnectHopHTTPSProxy(STREAM *S, const char *Proxy, const char *Destination)
{
    char *Tempstr=NULL, *Token=NULL;
    char *Host=NULL, *User=NULL, *Pass=NULL;
    const char *ptr=NULL;
    int result=FALSE, Port;

    ParseConnectDetails(Proxy, &Token, &Host, &Token, &User, &Pass, NULL);
    Port=atoi(Token);
    if (! (S->State & SS_INITIAL_CONNECT_DONE))
    {
        if (Port==0) Port=443;
        S->in_fd=TCPConnect(Host, Port, "");
        S->out_fd=S->in_fd;
        if (S->in_fd == -1)
        {
            RaiseError(0, "ConnectHopHTTPSProxy", "failed to connect to proxy at %s:%d", Host, Port);
            return(FALSE);
        }
    }

    ptr=Destination;
    if (strncmp(ptr,"tcp:",4)==0) ptr+=4;
    Tempstr=FormatStr(Tempstr,"CONNECT %s HTTP/1.1\r\n\r\n",ptr);

    STREAMWriteLine(Tempstr,S);
    STREAMFlush(S);

    Tempstr=STREAMReadLine(Tempstr,S);
    if (Tempstr)
    {
        StripTrailingWhitespace(Tempstr);

        ptr=GetToken(Tempstr," ",&Token,0);
        ptr=GetToken(ptr," ",&Token,0);

        if (*Token=='2') result=TRUE;
        else RaiseError(0, "ConnectHopHTTPSProxy", "proxy request to %s:%d failed. %s", Host, Port, Tempstr);

        while (StrValid(Tempstr))
        {
            Tempstr=STREAMReadLine(Tempstr,S);
            StripTrailingWhitespace(Tempstr);
        }
    }
    else RaiseError(0, "ConnectHopHTTPSProxy", "proxy request to %s:%d failed. Server Disconnectd.", Host, Port);

    DestroyString(Tempstr);
    DestroyString(Token);
    DestroyString(Host);
    DestroyString(User);
    DestroyString(Pass);

    return(result);
}


#define HT_IP4 1
#define HT_DOMAIN 3
#define HT_IP6 4

#define SOCKS5_AUTH_NONE 0
#define SOCKS5_AUTH_PASSWD 2

int ConnectHopSocks5Auth(STREAM *S, const char *User, const char *Pass)
{
    char *Tempstr=NULL;
    const char *p_Password;
    char *ptr;
    int result, RetVal=FALSE;
    uint8_t len, passlen;

    Tempstr=SetStrLen(Tempstr, 10);

//socks5 version
    Tempstr[0]=5;
//Number of Auth Methods (just 1, username/password)
    Tempstr[1]=1;
//Auth method 2, username/password
    if (StrValid(User) || StrValid(Pass)) Tempstr[2]=SOCKS5_AUTH_PASSWD;
    else Tempstr[2]=SOCKS5_AUTH_NONE;

    STREAMWriteBytes(S,Tempstr,3);
    STREAMFlush(S);

    result=STREAMReadBytes(S,Tempstr,10);
    if ((result > 1) && (Tempstr[0]==5))
    {
        // Second Byte is authentication type selected by the server
        switch (Tempstr[1])
        {
        //no authentication required
        case 0:
            RetVal=TRUE;
            break;

        //gssapi
        case 1:
            break;

        //username/password
        case 2:
            if (Pass)
            {
                p_Password=Pass;
                passlen=strlen(Pass);

            }
            // must be careful with password len, as it won't be null terminated
            else passlen=CredsStoreLookup("", User, &p_Password);

            Tempstr=SetStrLen(Tempstr, StrLen(User) + passlen + 10);
            ptr=Tempstr;
            //version 1 of username/password authentication
            *ptr=1;
            ptr++;

            //username
            len=StrLen(User) & 0xFF;
            *ptr=len;
            ptr++;
            memcpy(ptr, User, len);
            ptr+=len;

            //password
            len=passlen & 0xFF;
            *ptr=len;
            ptr++;
            memcpy(ptr, Pass, len);
            ptr+=len;

            len=ptr-Tempstr;
            STREAMWriteBytes(S,Tempstr,len);

            //we have to flush to be sure data is sent, but this also wipes output
            //data buffer, which is useful given that we just sent a password
            STREAMFlush(S);

            //As this memory contained a password, we wipe it
            xmemset(Tempstr, 0, len);

            result=STREAMReadBytes(S,Tempstr,10);

            //two bytes reply. Byte1 is Version Byte2 is 0 for success
            if ((result > 1) && (Tempstr[0]==1) && (Tempstr[1]==0)) RetVal=TRUE;
            break;
        }
    }

    DestroyString(Tempstr);

    return(RetVal);
}



static char *ConnectHopSocks5WriteAddress(char *Dest, int AddrType, const char *Addr)
{
    char *ptr;
    int val;

    ptr=Dest;
    *ptr=AddrType;
    ptr++;
    switch (AddrType)
    {
    case HT_IP4:
        *((uint32_t *) ptr)=StrtoIP(Addr);
        ptr+=4;
        break;

    case HT_IP6:
        StrtoIP6(Addr, (struct in6_addr *) ptr);
        ptr+=16;
        break;

    default:
        val=StrLen(Addr);
        *ptr=val;
        ptr++;
        memcpy(ptr, Addr, val);
        ptr+=val;
        break;
    }

    return(ptr);
}


static int ConnectHopSocks5ReadAddress(STREAM *S)
{
    char *Tempstr=NULL, *IP6=NULL;
    int result, val;
    uint32_t IP4addr;

    Tempstr=SetStrLen(Tempstr, 1024);
    //read address type
    result=STREAMReadBytes(S,Tempstr,1);
    if (result == 1)
    {
        switch (Tempstr[0])
        {
        case HT_IP4:
            result=STREAMReadBytes(S,(char *) &IP4addr,4);
            if (IP4addr != 0) STREAMSetValue(S, "IPAddress", IPtoStr(IP4addr));
            break;

        case HT_IP6:
            IP6=SetStrLen(IP6, 32);
            result=STREAMReadBytes(S,IP6,16);
            Tempstr=IP6toStr(Tempstr, (struct in6_addr *) IP6);
            if (IP4addr != 0) STREAMSetValue(S, "IPAddress", Tempstr);
            break;

        default:
            result=STREAMReadBytes(S,Tempstr,1);
            val=Tempstr[0] & 0xFF;
            result=STREAMReadBytes(S,Tempstr,val);
            break;
        }
    }
    Destroy(Tempstr);
    Destroy(IP6);

    return(TRUE);
}



static int ConnectHopSocks4ReadReply(STREAM *S)
{
    char *Tempstr=NULL;
    int result;
    int RetVal=FALSE;
    uint32_t IP;

    Tempstr=SetStrLen(Tempstr, 1024);
    //Positive response will be 0x00 0x5a 0x00 0x00 0x00 0x00 0x00 0x00
    //although only the leading two bytes (0x00 0x5a, or \0Z) matters
    result=STREAMReadBytes(S,Tempstr,8);
    if ((result==8) && (Tempstr[0]=='\0') && (Tempstr[1]=='Z'))
    {
        RetVal=TRUE;
        IP=*(uint32_t *) (Tempstr + 4);
        if (IP != 0) STREAMSetValue(S, "IPAddress", IPtoStr(IP));
    }
    return(RetVal);
}


int ConnectHopSocks(STREAM *S, int SocksLevel, const char *ProxyURL, const char *Destination)
{
    char *Tempstr=NULL;
    char *Token=NULL, *Host=NULL, *User=NULL, *Pass=NULL;
    uint8_t *ptr;
    uint32_t IP;
    const char *tptr;
    int result, RetVal=FALSE, val;
    uint8_t HostType=HT_IP4;

    ParseConnectDetails(ProxyURL, NULL, &Host, &Token, &User, &Pass, NULL);
    if (! (S->State & SS_INITIAL_CONNECT_DONE))
    {
        val=atoi(Token);
        S->in_fd=TCPConnect(Host, val, "");
        S->out_fd=S->in_fd;
        if (S->in_fd == -1)
        {
            RaiseError(0, "ConnectHopSocks", "connection to socks proxy at %s failed", ProxyURL);
            return(FALSE);
        }


        if (SocksLevel==CONNECT_HOP_SOCKS5)
        {
            if (! ConnectHopSocks5Auth(S, User, Pass))
            {
                RaiseError(0, "ConnectHopSocks", "authentication to socks proxy at %s failed", ProxyURL);
                return(FALSE);
            }
        }
    }

//Horrid binary protocol.
    Tempstr=SetStrLen(Tempstr, StrLen(User) + 20 + StrLen(Destination));
    ptr=(uint8_t *) Tempstr;

//version
    if (SocksLevel==CONNECT_HOP_SOCKS5) *ptr=5;
    else *ptr=4; //version number
    ptr++;

//connection type
    *ptr=1; //outward connection (2 binds a port for incoming)
    ptr++;



//Sort out destination now
    tptr=Destination;
    if (strncmp(tptr,"tcp:",4)==0) tptr+=4;
    tptr=GetToken(tptr,":",&Token,0);
    if (IsIP4Address(Token)) HostType=HT_IP4;
    else if (IsIP6Address(Token)) HostType=HT_IP6;
    else HostType=HT_DOMAIN;


    if (SocksLevel==CONNECT_HOP_SOCKS5)
    {
//Socks 5 has a 'reserved' byte after the connection type
        *ptr=0;
        ptr++;
        ptr=ConnectHopSocks5WriteAddress(ptr, HostType, Token);
    }


//destination port. By a weird coincidence this is in the right place
//for either socks4 or 5, despite the fact that it comes after the
//host in socks5, and before the host in socks4
    *((uint16_t *) ptr) =htons(atoi(tptr));
    ptr+=2;

    if (SocksLevel==CONNECT_HOP_SOCKS4)
    {
        //destination host
        switch (HostType)
        {
        case HT_IP4:
            *((uint32_t *) ptr) =StrtoIP(Token);
            ptr+=4;
            val=StrLen(User)+1;
            memcpy(ptr,User,val);
            ptr+=val;
            break;

        default:
            *((uint32_t *) ptr) =StrtoIP("0.0.0.1");
            ptr+=4;
            break;
        }

        val=StrLen(User)+1;
        memcpy(ptr, User, val);
        ptr+=val;

        //+1 to include terminating \0
        val=StrLen(Token) +1;
        memcpy(ptr, Token, val);
        ptr+=val;
    }

    STREAMWriteBytes(S,Tempstr,(char *)ptr-Tempstr);
    STREAMFlush(S);


    Tempstr=SetStrLen(Tempstr, 32);
    if (SocksLevel==CONNECT_HOP_SOCKS5)
    {
        //read socks version (5) response code (0 for success) and a reserved byte
        result=STREAMReadBytes(S,Tempstr,3);
        if ((result == 3) && (Tempstr[0]==5) && (Tempstr[1]==0))
        {
            RetVal=TRUE;
            ConnectHopSocks5ReadAddress(S);
            //read port
            result=STREAMReadBytes(S,Tempstr,2);
        }
    }
    else RetVal=ConnectHopSocks4ReadReply(S);


    if (! RetVal) RaiseError(0, "ConnectHopSocks", "socks proxy at %s refused connection to %s", ProxyURL, Destination);

    DestroyString(Tempstr);
    DestroyString(Host);
    DestroyString(User);
    DestroyString(Pass);
    DestroyString(Token);

    return(RetVal);
}




int SendPublicKeyToRemote(STREAM *S, char *KeyFile, char *LocalPath)
{
    char *Tempstr=NULL, *Line=NULL;
    STREAM *LocalFile;


    Tempstr=FormatStr(Tempstr,"rm -f %s ; touch %s; chmod 0600 %s\n",KeyFile,KeyFile,KeyFile);
    STREAMWriteLine(Tempstr,S);
    LocalFile=STREAMFileOpen(LocalPath,SF_RDONLY);
    if (LocalFile)
    {
        Line=STREAMReadLine(Line,LocalFile);
        while (Line)
        {
            StripTrailingWhitespace(Line);
            Tempstr=FormatStr(Tempstr,"echo '%s' >> %s\n",Line,KeyFile);
            STREAMWriteLine(Tempstr,S);
            Line=STREAMReadLine(Line,LocalFile);
        }
        STREAMClose(LocalFile);
    }

    return(TRUE);
}



//this function launches an ssh helper process for either ssh, sshtunnel or sshproxy connection hops.
//An sshtunnel hop launches an ssh process with '-L <local port>:<remote host>:<remote port>' syntax.
//An sshproxy hop launches an ssh process with '-D 127.0.0.1:<local port>' syntax (socks proxy).
//For both these configs the local port is selected randomly by this function
static STREAM *ConnectHopSSHSpawnHelper(const char *ProxyURL, const char *Fmt, const char *Destination)
{
    int i, LocalPort, SshPort=22;
    char *RemoteHost=NULL, *RemotePort=NULL, *Tempstr=NULL;
    char *SshHost=NULL, *SshUser=NULL, *SshPassword=NULL;
    STREAM *tmpS=NULL;

    ParseConnectDetails(ProxyURL, NULL, &SshHost, &Tempstr, &SshUser, &SshPassword, NULL);
    if (StrValid(Tempstr)) SshPort=atoi(Tempstr);

    ParseConnectDetails(Destination, NULL, &RemoteHost, &RemotePort, NULL, NULL, NULL);

    for (i=0; i < 10; i++)
    {
        //pick a random port above port 9000
        LocalPort=(rand() % (0xFFFF - 9000)) +9000;
        if (strncmp(Fmt,"stdin:",6)==0) Tempstr=FormatStr(Tempstr, Fmt, RemoteHost, RemotePort);
        else Tempstr=FormatStr(Tempstr, Fmt, LocalPort, RemoteHost, RemotePort);
        tmpS=SSHConnect(SshHost, SshPort, SshUser, SshPassword, Tempstr, 0);
        if (tmpS)
        {
            Tempstr=FormatStr(Tempstr, "%d", LocalPort);
            STREAMSetValue(tmpS, "LU:SshTunnelPort", Tempstr);
            break;
        }
    }

    Destroy(RemoteHost);
    Destroy(RemotePort);
    Destroy(SshHost);
    Destroy(SshUser);
    Destroy(SshPassword);
    Destroy(Tempstr);

    return(tmpS);
}



int ConnectHopSSH(STREAM *S, int Type, const char *ProxyURL, const char *Destination)
{
    STREAM *tmpS;
    ListNode *Node;
    char *Tempstr=NULL;
    int result=FALSE, i;
    unsigned int Port=0;


    switch (Type)
    {
    case CONNECT_HOP_SSHTUNNEL:
        tmpS=ConnectHopSSHSpawnHelper(ProxyURL, "tunnel:%d:%s:%s", Destination);

        if (tmpS)
        {
            if (! S->Items) S->Items=ListCreate();
            Port=atoi(STREAMGetValue(tmpS, "LU:SshTunnelPort"));
            ListAddNamedItem(S->Items, "LU:AssociatedStream", tmpS);
            for (i=0; i < 60; i++)
            {
                S->in_fd=TCPConnect("127.0.0.1", Port, "");
                if (S->in_fd > -1)
                {
                    S->out_fd=S->in_fd;
                    result=TRUE;
                    break;
                }
                usleep(200000);
            }
        }
        break;

    case CONNECT_HOP_SSHPROXY:
        Node=ListFindNamedItem(ProxyHelpers, ProxyURL);
        if (Node) tmpS=(STREAM *) Node->Item;
        else
        {
            tmpS=ConnectHopSSHSpawnHelper(ProxyURL, "proxy:127.0.0.1:%d", Destination);
            if (tmpS)
            {
                if (! ProxyHelpers) ProxyHelpers=ListCreate();
                Tempstr=FormatStr(Tempstr, "%d", getpid());
                STREAMSetValue(tmpS, "LU:LauncherPID", Tempstr);
                ListAddNamedItem(ProxyHelpers, ProxyURL, tmpS);
                LibUsefulSetupAtExit();
            }
        }

        if (tmpS)
        {
            usleep(200000);
            Tempstr=MCopyStr(Tempstr, "127.0.0.1:", STREAMGetValue(tmpS, "LU:SshTunnelPort"), NULL);
            result=ConnectHopSocks(S, CONNECT_HOP_SOCKS5, Tempstr, Destination);
        }
        break;


    default:
        tmpS=ConnectHopSSHSpawnHelper(ProxyURL, "stdin:%s:%s", Destination);
        if (tmpS)
        {
            usleep(200000);
            result=TRUE;
            S->in_fd=tmpS->in_fd;
            S->out_fd=tmpS->out_fd;
        }

        //STREAMDestroy is like STREAMClose, except it doesn't close any file descriptors
        if (tmpS) STREAMDestroy(tmpS);
        break;
    }

    if (! result) RaiseError(0, "ConnectHopSSH", "failed to sshtunnel via %s to %s", ProxyURL, Destination);

    Destroy(Tempstr);
    return(result);
}


int STREAMProcessConnectHops(STREAM *S, const char *HopList)
{
    int val, result=FALSE;
    char *Dest=NULL, *HopURL=NULL, *NextHop=NULL, *Token=NULL;
    const char *ptr;
    int count=0;


    ptr=GetToken(HopList, "|", &HopURL,0);

//Note we check 'StrValid' not just whether ptr is null. This is because ptr will be an empty string
//for the last token, and we don't want to process th last token, which will be the 'actual' connection
    while (StrValid(ptr))
    {
        ptr=GetToken(ptr, "|", &NextHop,0);
        ParseConnectDetails(NextHop, NULL, &Dest, &Token, NULL, NULL, NULL);
        Dest=MCatStr(Dest,":",Token,NULL);

        GetToken(HopURL,":",&Token,0);
        val=MatchTokenFromList(Token,HopTypes,0);

        switch (val)
        {
        case CONNECT_HOP_TCP:
            //this type assumes that connecting to a host and port instantly puts us through to the Destination.
            //thus we do nothing with the Destination value, except maybe log it if connection fails
            //It's a no-op unless it's the first item in the connection chain, as otherwise previous connections
            //will have effectively processed this already
            if (count > 0) result=TRUE;
            else
            {
                if (STREAMConnect(S, HopURL, "")) result=TRUE;
                else RaiseError(0, "ConnectHopTCP", "failed to connect to %s for destination %s", HopURL, Dest);
            }
            break;

        case CONNECT_HOP_HTTPTUNNEL:
            result=ConnectHopHTTPSProxy(S, HopURL, Dest);
            break;

        case CONNECT_HOP_SSH:
        case CONNECT_HOP_SSHTUNNEL:
        case CONNECT_HOP_SSHPROXY:
            if (count==0) result=ConnectHopSSH(S, val, HopURL, Dest);
            else
            {
                result=FALSE;
                RaiseError(0, "ConnectHopSSH", "SSH connect hops must be first in hop chain. Connection failed to %s for destination %s", HopURL, Dest);
            }
            break;

        case CONNECT_HOP_SOCKS4:
        case CONNECT_HOP_SOCKS5:
            result=ConnectHopSocks(S, val, HopURL, Dest);
            break;

        default:
            RaiseError(0, "ConnectHop", "unknown connection proxy type %s", HopURL);
            break;
        }

        S->State=SS_INITIAL_CONNECT_DONE;
        count++;
        HopURL=CopyStr(HopURL, NextHop);
    }

    if (StrValid(HopURL))
    {
        if (! StrValid(S->Path)) S->Path=CopyStr(S->Path,HopURL);
        if (strncmp(HopURL,"ssl:",4)==0) DoSSLClientNegotiation(S,0);
    }

    DestroyString(HopURL);
    DestroyString(NextHop);
    DestroyString(Token);
    DestroyString(Dest);

    STREAMFlush(S);
    return(result);
}



