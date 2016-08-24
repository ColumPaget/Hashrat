#ifndef LIBUSEFUL_MESSAGE_BUS_H
#define LIBUSEFUL_MESSAGE_BUS_H

//Vars gives us list node, and also 'GetVar' which is used in the
//.c module of this .h
#include "Vars.h"
#include "file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*MSG_FUNC)(const char *PeerName, ListNode *Variables);

void MessageBusRegister(const char *URL, int MaxConnections, int Timeout, MSG_FUNC);
STREAM *MessageBusWrite(const char *URL, const char *Args);
STREAM *MessageBusSend(const char *URL, ListNode *Args);
int MessageBusRecv(STREAM *S, char **Source, ListNode *Vars);
int MessageQueueAddToSelect(ListNode *SelectList);


#ifdef __cplusplus
}
#endif

#endif
