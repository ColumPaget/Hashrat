#include "ConnectionChain.h"
#include "ParseURL.h"
#include "SpawnPrograms.h"

char *HopTypes[]={"none","direct","http_proxy","ssh","sshtunnel","shell","telnet",NULL};
typedef enum {CONNECT_HOP_NONE, CONNECT_HOP_TCP, CONNECT_HOP_HTTP_PROXY, CONNECT_HOP_SSH, CONNECT_HOP_SSHTUNNEL, CONNECT_HOP_SHELL_CMD, CONNECT_HOP_TELNET} THopTypes;


int DoHTTPProxyTunnel(STREAM *S, char *Host, int Port, int Flags)
{
char *Tempstr=NULL, *Token=NULL, *ptr=NULL;
int result=FALSE;

	if (Flags & CONNECT_SSL) Tempstr=FormatStr(Tempstr,"CONNECT https://%s:%d HTTP/1.1\r\n\r\n",Host,Port);
	else Tempstr=FormatStr(Tempstr,"CONNECT http://%s:%d HTTP/1.1\r\n\r\n",Host,Port);
	STREAMWriteLine(Tempstr,S);
	STREAMFlush(S);
	
	Tempstr=STREAMReadLine(Tempstr,S);
	StripTrailingWhitespace(Tempstr);

	ptr=GetToken(Tempstr," ",&Token,0);
	ptr=GetToken(ptr," ",&Token,0);

	if (*Token==2) result=TRUE;
	while (StrLen(Tempstr))
	{
		Tempstr=STREAMReadLine(Tempstr,S);
		StripTrailingWhitespace(Tempstr);
	}

DestroyString(Tempstr);
DestroyString(Token);

return(result);
}


int SendPublicKeyToRemote(STREAM *S, char *KeyFile, char *LocalPath)
{
char *Tempstr=NULL, *RetStr=NULL, *Line=NULL;
STREAM *LocalFile;


Tempstr=FormatStr(Tempstr,"rm -f %s ; touch %s; chmod 0600 %s\n",KeyFile,KeyFile,KeyFile);
STREAMWriteLine(Tempstr,S);
LocalFile=STREAMOpenFile(LocalPath,O_RDONLY);
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
char *Tempstr=NULL, *KeyFile=NULL, *Token=NULL, *Token2=NULL, *ptr;
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
			AuthS=STREAMSpawnCommand(Tempstr,COMMS_BY_PTY);
			STREAMSetValue(S,"HelperPID:SSH",STREAMGetValue(AuthS,"PeerPID"));
		}
		else if (S->in_fd==-1) 
		{
			Tempstr=CatStr(Tempstr, " 2> /dev/null");
			PseudoTTYSpawn(&S->in_fd,Tempstr,0);
			S->out_fd=S->in_fd;
			if (S->in_fd > -1)
			{
				result=TRUE;
				STREAMSetFlushType(S,FLUSH_LINE,0);
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
		STREAMSetTimeout(AuthS,1);
		//STREAMExpectSilence(AuthS);
		sleep(3);

		if (Type==CONNECT_HOP_SSHTUNNEL) 
		{
			S->in_fd=ConnectToHost("127.0.0.1",TunnelPort,0);
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
char *Token=NULL, *Token2=NULL, *ptr;
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
			S->in_fd=ConnectToHost(Host,Port,0); 
			S->out_fd=S->in_fd;
			if (S->in_fd > -1) result=TRUE;
		}
		break;

	case CONNECT_HOP_HTTP_PROXY:
		result=DoHTTPProxyTunnel(S, Host, Port, 0);
		break;	

	case CONNECT_HOP_SSH:
	case CONNECT_HOP_SSHTUNNEL:
		result=ConnectHopSSH(S, val, Host, Port, User, Pass, S->Path);
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
			PseudoTTYSpawn(& S->in_fd,Tempstr,0);
		        S->out_fd=S->in_fd;
			if (S->in_fd > -1)
			{
				result=TRUE;
				STREAMSetFlushType(S,FLUSH_LINE,0);
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
		STREAMSetTimeout(S,2);
		STREAMExpectSilence(S);
		break;


}

DestroyString(Tempstr);
DestroyString(Token);
DestroyString(KeyFile);
DestroyString(Host);
DestroyString(User);
DestroyString(Pass);

STREAMSetTimeout(S,30);
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



int STREAMAddConnectionHop(STREAM *S, char *Value)
{
char *Tempstr=NULL;
char *ConnectType=NULL;
int i;

StripTrailingWhitespace(Value);
StripLeadingWhitespace(Value);

if (! S->Values) S->Values=ListCreate();
Tempstr=FormatStr(Tempstr,"ConnectHop:%d",ListSize(S->Values));
STREAMSetValue(S,Tempstr,Value);

DestroyString(Tempstr);
return(TRUE);
}

void STREAMAddConnectionHopList(STREAM *S, char *HopList)
{
char *Hop=NULL, *ptr;

ptr=GetToken(HopList,",",&Hop,0);
while (ptr)
{
STREAMAddConnectionHop(S,Hop);
ptr=GetToken(ptr,",",&Hop,0);
}

DestroyString(Hop);
}
