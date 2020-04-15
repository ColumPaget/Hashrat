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

//get a variable from the list by name
#define GetVar(List, Name) GetTypedVar((List), (Name), ANYTYPE)

//unset a variable by name
#define UnsetVar(List, Name) UnsetTypedVar((List), (Name), ANYTYPE)

//set a variable with a type and a timestamp
ListNode *SetDetailVar(ListNode *Vars, const char *Name, const char *Data, int ItemType, time_t Time);

//set a variable with a type
ListNode *SetTypedVar(ListNode *Vars, const char *Name, const char *Data, int Type);

//set a variable
ListNode *SetVar(ListNode *Vars, const char *Name, const char *Data);

//get a variable by name and type
const char *GetTypedVar(ListNode *Vars, const char *Name, int Type);

//unset a variable by name and type
void UnsetTypedVar(ListNode *Vars,const char *Name, int Type);

//clear all variables
void ClearVars(ListNode *Vars);

//copy variables from one list to another
void CopyVars(ListNode *Dest, ListNode *Source);

//given a format string containing vars in the form $(VarName) and a list of variables, substitute the $(VarName) entries
//with the approriate named variable
char *SubstituteVarsInString(char *Buffer, const char *Fmt, ListNode *Vars, int Flags);

//given a format string containing vars in the form $(VarName) and a 'Data' string, extract strings from the 'Data' string
//that positionally match the $(VarName) entries, and add these to the varible list 'Vars'
int ExtractVarsFromString(const char *Data, const char *FormatStr, ListNode *Vars);

//given a string that contains '$(VarName)' entries, return a list of VarNames in 'Vars'.
//The names are set in the '->Tag' part of the ListNode structures, so that values can be set to the '->Item' part of the structure
int FindVarNamesInString(const char *Data, ListNode *Vars);

#ifdef __cplusplus
}
#endif

#endif
