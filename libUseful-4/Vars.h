/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_VARS_H
#define LIBUSEFUL_VARS_H

/*
Utility functions to set named variables. Really just a wrapper around the List functions.
*/

#include "List.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GetVar(List, Name) GetTypedVar((List), (Name), ANYTYPE)
#define UnsetVar(List, Name) UnsetTypedVar((List), (Name), ANYTYPE)

ListNode *SetDetailVar(ListNode *Vars, const char *Name, const char *Data, int ItemType, time_t Time);
ListNode *SetTypedVar(ListNode *Vars, const char *Name, const char *Data, int Type);
ListNode *SetVar(ListNode *Vars, const char *Name, const char *Data);
const char *GetTypedVar(ListNode *Vars, const char *Name, int Type);
void UnsetTypedVar(ListNode *Vars,const char *Name, int Type);
void ClearVars(ListNode *Vars);
void CopyVars(ListNode *Dest, ListNode *Source);
char *SubstituteVarsInString(char *Buffer, const char *Fmt, ListNode *Vars, int Flags);
int ExtractVarsFromString(const char *Data, const char *FormatStr, ListNode *Vars);


#ifdef __cplusplus
}
#endif

#endif
