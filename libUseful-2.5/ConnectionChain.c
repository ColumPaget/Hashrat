#include "ConnectionChain.h"
#include "ParseURL.h"
#include "SpawnPrograms.h"
#include "expect.h"

const char *HopTypes[]={"none","direct","httptunnel","ssh","sshtunnel","socks4","socks5","shell","telnet",NULL};
typedef enum {CONNECT_HOP_NONE, CONNECT_HOP_TCP, CONNECT_HOP_HTTPTUNNEL, CONNECT_HOP_SSH, CONNECT_HOP_SSHTUNNEL, CONNECT_HOP_SOCKS4, CONNECT_HOP_SOCKS5, CONNECT_HOP_SHELL_CMD, CONNECT_HOP_TELNET} THopTypes;

char *GlobalConnectionChain=NULL;

int SetGlobalConnectionChain(const char *Chain)
{
char *Token=NULL, *Type=NULL, *ptr;
int result=TRUE;

ptr=GetToken(Chain, ",", &Token, 0);
while (ptr)
{
	GetToken(Token,":",&Type,0);
	if (MatchTokenFromList(Type, HopTypes, 0)==-1) result=FALSE;
	ptr=GetToken(ptr, ",", &Token, 0);
}

GlobalConnectionChain=CopyStr(GlobalConnectionChain, Chain);

DestroyString(Token);
DestroyString(Type);
return(result);
}



int DoHTTPProxyTunnel(STREAM *S, const char *Host, int Port, const char *Destination, int Flags)
{
char *Tempstr=NULL, *Token=NULL;
const char *ptr=NULL;
int result=FALSE;

	S->in_fd=TCPConnect(Host,Port,0); 
	S->out_fd=S->in_fd;
	if (S->in_fd == -1) return(FALSE);

	ptr=Destination;
	if (strncmp(ptr,"tcp:",4)==0) ptr+=4;
	Tempstr=FormatStr(Tempstr,"CONNECT %s HTTP/1.1\r\n\r\n",ptr);

	STREAMWriteLine(Tempstr,S);
	STREAMFlush(S);
	
	Tempstr=STREAMReadLine(Tempstr,S);
	StripTrailingWhitespace(Tempstr);

	ptr=GetToken(Tempstr," ",&Token,0);
	ptr=GetToken(ptr," ",&Token,0);

	if (*Token=='2') result=TRUE;
	while (StrLen(Tempstr))
	{
		Tempstr=STREAMReadLine(Tempstr,S);
		StripTrailingWhitespace(Tempstr);
	}

DestroyString(Tempstr);
DestroyString(Token);

return(result);
}


#define HT_IP4 1
#define HT_DOMAIN 3
#define HT_IP6 4


int ConnectHopSocks5Auth(STREAM *S, const char *User, const char *Pass)
{
char *Tempstr=NULL, *ptr;
int result, RetVal=FALSE;
uint8_t len;

Tempstr=SetStrLen(Tempstr, 10);

//socks5 version
Tempstr[0]=5;
//Number of Auth Methods (just 1, username/password)
Tempstr[1]=1;
//Auth method 2, username/password
Tempstr[2]=2;

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
	Tempstr=SetStrLen(Tempstr, StrLen(User) + StrLen(Pass) + 10);
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
	len=StrLen(Pass) & 0xFF;
	*ptr=len;
	ptr++;
	memcpy(ptr, Pass, len);
	ptr+=len;

	STREAMWriteBytes(S,Tempstr,ptr-Tempstr);
	STREAMFlush(S);

	result=STREAMReadBytes(S,Tempstr,10);

	//two bytes reply. Byte1 is Version Byte2 is 0 for success
	if ((result > 1) && (Tempstr[0]==1) && (Tempstr[1]==0)) RetVal=TRUE;
	break;
	}
}

DestroyString(Tempstr);

return(RetVal);
}



int ConnectHopSocks(STREAM *S, int Type, const char *Host, int Port, const char *User, const char *Pass, const char *Path)
{
char *Tempstr=NULL;
uint8_t *ptr;
uint32_t IP;
char *Token=NULL;
const char *tptr;
int result, RetVal=FALSE, val;
uint8_t HostType=HT_IP4;

S->in_fd=TCPConnect(Host,Port,0); 
S->out_fd=S->in_fd;
if (S->in_fd == -1) return(FALSE);


if (Type==CONNECT_HOP_SOCKS5)
{
if (! ConnectHopSocks5Auth(S, User, Pass)) return(FALSE);
}

//Horrid binary protocol. 
Tempstr=SetStrLen(Tempstr, StrLen(User) + 20 + StrLen(Path));
ptr=Tempstr;

//version
if (Type==CONNECT_HOP_SOCKS5) *ptr=5;
else *ptr=4; //version number
ptr++;

//connection type
*ptr=1; //outward connection (2 binds a port for incoming)
ptr++;

//Sort out path now
tptr=Path;
if (strncmp(tptr,"tcp:",4)==0) tptr+=4;
tptr=GetToken(tptr,":",&Token,0);
if (IsIP4Address(Token)) HostType=HT_IP4;
else if (IsIP6Address(Token)) HostType=HT_IP6;
else HostType=HT_DOMAIN;


if (Type==CONNECT_HOP_SOCKS5) 
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

if (Type==CONNECT_HOP_SOCKS4)
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

STREAMWriteBytes(S,Tempstr,(char *)ptr-Tempstr); STREAMFlush(S);
Tempstr=SetStrLen(Tempstr, 32);
result=STREAMReadBytes(S,Tempstr,32);


if (Type==CONNECT_HOP_SOCKS5)
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


DestroyString(Tempstr);
DestroyString(Token);

return(RetVal);
}




int SendPublicKeyToRemote(STREAM *S, char *KeyFile, char *LocalPath)
{
char *Tempstr=NULL, *Line=NULL;
STREAM *LocalFile;


Tempstr=FormatStr(Tempstr,"rm -f %s ; touch %s; chmod 0600 %s\n",KeyFile,KeyFile,KeyFile);
STREAMWriteLine(Tempstr,S);
LocalFile=STREAMOpenFile(LocalPath,SF_RDONLY);
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



int ConnectHopSSH(STREAM *S,int Type, char *Host, int Port, char *User, char *Pass, char *NextHop)
{
char *Tempstr=NULL, *KeyFile=NULL, *Token=NULL, *Token2=NULL;
STREAM *AuthS;
int result=FALSE, val;
unsigned int TunnelPort=0;

if (Type==CONNECT_HOP_SSHTUNNEL) 
{
	TunnelPort=(rand() % (0xFFFF - 9000)) +9000;
	//Host will be Token, and port Token2
	ParseConnectDetails(NextHop, NULL, &Token, &Token2, NULL, NULL, NULL);
	Tempstr=FormatStr(Tempstr,"ssh -2 -N %s@%s  -L %d:%s:%s ",User,Host,TunnelPort,Token,Token2);

}
else Tempstr=MCopyStr(Tempstr,"ssh -2 -T ",User,"@",Host, " ", NULL );

if (strncmp(Pass,"keyfile:",8)==0)
{

		if (S->in_fd != -1)
		{
			Token=FormatStr(Token,".%d-%d",getpid(),time(NULL));
			SendPublicKeyToRemote(S,Token,Pass+8);
			KeyFile=CopyStr(KeyFile,Token);
		}
		Tempstr=MCatStr(Tempstr,"-i ",KeyFile," ",NULL);
		}

		if (Port > 0)
		{
		Token=FormatStr(Token," -p %d ",Port);
		Tempstr=CatStr(Tempstr,Token);
		}

		if (Type==CONNECT_HOP_SSHTUNNEL) 
		{
			Tempstr=CatStr(Tempstr, " 2> /dev/null");
			AuthS=STREAMSpawnCommand(Tempstr,"", "", COMMS_BY_PTY);
			STREAMSetValue(S,"HelperPID:SSH",STREAMGetValue(AuthS,"PeerPID"));
		}
		else if (S->in_fd==-1) 
		{
			Tempstr=CatStr(Tempstr, " 2> /dev/null");
			PseudoTTYSpawn(&S->in_fd,Tempstr,"","",0);
			S->out_fd=S->in_fd;
			if (S->in_fd > -1)
			{
				result=TRUE;
				STREAMSetFlushType(S,FLUSH_LINE,0,0);
			}
			AuthS=S;
		}
		else 
		{
			if (StrLen(KeyFile)) Tempstr=MCatStr(Tempstr," ; rm -f ",KeyFile,NULL);
			Tempstr=CatStr(Tempstr,"; exit\n");
			STREAMWriteLine(Tempstr,S);
			result=TRUE;
			AuthS=S;
		}

		if ((StrLen(KeyFile)==0) && (StrLen(Pass) > 0)) 
		{
			Token=MCopyStr(Token,Pass,"\n",NULL);
			for (val=0; val < 3; val++)
			{
			if (STREAMExpectAndReply(AuthS,"assword:",Token)) break;
			}
		}
		STREAMSetTimeout(AuthS,100);
		//STREAMExpectSilence(AuthS);
		sleep(3);

		if (Type==CONNECT_HOP_SSHTUNNEL) 
		{
			S->in_fd=TCPConnect("127.0.0.1",TunnelPort,0);
			S->out_fd=S->in_fd;
			if (S->in_fd > -1) result=TRUE;
		}


DestroyString(Tempstr);
DestroyString(KeyFile);
DestroyString(Token2);
DestroyString(Token);

return(result);
}


int STREAMProcessConnectHop(STREAM *S, char *HopURL, int LastHop)
{
int val, result=FALSE;
char *Token=NULL, *Token2=NULL;
char *Tempstr=NULL;
char *User=NULL, *Host=NULL,*Pass=NULL, *KeyFile=NULL;
int Port=0;

ParseConnectDetails(HopURL, &Token, &Host, &Token2, &User, &Pass, NULL);

Port=atoi(Token2);

val=MatchTokenFromList(Token,HopTypes,0);
switch (val)
{
	case CONNECT_HOP_TCP:
		if (S->in_fd==-1)
		{
			S->in_fd=TCPConnect(Host,Port,0); 
			S->out_fd=S->in_fd;
			if (S->in_fd > -1) result=TRUE;
		}
		break;

	case CONNECT_HOP_HTTPTUNNEL:
		result=DoHTTPProxyTunnel(S, Host, Port, S->Path, 0);
	break;	

	case CONNECT_HOP_SSH:
	case CONNECT_HOP_SSHTUNNEL:
		result=ConnectHopSSH(S, val, Host, Port, User, Pass, S->Path);
	break;

	case CONNECT_HOP_SOCKS4:
	case CONNECT_HOP_SOCKS5:
		result=ConnectHopSocks(S, val, Host, Port, User, Pass, S->Path);
	break;

	case CONNECT_HOP_SHELL_CMD:
	break;

	case CONNECT_HOP_TELNET:
		if (Port > 0)
		{
		Tempstr=FormatStr(Tempstr,"telnet -8 %s %d ",Host, Port);
		}
		else Tempstr=FormatStr(Tempstr,"telnet -8 %s ",Host);

		if (S->in_fd==-1) 
		{
			PseudoTTYSpawn(& S->in_fd,Tempstr,"","",0);
		        S->out_fd=S->in_fd;
			if (S->in_fd > -1)
			{
				result=TRUE;
				STREAMSetFlushType(S,FLUSH_LINE,0,0);
			}

		}
		else 
		{
			Tempstr=CatStr(Tempstr,";exit\n");
			STREAMWriteLine(Tempstr,S);
			result=TRUE;
		}
		if (StrLen(User) > 0) 
		{
			Tempstr=MCopyStr(Tempstr,User,"\n",NULL);
			STREAMExpectAndReply(S,"ogin:",Tempstr);
		}
		if (StrLen(Pass) > 0) 
		{
			Tempstr=MCopyStr(Tempstr,Pass,"\n",NULL);
			STREAMExpectAndReply(S,"assword:",Tempstr);
		}
		STREAMExpectSilence(S,2);
		break;


}

DestroyString(Tempstr);
DestroyString(Token);
DestroyString(KeyFile);
DestroyString(Host);
DestroyString(User);
DestroyString(Pass);

STREAMFlush(S);
return(result);
}


/*
int STREAMInternalLastHop(STREAM *S,char *DesiredHost,int DesiredPort, char *LastHop)
{
int result, Type,Port;
char *Host=NULL, *User=NULL, *Pass=NULL, *KeyFile=NULL;

ParseConnectHop(LastHop, &Type,  &Host, &User, &Pass, &KeyFile, &Port);
switch (Type)
{

}
result=STREAMProcessConnectHop(S, Tempstr, TRUE);

DestroyString(Tempstr);
DestroyString(Host);
DestroyString(User);
DestroyString(Pass);
DestroyString(KeyFile);
return(result);
}
*/



int STREAMAddConnectionHop(STREAM *S, char *HopsString)
{
char *Tempstr=NULL, *Value=NULL, *ptr;

if (! S->Values) S->Values=ListCreate();

ptr=GetToken(HopsString, ",", &Value,0);
while (ptr)
{
StripTrailingWhitespace(Value);
StripLeadingWhitespace(Value);

Tempstr=FormatStr(Tempstr,"ConnectHop:%d",ListSize(S->Values));
STREAMSetValue(S,Tempstr,Value);
ptr=GetToken(ptr, ",", &Value,0);
}

DestroyString(Tempstr);
DestroyString(Value);
return(TRUE);
}

