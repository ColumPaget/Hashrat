#include "ConnectManager.h"
#include "socket.h"
#include "SpawnPrograms.h"

ListNode *ConnectManServers=NULL;
ListNode *ConnectManClients=NULL;
ListNode *Timers=NULL;


int ConnectManagerAddServer(int sock, char *Name,  CONNECT_FUNC OnConnect, ONDATA_FUNC OnData)
{
TConnectManagerItem *Item;

if (! ConnectManServers) ConnectManServers=ListCreate();

if (sock==-1) return(FALSE);

Item=(TConnectManagerItem *) calloc(1,sizeof(TConnectManagerItem));
Item->OnConnect=OnConnect;
Item->OnData=OnData;
Item->Data=(void *) STREAMFromFD(sock);
Item->Name=CopyStr(Item->Name,Name);

ListAddItem(ConnectManServers,Item);
return(TRUE);
}




STREAM *ConnectManagerAddClient(char *Host, int Port, int Flags, char *Name, CONNECT_FUNC OnConnect, ONDATA_FUNC OnData)
{
STREAM *S;
TConnectManagerItem *Item;

if (! ConnectManClients) ConnectManClients=ListCreate();

S=STREAMCreate();
if (! STREAMConnectToHost(S,Host,Port,Flags))
{
STREAMClose(S);
return(NULL);
}

Item=(TConnectManagerItem *) calloc(1,sizeof(TConnectManagerItem));
Item->OnConnect=OnConnect;
Item->OnData=OnData;
Item->Data=(void *) S;
Item->Name=CopyStr(Item->Name,Name);
Item->Host=CopyStr(Item->Host,Host);
Item->Port=Port;

if (Item->OnConnect && STREAMIsConnected(S)) Item->OnConnect(Item);

ListAddItem(ConnectManClients,Item);
return(S);
}



TConnectManagerItem *ConnectManagerAddIncoming(STREAM *S, char *Name, ONDATA_FUNC OnData)
{
TConnectManagerItem *Item=NULL;

if (! ConnectManClients) ConnectManClients=ListCreate();

Item=(TConnectManagerItem *) calloc(1,sizeof(TConnectManagerItem));
Item->OnData=OnData;
Item->Data=(void *) S;
Item->Name=CopyStr(Item->Name,Name);

ListAddItem(ConnectManClients,Item);
return(Item);
}




int ConnectManagerAddTimer(int Secs, char *Name, ONTIMER_FUNC OnTime, void *Data)
{
TConnectManagerItem *Item;

if (! Timers) Timers=ListCreate();

Item=(TConnectManagerItem *) calloc(1,sizeof(TConnectManagerItem));
Item->OnData=(ONDATA_FUNC) OnTime;
Item->Name=CopyStr(Item->Name,Name);
Item->TimerVal=Secs;
Item->LastTimerFire=time(NULL);
Item->Data=Data;

ListAddItem(Timers,Item);
return(TRUE);
}



int ConnectManagerCountNamedConnections(char *Name)
{
TConnectManagerItem *Item;
ListNode *Curr;
int count=0;

	Curr=ListGetNext(ConnectManClients);
	while (Curr)
	{
		Item=(TConnectManagerItem *) Curr->Item;
		if (strcmp(Item->Name,Name)==0) count++;
		Curr=ListGetNext(Curr);
	}

return(count);
}


STREAM *ConnectManagerGetStreamByName(char *Name)
{
TConnectManagerItem *Item;
ListNode *Curr;
int count=0;

	Curr=ListGetNext(ConnectManClients);
	while (Curr)
	{
		Item=(TConnectManagerItem *) Curr->Item;
		if (strcmp(Item->Name,Name)==0) return((STREAM *) Item->Data);
		Curr=ListGetNext(Curr);
	}

return(NULL);
}


ListNode *ConnectManagerGetConnectionList()
{
	return(ConnectManClients);
}

void ConnectManagerMainLoop()
{
TConnectManagerItem *Item, *NewItem;
ListNode *Curr, *Prev;
int highfd=0;
fd_set ReadSet, WriteSet;
int ipaddr, sock, result, SelectResult, NextTimerFire;
STREAM *S;
time_t Now;
struct timeval tv;
int MoreData=FALSE;

while (1)
{
	MoreData=FALSE;
  time(&Now);
 	NextTimerFire=60;
	Curr=ListGetNext(Timers);
	while (Curr)
	{
		Item=(TConnectManagerItem *) Curr->Item;
		if (Item->LastTimerFire==0) Item->LastTimerFire=Now;
		result=(Item->LastTimerFire + Item->TimerVal) - Now;
	    if (result < NextTimerFire) NextTimerFire=result;
		Curr=ListGetNext(Curr);
	}



		FD_ZERO(&ReadSet);
		FD_ZERO(&WriteSet);
		Curr=ListGetNext(ConnectManServers);
		while (Curr)
		{
			Item=(TConnectManagerItem *) Curr->Item;
		 	 S=(STREAM *) Item->Data;
			FD_SET(S->in_fd,&ReadSet);
			if (S->in_fd > highfd) highfd=S->in_fd;
			Curr=ListGetNext(Curr);
		}


		Curr=ListGetNext(ConnectManClients);
		while (Curr)
		{
			Item=(TConnectManagerItem *) Curr->Item;
		  S=(STREAM *) Item->Data;
			if (! S) 
			{
				ListDeleteNode(Curr);
				continue;
			}
 
			if (S->State & SS_CONNECTING)
			{
				FD_SET(S->in_fd,&WriteSet);
			}
			if (S->InEnd > S->InStart) MoreData=TRUE;
			else
			{
			//always add to read set
			FD_SET(S->in_fd,&ReadSet);
			if (S->in_fd > highfd) highfd=S->in_fd;
			}

			Curr=ListGetNext(Curr);
		}

	if (MoreData)
	{
	tv.tv_usec = 10;
  tv.tv_sec = 0;
	SelectResult=0;
	}
	else
	{
		//SELECT!!!
 	 tv.tv_usec = 20000;
 	 tv.tv_sec = NextTimerFire;
	}
	SelectResult=select(highfd+1,&ReadSet,&WriteSet,NULL,&tv);


	if (SelectResult > 0)
	{
		Curr=ListGetNext(ConnectManServers);
		while (Curr)
		{
			Item=(TConnectManagerItem *) Curr->Item;

   		S=(STREAM *) Item->Data;
			if (! S) 
			{
				ListDeleteNode(Curr);
				continue;
			}


			if (FD_ISSET(S->in_fd,&ReadSet))
			{
				sock=TCPServerSockAccept(S->in_fd,&ipaddr);
				if (sock > -1)
				{
				S=STREAMFromFD(sock);
				STREAMSetFlushType(S,FLUSH_LINE,0);
				S->Path=CopyStr(S->Path,IPtoStr(ipaddr));
				NewItem=ConnectManagerAddIncoming(S,Item->Name, Item->OnData);
				if (Item->OnConnect) Item->OnConnect(NewItem);	
				}
			}

			Curr=ListGetNext(Curr);
		}
	}


		Curr=ListGetNext(ConnectManClients);
		while (Curr)
		{
			Item=(TConnectManagerItem *) Curr->Item;
	 		S=(STREAM *) Item->Data;
			if (! S) 
			{
				ListDeleteNode(Curr);
				continue;
			}

			if ((SelectResult > 0) && FD_ISSET(S->in_fd,&WriteSet))
			{
				if (S->State & SS_CONNECTING)
				{
					if (STREAMIsConnected(S)) 
					{
						STREAMSetNonBlock(S,FALSE);
						if (Item->OnConnect) Item->OnConnect(Item);
					}

				}
			}

			if (
					 (S->InEnd > S->InStart) ||
					 ((SelectResult > 0) && (FD_ISSET(S->in_fd,&ReadSet)))
				)
			{
                if (! (S->State & SS_CONNECTING))
                {
				if (Item->OnData)
				{
					  result=Item->OnData(S, Item->Name);
					  if (! result)
						{	
					    STREAMClose(S);
							Prev=ListGetPrev(Curr);
							ListDeleteNode(Curr);
							free(Item);
						  Curr=Prev;	
					  }
						else if (result==RECONNECT)
						{
							STREAMClose(S);
							S=STREAMCreate();
							STREAMConnectToHost(S,Item->Host,Item->Port,CONNECT_NONBLOCK);
							Item->Data=(void *) S;
						}

				}
				}
			}
			Curr=ListGetNext(Curr);
		}


  time(&Now);
	Curr=ListGetNext(Timers);
	while (Curr)
	{
	   Item=(TConnectManagerItem *) Curr->Item;
	   if ( (Now - Item->LastTimerFire) >= Item->TimerVal ) 
       {
			if (Item->OnData) ((ONTIMER_FUNC)Item->OnData)(Item->Data,Item->Name);
			Item->LastTimerFire=Now;
       }
	  Curr=ListGetNext(Curr);
	}

	
}

}

