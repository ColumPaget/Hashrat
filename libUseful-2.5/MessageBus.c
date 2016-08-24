#include "MessageBus.h"
#include "defines.h"
#include "SpawnPrograms.h"
#include "http.h"

#define MESSAGEAGENT_BUSY 1

typedef struct
{
char *URL;
int MaxConnections;
int Timeout;
ListNode *Connections;
MSG_FUNC AgentFunction;
} TMessageBus;

ListNode *MessageBusConnections=NULL;
STREAM *MessageS=NULL;

// typedef void (*MSG_FUNC)(const char *PeerName, ListNode *Variables);

void MessageBusRegister(const char *URL, int MaxConnections, int Timeout, MSG_FUNC AgentFunction)
{
TMessageBus *MA;

if (! MessageBusConnections) MessageBusConnections=ListCreate();

MA=(TMessageBus *) calloc(1,sizeof(TMessageBus));
MA->URL=CopyStr(MA->URL, URL);
MA->Connections=ListCreate();
MA->AgentFunction=AgentFunction;
ListAddItem(MessageBusConnections, MA); 
}



int MessageBusNativeRecv(STREAM *S, char **Source, ListNode *Vars)
{
char *Tempstr=NULL, *EncName=NULL, *EncValue=NULL;
const char *ptr;
ListNode *Node;

Tempstr=STREAMReadLine(Tempstr, S);
if (StrEnd(Tempstr))
{
	//could be blank (timeout) or null. DestroyString will figure it out
	DestroyString(Tempstr);
	return(FALSE);
}

StripTrailingWhitespace(Tempstr);
ptr=GetToken(Tempstr, "?", Source, 0);
while (ptr)
{
	ptr=GetNameValuePair(ptr, "&","=", &EncName, &EncValue);
	Node=ListAddItem(Vars, NULL);
	Node->Tag=HTTPUnQuote(Node->Tag, EncName);
	Node->Item=HTTPUnQuote(Node->Item, EncValue);
}

DestroyString(Tempstr);
DestroyString(EncValue);
DestroyString(EncName);

return(TRUE);
}


void JSONParse(const char *Doc, ListNode *Vars)
{
char *Prefix=NULL, *Name=NULL, *Value=NULL, *Tempstr=NULL, *tptr;
const char *ptr;

ptr=Doc;
while (isspace(*ptr) || (*ptr=='{')) ptr++;
while (ptr)
{
	ptr=GetToken(ptr,",|:", &Name, GETTOKEN_MULTI_SEPARATORS);
	if (strcmp(Name,"}")==0) 
	{
	//we've come out of one level of nesting. remove last item from prefix
	if (StrValid(Prefix)) 
	{
		tptr=strrchr(Prefix, ':');
		if (tptr) 
		{
			*tptr='\0';
			tptr=strrchr(tptr, ':');
			if (tptr) *tptr='\0';
		}
	}
	}
	else
	{
		ptr=GetToken(ptr,",|:",&Value, GETTOKEN_MULTI_SEPARATORS);
		if (strcmp(Value,"{")==0) Prefix=MCatStr(Prefix,Name,":",NULL);
		else
		{
			Tempstr=MCopyStr(Tempstr, Prefix, Name, NULL);
			SetVar(Vars, Tempstr, Value);
		}
	}
}

DestroyString(Tempstr);
DestroyString(Prefix);
DestroyString(Name);
DestroyString(Value);
}


void XMLParse(const char *Doc, ListNode *Vars)
{
char *Name=NULL, *Value=NULL, *Data=NULL, *Extra=NULL;
const char *ptr;

ptr=XMLGetTag(Doc, NULL, &Name, &Data);
while (ptr)
{
	if (StrValid(Name))
	{
		ptr=XMLGetTag(ptr, NULL, &Extra, &Value);
		if (StrValid(Extra))
		{
			if (*Name != '/') SetVar(Vars, Name, "");
			Name=CopyStr(Name, Extra);
		}
		else
		{
			if (*Name != '/')SetVar(Vars, Name, Value);
			ptr=XMLGetTag(ptr, NULL, &Name, &Data);
		}
	}
	else ptr=XMLGetTag(ptr, NULL, &Name, &Data);
}

DestroyString(Name);
DestroyString(Data);
DestroyString(Value);
DestroyString(Extra);
}



int MessageBusRecv(STREAM *S, char **Source, ListNode *Vars)
{
char *Doc=NULL, *ptr;
HTTPInfoStruct *Info;
ListNode *Curr;
TMessageBus *MA;
int result=FALSE;

if (! Vars) return(FALSE);
if (! S) return(FALSE);
ListClear(Vars, DestroyString);

if (S->Type==STREAM_TYPE_HTTP)
{
	Info=(HTTPInfoStruct *) STREAMGetItem(S, "HTTPInfo");
	if (Info)
	{
	HTTPTransact(Info);
	Doc=HTTPReadDocument(Doc, S);
	ptr=STREAMGetValue(S, "HTTP:Content-Type");
	if (strcmp(ptr, "application/json")==0) JSONParse(Doc, Vars);
	if (strcmp(ptr, "application/xml")==0) XMLParse(Doc, Vars);

	HTTPInfoDestroy(Info);
	DestroyString(Doc);
	result=TRUE;
	}
}
else result=MessageBusNativeRecv(S, Source, Vars);

MA=(TMessageBus *) STREAMGetItem(S, "MessageBus:Agent");
Curr=ListFindItem(MA->Connections, S);
if (Curr) Curr->ItemType=0;
if (! result)
{
	if (Curr) ListDeleteNode(Curr);
	STREAMClose(S);
}

return(result);
}


int MessageBusHandler(void *p_MA)
{
char *URL=NULL;
ListNode *Vars;
TMessageBus *MA;

MA=(TMessageBus *) p_MA;
Vars=ListCreate();
MessageS=STREAMFromDualFD(0,1);
MessageS->Type=STREAM_TYPE_MESSAGEBUS;
STREAMSetTimeout(MessageS, MA->Timeout * 100);
STREAMSetItem(MessageS, "MessageBus:Agent", MA);
while (MessageBusRecv(MessageS, &URL, Vars))
{
	if (MA->AgentFunction) MA->AgentFunction(URL, Vars);
}

ListDestroy(Vars,DestroyString);
DestroyString(URL);
}



STREAM *MessageBusConnect(TMessageBus *MA, const char *Args)
{
STREAM *S;
char *Tempstr=NULL;
HTTPInfoStruct *HTTPInfo;

Tempstr=MCopyStr(Tempstr, MA->URL, Args, NULL);
if (strncmp(MA->URL,"http:",5)==0)
{
	HTTPInfo=HTTPInfoFromURL("GET", MA->URL);
	S=HTTPConnect(HTTPInfo);
	STREAMSetItem(S, "HTTPInfo", HTTPInfo);
}
else
{
	S=STREAMSpawnFunction(MessageBusHandler, MA);
	S->Type=STREAM_TYPE_MESSAGEBUS;
}

if (S) STREAMSetItem(S, "MessageBus:Agent", MA);
DestroyString(Tempstr);
return(S);
}



STREAM *MessageBusAgentFindConnect(TMessageBus *MA, const char *Args)
{
STREAM *S=NULL;
ListNode *Curr;

	Curr=ListGetNext(MA->Connections);
	while (Curr) 
	{
		if (! Curr->ItemType) S=(STREAM *) Curr->Item;
		Curr=ListGetNext(Curr);
	}

	if (! S)
	{
	S=MessageBusConnect(MA, Args);
	Curr=ListAddItem(MA->Connections, S);
	Curr->ItemType=MESSAGEAGENT_BUSY;
	}

return(S);
}




STREAM *MessageBusFindConnect(const char *URL, const char *Args)
{
ListNode *Curr;
TMessageBus *MA, *Found=NULL;
STREAM *S=NULL;

Curr=ListGetNext(MessageBusConnections);
while (Curr)
{
	MA=(TMessageBus *) Curr->Item;
	if (strcmp(MA->URL, URL)==0) 
	{
		Found=MA;
		break;
	}
	Curr=ListGetNext(Curr);
}

if (Found) 
{
	S=MessageBusAgentFindConnect(MA, Args);
}

return(S);
}


STREAM *MessageBusWrite(const char *URL, const char *Args)
{
char *Message=NULL;
STREAM *S=NULL;

//we are a messagebus 'client' with a connection to a parent process
//so just write out of that
if (MessageS) S=MessageS;
else
{
	//we are a messagebus 'server' in the parent process, and must find the
	//right connection to write to
	S=MessageBusFindConnect(URL, Args);
	if (! S) return(NULL);
}


if (S->Type == STREAM_TYPE_MESSAGEBUS)
{
	Message=MCopyStr(Message, URL, "?", Args, "\r\n", NULL);
	STREAMWriteLine(Message, S);
	STREAMFlush(S);
}


DestroyString(Message);
return(S);
}


STREAM *MessageBusSend(const char *URL, ListNode *Args)
{
char *Tempstr=NULL, *EncodedName=NULL, *EncodedValue=NULL;
ListNode *Curr;
STREAM *S;
int result;

Tempstr=MCopyStr(Tempstr, URL, "?", NULL);
Curr=ListGetNext(Args);
while (Curr)
{
	EncodedName=HTTPQuote(EncodedName, Curr->Tag);
	EncodedValue=HTTPQuote(EncodedValue, (char *) Curr->Item);
	Tempstr=MCatStr(Tempstr, EncodedName, "=", EncodedValue, "&", NULL);
	Curr=ListGetNext(Curr);
}

S=MessageBusWrite(URL, Tempstr);

DestroyString(Tempstr);
DestroyString(EncodedName);
DestroyString(EncodedValue);

return(S);
}




int MessageQueueAddToSelect(ListNode *SelectList)
{
ListNode *Curr;
TMessageBus *MA;

Curr=ListGetNext(MessageBusConnections);
while (Curr)
{
	MA=(TMessageBus *) Curr->Item;
	ListAppendItems(SelectList, MA->Connections, NULL);
	Curr=ListGetNext(Curr);
}

return(TRUE);
}


