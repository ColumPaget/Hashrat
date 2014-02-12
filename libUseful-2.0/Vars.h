#ifndef LIBUSEFUL_VARS_H
#define LIBUSEFUL_VARS_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

void SetVar(ListNode *Vars, const char *Name, const char *Data);
char *GetVar(ListNode *Vars, const char *Name);
void UnsetVar(ListNode *Vars,const char *Name);
void ClearVars(ListNode *Vars);
void CopyVars(ListNode *Dest, ListNode *Source);
char *SubstituteVarsInString(char *Buffer, const char *Fmt, ListNode *Vars, int Flags);
int ExtractVarsFromString(char *Data, const char *FormatStr, ListNode *Vars);


#ifdef __cplusplus
}
#endif

#endif
