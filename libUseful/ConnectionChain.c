#include "ConnectionChain.h"
#include "URL.h"
#include "SpawnPrograms.h"
#include "Expect.h"
#include "SecureMem.h"
#include "Errors.h"
#include "Stream.h"
#include "Ssh.h"

typedef enum {CONNECT_HOP_NONE, CONNECT_HOP_TCP, CONNECT_HOP_HTTPTUNNEL, CONNECT_HOP_SSH, CONNECT_HOP_SSHTUNNEL, CONNECT_HOP_SOCKS4, CONNECT_HOP_SOCKS5, CONNECT_HOP_SHELL_CMD, CONNECT_HOP_TELNET} THopTypes;
static const char *HopTypes[]= {"none","tcp","https","ssh","sshtunnel","socks4","socks5","shell","telnet",NULL};

char *GlobalConnectionChain=NULL;

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



int ConnectHopHTTPSProxy(STREAM *S, const char *Proxy, const char *Destination)
{
    char *Tempstr=NULL, *Token=NULL;
    char *Proto=NULL, *Host=NULL, *User=NULL, *Pass=NULL;
    const char *ptr=NULL;
    int result=FALSE, Port;

    ParseConnectDetails(Proxy, &Token, &Host, &Token, &User, &Pass, NULL);
    Port=atoi(Token);
    if (! (S->State & SS_INITIAL_CONNECT_DONE))
    {
        if (Port==0) Port=443;
        S->in_fd=TCPConnect(Host,Port,0);
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

        while (StrLen(Tempstr))
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
        S->in_fd=TCPConnect(Host, val, 0);
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

        *ptr=HostType;
        ptr++;
        switch (HostType)
        {
        case HT_IP4:
            *((uint32_t *) ptr) =StrtoIP(Token);
            ptr+=4;
            break;

        case HT_IP6:
            break;

        default:
            val=StrLen(Token);
            *ptr=val;
            ptr++;
            memcpy(ptr, Token, val);
            ptr+=val;
            break;
        }
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
    result=STREAMReadBytes(S,Tempstr,32);


    if (SocksLevel==CONNECT_HOP_SOCKS5)
    {
        if ((result > 8) && (Tempstr[0]==5) && (Tempstr[1]==0))
        {
            RetVal=TRUE;
        }
    }
    else
    {
        //Positive response will be 0x00 0x5a 0x00 0x00 0x00 0x00 0x00 0x00
        //although only the leading two bytes (0x00 0x5a, or \0Z) matters
        if ((result==8) && (Tempstr[0]=='\0') && (Tempstr[1]=='Z'))
        {
            RetVal=TRUE;

            IP=*(uint32_t *) (Tempstr + 4);
            if (IP != 0) STREAMSetValue(S, "IPAddress", IPtoStr(IP));
        }
    }


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



int ConnectHopSSH(STREAM *S, int Type, const char *ProxyURL, const char *Destination)
{
    char *Tempstr=NULL, *Token=NULL, *Token2=NULL;
    char *Host=NULL, *User=NULL, *Pass=NULL;
    STREAM *tmpS;
    int result=FALSE, val, i;
    unsigned int Port=0, DPort=0;


    ParseConnectDetails(ProxyURL, NULL, &Host, &Token, &User, &Pass, NULL);
    if (StrValid(Token)) Port=atoi(Token);

    if (Type==CONNECT_HOP_SSHTUNNEL)
    {
//get hostname and port of next hop
        DPort=(rand() % (0xFFFF - 9000)) +9000;
        //Host will be Token, and port Token2
        ParseConnectDetails(Destination, NULL, &Token, &Token2, NULL, NULL, NULL);
        Tempstr=FormatStr(Tempstr,"tunnel:%d:%s:%s ",DPort,Token,Token2);
        tmpS=SSHConnect(Host, Port, User, Pass, Tempstr, 0);
        if (tmpS)
        {
            if (! S->Items) S->Items=ListCreate();
            ListAddNamedItem(S->Items, "LU:AssociatedStream", tmpS);
            for (i=0; i < 60; i++)
            {
                S->in_fd=TCPConnect("127.0.0.1",DPort,0);
                if (S->in_fd > -1)
                {
                    S->out_fd=S->in_fd;
                    result=TRUE;
                    break;
                }
                usleep(200000);
            }
        }
    }
    else
    {
        ParseConnectDetails(Destination, NULL, &Token, &Token2, NULL, NULL, NULL);
        DPort=atoi(Token2);
        Tempstr=FormatStr(Tempstr,"stdin:%s:%d", Token, DPort);
        tmpS=SSHConnect(Host, Port, User, Pass, Tempstr, 0);
        if (tmpS)
        {
            usleep(200000);
            result=TRUE;
            S->in_fd=tmpS->in_fd;
            S->out_fd=tmpS->out_fd;
        }

				//STREAMDestroy is like STREAMClose, except it doesn't close any file descriptors
        if (tmpS) STREAMDestroy(tmpS);
    }

    if (! result) RaiseError(0, "ConnectHopSSH", "failed to sshtunnel via %s to %s", ProxyURL, Destination);

    DestroyString(Tempstr);
    DestroyString(Token2);
    DestroyString(Token);
    DestroyString(Host);
    DestroyString(User);
    DestroyString(Pass);

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
                if (STREAMDirectConnect(S, HopURL, 0)) result=TRUE;
                else RaiseError(0, "ConnectHopTCP", "failed to connect to %s for destination %s", HopURL, Dest);
            }
            break;

        case CONNECT_HOP_HTTPTUNNEL:
            result=ConnectHopHTTPSProxy(S, HopURL, Dest);
            break;

        case CONNECT_HOP_SSH:
        case CONNECT_HOP_SSHTUNNEL:
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
        if (! StrLen(S->Path)) S->Path=CopyStr(S->Path,HopURL);
        if (strncmp(HopURL,"ssl:",4)==0) DoSSLClientNegotiation(S,0);
    }

    DestroyString(HopURL);
    DestroyString(NextHop);
    DestroyString(Token);
    DestroyString(Dest);

    STREAMFlush(S);
    return(result);
}



