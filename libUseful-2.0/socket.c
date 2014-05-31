#include "socket.h"
#include "ConnectionChain.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>



int IsIP4Address(char *Str)
{
char *ptr;
int dot_count=0;
int AllowDot=FALSE;

if (! Str) return(FALSE);

for (ptr=Str; *ptr != '\0'; ptr++)
{
	if (*ptr == '.') 
	{
		if (! AllowDot) return(FALSE);
		dot_count++;
		AllowDot=FALSE;
	}
	else 
	{
		if (! isdigit(*ptr)) return(FALSE);
		AllowDot=TRUE;
	}
}

if (dot_count != 3) return(FALSE);

return(TRUE);
}


int IsIP6Address(char *Str)
{
char *ptr;
const char *IP6CHARS="0123456789abcdefABCDEF:%";

if (!Str) return(FALSE);

for (ptr=Str; *ptr != '\0'; ptr++)
{
 if (*ptr=='%') break;
 if (! strchr(IP6CHARS,*ptr)) return(FALSE);
}

return(TRUE);
}


int IsSockConnected(int sock)
{
struct sockaddr_in sa;
int salen, result;

if (sock==-1) return(FALSE);
salen=sizeof(sa);
result=getpeername(sock,(struct sockaddr *) &sa, &salen);
if (result==0) return(TRUE);
if (errno==ENOTCONN) return(SOCK_CONNECTING);
return(FALSE);
}


const char *GetInterfaceIP(const char *Interface)
{
 int fd, result;
 struct ifreq ifr;

 fd = socket(AF_INET, SOCK_DGRAM, 0);
 if (fd==-1) return("");

 ifr.ifr_addr.sa_family = AF_INET;
 strncpy(ifr.ifr_name, Interface, IFNAMSIZ-1);
 result=ioctl(fd, SIOCGIFADDR, &ifr);
 if (result==-1) return("");
 close(fd);

 return(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}


#ifdef USE_INET6
int InitServerSock(char *Address, int Port)
{
int sock;
struct sockaddr_storage sa;
struct sockaddr_in *sa4;
struct sockaddr_in6 *sa6;
int salen;
int result;
char *p_Addr=NULL, *ptr;


if (StrLen(Address))  
{
	if (IsIP4Address(Address)) 
	{
		sa.ss_family=AF_INET;
		p_Addr=Address;
	}
	else if (IsIP6Address(Address)) 
	{
		sa.ss_family=AF_INET6;
		p_Addr=Address;
	}
	else 
	{
		sa.ss_family=AF_INET;
		p_Addr=GetInterfaceIP(Address);
	}
}
else sa.ss_family=AF_INET;

if (sa.ss_family==AF_INET)
{
	sa4=(struct sockaddr_in *) &sa;
	if (StrLen(p_Addr)) sa4->sin_addr.s_addr=StrtoIP(p_Addr);
	else sa4->sin_addr.s_addr=INADDR_ANY;
	sa4->sin_port=htons(Port);
	sa4->sin_family=AF_INET;
	salen=sizeof(struct sockaddr_in);
}
else
{
	sa6=(struct sockaddr_in6 *) &sa;
	sa6->sin6_port=htons(Port);
	sa6->sin6_family=AF_INET6;
	if (StrLen(p_Addr)) 
	{
		ptr=strchr(p_Addr,'%');
		if (ptr)
		{
			sa6->sin6_scope_id=if_nametoindex(ptr+1);
			*ptr='\0';	
		}
		inet_pton(AF_INET6, p_Addr, &(sa6->sin6_addr));
	}
	else sa6->sin6_addr=in6addr_any;
	salen=sizeof(struct sockaddr_in6);
}

sock=socket(sa.ss_family,SOCK_STREAM,0);
if (sock <0) return(-1);

//No reason to pass server/listen sockets across an exec
fcntl(sock, F_SETFD, FD_CLOEXEC);

result=1;
setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&result,sizeof(result));

result=bind(sock,(struct sockaddr *) &sa, salen);

if (result==0)
{
 result=listen(sock,10);
}

if (result==0) return(sock);
else 
{
close(sock);
return(-1);
}
}

#else 

int InitServerSock(char *Address, int Port)
{
struct sockaddr_in sa;
int result;
char *ptr;
int salen;
int sock;

sa.sin_port=htons(Port);
sa.sin_family=AF_INET;


if (StrLen(Address) > 0) 
{
	if (IsIP6Address(Address)) return(-1);

	if (IsIP4Address(Address)) ptr=Address;
	else ptr=GetInterfaceIP(Address);
	sa.sin_addr.s_addr=StrtoIP(ptr);
}
else sa.sin_addr.s_addr=INADDR_ANY;

sock=socket(AF_INET,SOCK_STREAM,0);
if (sock <0) return(-1);

//No reason to pass server/listen sockets across an exec
fcntl(sock, F_SETFD, FD_CLOEXEC);

result=1;
salen=sizeof(result);
setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&result,salen);


salen=sizeof(struct sockaddr_in);
result=bind(sock,(struct sockaddr *) &sa, salen);

if (result==0)
{
 result=listen(sock,10);
}

if (result==0) return(sock);
else 
{
close(sock);
return(-1);
}
}
#endif


int InitUnixServerSock(char *Path)
{
int sock;
struct sockaddr_un sa;
int salen;
int result;

sock=socket(AF_UNIX,SOCK_STREAM,0);
if (sock <0) return(-1);

//No reason to pass server/listen sockets across an exec
fcntl(sock, F_SETFD, FD_CLOEXEC);

result=1;
salen=sizeof(result);
strcpy(sa.sun_path,Path);
sa.sun_family=AF_UNIX;
salen=sizeof(struct sockaddr_un);
result=bind(sock,(struct sockaddr *) &sa, salen);

if (result==0)
{
 result=listen(sock,10);
}

if (result==0) return(sock);
else 
{
close(sock);
return(-1);
}
}


int TCPServerSockAccept(int ServerSock, char **Addr)
{
struct sockaddr_storage sa;
int salen;
int sock;

salen=sizeof(sa);
sock=accept(ServerSock,(struct sockaddr *) &sa,&salen);
if (Addr) 
{
*Addr=SetStrLen(*Addr,NI_MAXHOST);
getnameinfo((struct sockaddr *) &sa, salen, *Addr, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
}
return(sock);
}


int UnixServerSockAccept(int ServerSock)
{
struct sockaddr_un sa;
int salen;
int sock;

salen=sizeof(sa);
sock=accept(ServerSock,(struct sockaddr *) &sa,&salen);
return(sock);
}







#ifdef USE_INET6
int GetSockDetails(int sock, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort)
{
int salen, result;
struct sockaddr_storage sa;
char *Tempstr=NULL;

*LocalPort=0;
*RemotePort=0;
*LocalAddress=CopyStr(*LocalAddress,"");
*RemoteAddress=CopyStr(*RemoteAddress,"");

salen=sizeof(struct sockaddr_storage);
result=getsockname(sock, (struct sockaddr *) &sa, &salen);

if (result==0)
{
*LocalAddress=SetStrLen(*LocalAddress,NI_MAXHOST);
Tempstr=SetStrLen(Tempstr,NI_MAXSERV);
getnameinfo((struct sockaddr *) &sa, salen, *LocalAddress, NI_MAXHOST, Tempstr, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);
	*LocalPort=atoi(Tempstr);

	//Set Address to be the same as control sock, as it might not be INADDR_ANY
	result=getpeername(sock, (struct sockaddr *) &sa, &salen);

	if (result==0)
	{
		*RemoteAddress=SetStrLen(*RemoteAddress,NI_MAXHOST);
		Tempstr=SetStrLen(Tempstr,NI_MAXSERV);
		getnameinfo((struct sockaddr *) &sa, salen, *RemoteAddress, NI_MAXHOST, Tempstr, NI_MAXSERV, NI_NUMERICHOST|NI_NUMERICSERV);
		*RemotePort=atoi(Tempstr);

	}

	//We've got the local sock, so lets still call it a success
	result=0;
}

DestroyString(Tempstr);

if (result==0) return(TRUE);
return(FALSE);
}

#else

int GetSockDetails(int sock, char **LocalAddress, int *LocalPort, char **RemoteAddress, int *RemotePort)
{
int salen, result;
struct sockaddr_in sa;

*LocalPort=0;
*RemotePort=0;
*LocalAddress=CopyStr(*LocalAddress,"");
*RemoteAddress=CopyStr(*RemoteAddress,"");

salen=sizeof(struct sockaddr_in);
result=getsockname(sock, (struct sockaddr *) &sa, &salen);

if (result==0)
{
	*LocalAddress=CopyStr(*LocalAddress,IPtoStr(sa.sin_addr.s_addr));
	*LocalPort=ntohs(sa.sin_port);

	//Set Address to be the same as control sock, as it might not be INADDR_ANY
	result=getpeername(sock, (struct sockaddr *) &sa, &salen);

	if (result==0)
	{
		*RemoteAddress=CopyStr(*RemoteAddress,IPtoStr(sa.sin_addr.s_addr));
		*RemotePort=sa.sin_port;
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
int ReconnectSock(int sock, char *Host, int Port, int Flags)
{
int salen, result;
struct sockaddr_in sa;
struct hostent *hostdata;

sa.sin_family=AF_INET;
sa.sin_port=htons(Port);

if (IsIP4Address(Host))
{
inet_aton(Host, (struct in_addr *) &sa.sin_addr);
}
else 
{ 
   hostdata=gethostbyname(Host);
   if (!hostdata) 
   {
     return(-1);
   }
sa.sin_addr=*(struct in_addr *) *hostdata->h_addr_list;
}

salen=sizeof(sa);
if (Flags & CONNECT_NONBLOCK) 
{
fcntl(sock,F_SETFL,O_NONBLOCK);
}

result=connect(sock,(struct sockaddr *)&sa, salen);
if (result==0) result=TRUE;

if ((result==-1) && (Flags & CONNECT_NONBLOCK) && (errno == EINPROGRESS)) result=FALSE;
return(result);
}


int ConnectToHost(char *Host, int Port,int Flags)
{
int sock, result;

sock=socket(AF_INET,SOCK_STREAM,0);
if (sock <0) return(-1);
result=ReconnectSock(sock,Host,Port,Flags);
if (result==-1)
{
close(sock);
return(-1);
}

return(sock);

}




int CheckForTerm(DownloadContext *CTX, char inchar)
{

    if (inchar == CTX->TermStr[CTX->TermPos]) 
    {
        CTX->TermPos++;
        if (CTX->TermPos >=strlen(CTX->TermStr)) 
        {
           CTX->TermPos=0;
           return(TRUE);
        }
    }
    else
    {
      if (CTX->TermPos >0) 
      {
         STREAMWriteBytes(CTX->Output, CTX->TermStr,CTX->TermPos);
         CTX->TermPos=0;
         return(CheckForTerm(CTX,inchar));
      }
    }
return(FALSE); 
}



int  ProcessIncommingBytes(DownloadContext *CTX)
{
int inchar, FoundTerm=FALSE, err;

 inchar=STREAMReadChar(CTX->Input);

 if (inchar==EOF) return(TRUE);

 FoundTerm=CheckForTerm(CTX,(char) inchar);
 while ((inchar !=EOF) && (! FoundTerm))
 {
     if (CTX->TermPos==0) STREAMWriteChar(CTX->Output, (char) inchar);
     inchar=STREAMReadChar(CTX->Input);
     err=errno;
     FoundTerm=CheckForTerm(CTX, (char) inchar);
 }
if (inchar==EOF) return(TRUE);
if (FoundTerm) return(TRUE);
return(FALSE);
}


int DownloadToTermStr2(STREAM *Connection, STREAM *SaveFile, char *TermStr)
{
DownloadContext CTX;

CTX.TermStr=CopyStr(NULL,TermStr);
CTX.Input=Connection;
CTX.Output=SaveFile;
CTX.TermPos=0;

while(ProcessIncommingBytes(&CTX) !=TRUE);

DestroyString(CTX.TermStr);
return(TRUE);
}

int DownloadToDot(STREAM *Connection, STREAM *SaveFile)
{
DownloadToTermStr2(Connection,SaveFile,"\r\n.\r\n");
}


int DownloadToTermStr(STREAM *Connection, STREAM *SaveFile, char *TermStr)
{
char *Tempstr=NULL;

Tempstr=STREAMReadLine(Tempstr,Connection);
while (Tempstr)
{
  if (strcmp(Tempstr,TermStr)==0)
{
 break;
}
  STREAMWriteLine(Tempstr,SaveFile);
  Tempstr=STREAMReadLine(Tempstr,Connection);
}
return(TRUE);
}

char *LookupHostIP(char *Host)
{
struct hostent *hostdata;

   hostdata=gethostbyname(Host);
   if (!hostdata) 
   {
     return(NULL);
   }

//inet_ntoa shouldn't need this cast to 'char *', but it emitts a warning
//without it
return((char *) inet_ntoa(*(struct in_addr *) *hostdata->h_addr_list));
}


char *GetRemoteIP(int sock)
{
struct sockaddr_in sa;
int salen, result;

salen=sizeof(struct sockaddr_in);
result=getpeername(sock,(struct sockaddr *) &sa, &salen);
if  (result==-1)  
{
if (errno==ENOTSOCK)  return("127.0.0.1");
else return("0.0.0.0");
}

return((char *) inet_ntoa(sa.sin_addr));
}


char *IPStrToHostName(char *IPAddr)
{
struct sockaddr_in sa;
struct hostent *hostdata=NULL;

inet_aton(IPAddr,& sa.sin_addr);
hostdata=gethostbyaddr(&sa.sin_addr.s_addr,sizeof((sa.sin_addr.s_addr)),AF_INET);
if (hostdata) return(hostdata->h_name);
else return("");
}




char *IPtoStr(unsigned long Address)
{
struct sockaddr_in sa;
sa.sin_addr.s_addr=Address;
return((char *) inet_ntoa(sa.sin_addr));

}

unsigned long StrtoIP(char *Str)
{
struct sockaddr_in sa;
if (inet_aton(Str,&sa.sin_addr)) return(sa.sin_addr.s_addr);
return(0);
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
char *ptr;
struct timeval tv;

if (! S) return(FALSE);
if ((S->in_fd > -1) && (S->Timeout > 0) )
{
  tv.tv_sec=S->Timeout;
  tv.tv_usec=0;
  if (FDSelect(S->in_fd, SELECT_WRITE, &tv) != SELECT_WRITE)
  {
    close(S->in_fd);
    S->in_fd=-1;
    S->out_fd=-1;
  }
  else if (! (Flags & CONNECT_NONBLOCK))  STREAMSetNonBlock(S, FALSE);
}

if (S->in_fd > -1)
{
S->Type=STREAM_TYPE_TCP;
result=TRUE;
STREAMSetFlushType(S,FLUSH_LINE,0);
//if (Flags & CONNECT_SOCKS_PROXY) result=DoSocksProxyTunnel(S);
if (Flags & CONNECT_SSL) DoSSLClientNegotiation(S, Flags);

ptr=GetRemoteIP(S->in_fd);
if (ptr) STREAMSetValue(S,"PeerIP",ptr);
}

return(result);
}



int STREAMConnectToHost(STREAM *S, char *DesiredHost, int DesiredPort,int Flags)
{
ListNode *Curr;
char *Token=NULL, *ptr;
int result=FALSE;
int HopNo=0, val=0;
ListNode *LastHop=NULL;

S->Path=FormatStr(S->Path,"tcp:%s:%d",DesiredHost,DesiredPort);
//Find the last hop, used to decide what ssh command to use
Curr=ListGetNext(S->Values);
while (Curr)
{
ptr=GetToken(Curr->Tag,":",&Token,0);
if (strcasecmp(Token,"ConnectHop")==0) LastHop=Curr;
Curr=ListGetNext(Curr);
}

STREAMSetFlushType(S,FLUSH_LINE,0);
Curr=ListGetNext(S->Values);
while (Curr)
{
ptr=GetToken(Curr->Tag,":",&Token,0);

if (strcasecmp(Token,"ConnectHop")==0) result=STREAMProcessConnectHop(S, (char *) Curr->Item,Curr==LastHop);

HopNo++;
if (! result) break;
Curr=ListGetNext(Curr);
}

//If we're not handling the connection through 'Connect hops' then
//just connect to host
if ((HopNo==0) && StrLen(DesiredHost))
{
	if (Flags & CONNECT_NONBLOCK) S->Flags |= SF_NONBLOCK;
	val=Flags;

	//STREAMDoPostConnect handles this	
	if (S->Timeout > 0) val |= CONNECT_NONBLOCK;

	S->in_fd=ConnectToHost(DesiredHost,DesiredPort,val);

	S->out_fd=S->in_fd;
	if (S->in_fd > -1) result=TRUE;
}

if (result==TRUE)
{
	if (Flags & CONNECT_NONBLOCK) 
	{
		S->State |=SS_CONNECTING;
		S->Flags |=SF_NONBLOCK;
	}
	else
	{
		S->State |=SS_CONNECTED;
		result=STREAMDoPostConnect(S, Flags);
	}
}


return(result);
}





int OpenUDPSock(int Port)
{
	int result;
	struct sockaddr_in addr;
	int fd;

	addr.sin_family=AF_INET;
//	addr.sin_addr.s_addr=Interface;
	addr.sin_addr.s_addr=INADDR_ANY;
	addr.sin_port=htons(Port);

	fd=socket(AF_INET, SOCK_DGRAM,0);
	result=bind(fd,(struct sockaddr *) &addr, sizeof(addr));
        if (result !=0)
        {
		close(fd);
		return(-1);
        }
   return(fd);
}


int STREAMSendDgram(STREAM *S, char *Host, int Port, char *Bytes, int len)
{
struct sockaddr_in sa;
int salen;
struct hostent *hostdata;

sa.sin_port=htons(Port);
sa.sin_family=AF_INET;
inet_aton(Host,& sa.sin_addr);
salen=sizeof(sa);

if (IsIP4Address(Host))
{
   inet_aton(Host, (struct in_addr *) &sa.sin_addr);
}
else 
{ 
   hostdata=gethostbyname(Host);
   if (!hostdata) 
   {
     return(-1);
   }
sa.sin_addr=*(struct in_addr *) *hostdata->h_addr_list;
}


return(sendto(S->out_fd,Bytes,len,0,(struct sockaddr *) &sa,salen));
}



STREAM *STREAMOpenUDP(int Port,int NonBlock)
{
int fd;
STREAM *Stream;

fd=OpenUDPSock(Port);
if (fd <0) return(NULL);
Stream=STREAMFromFD(fd);
Stream->Path=FormatStr(Stream->Path,"udp::%d",Port);
Stream->Type=STREAM_TYPE_UDP;
return(Stream);
}



/* This is a simple function to decide if a string is an IP address as   */
/* opposed to a host/domain name.                                        */

int IsIPAddress(char *Str)
{
int len,count;
len=strlen(Str);
if (len <1) return(FALSE);
for (count=0; count < len; count++)
   if ((! isdigit(Str[count])) && (Str[count] !='.')) return(FALSE);
 return(TRUE);
}


