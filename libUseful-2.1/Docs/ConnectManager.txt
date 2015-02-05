#ifndef LIBUSEFUL_CONNECT_MANAGER_H
#define LIBUSEFUL_CONNECT_MANAGER_H

#include "file.h"
#include "includes.h"

#define RECONNECT 2

#ifdef __cplusplus
extern "C" {
#endif


typedef struct t_con_man_item TConnectManagerItem;


typedef int (*CONNECT_FUNC)(TConnectManagerItem *Item);
typedef int (*ONDATA_FUNC)(STREAM *S, char *Name);
typedef int (*ONTIMER_FUNC)(void *Data, char *Name);



typedef struct t_con_man_item
{
char *Name;
void *Data;
int TimerVal;
int LastTimerFire;
char *Host;
int Port;
CONNECT_FUNC OnConnect;
ONDATA_FUNC OnData;
};


int ConnectManagerAddServer(int sock, char *Name,  CONNECT_FUNC OnConnect, ONDATA_FUNC OnData);
STREAM *ConnectManagerAddClient(char *Host, int Port, int Flags, char *Name, CONNECT_FUNC OnConnect, ONDATA_FUNC OnData);
TConnectManagerItem *ConnectManagerAddIncoming(STREAM *S, char *Name, ONDATA_FUNC OnData);
int ConnectManagerAddTimer(int Secs, char *Name, ONTIMER_FUNC OnTime, void *Data);
int ConnectManagerCountNamedConnections(char *Name);
STREAM *ConnectManagerGetStreamByName(char *Name);
ListNode *ConnectManagerGetConnectionList();
void ConnectManagerMainLoop();

#ifdef __cplusplus
}
#endif


#endif
