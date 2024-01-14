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


//these are used by SubstituteVarsInString. See that function below for descriptions of their use
#define SUBS_QUOTE_VARS            1
#define SUBS_CASE_VARNAMES         2
#define SUBS_STRIP_VARS_WHITESPACE 4
#define SUBS_STRIP                 4
#define SUBS_INTERPRET_BACKSLASH   8
#define SUBS_QUOTES               16
#define SUBS_SHELL_SAFE           32
#define SUBS_HTTP_VARS            64



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
//'Flags' is a bitmask of the SUBS_ flags:
// SUBS_QUOTE_VARS              Surround substitutions with quotes, so "name=$(name) occupation=$(job)" becomes "name='Bill Bloggs' occupation='everyman'"
// SUBS_CASE_VARNAMES           Honor case in var names (so ThisThing is not equivalent to thisthing)
// SUBS_STRIP_VARS_WHITESPACE   Strip whitespace surrounding any value before substitution
// SUBS_STRIP                   Strip whitespace surrounding any value before substitution
// SUBS_INTERPRET_BACKSLASH     Interpret backslash in the substitution string as a quote, so don't substitute "\$(thing)"
// SUBS_QUOTES                  Honor quotes in the subsitution string, and don't quote anyting in quotes
// SUBS_SHELL_SAFE              Strip vars of any shell-unsafe characters (;&`) before substituting
// SUBS_HTTP_VARS               Quote values with HTTP style substitution
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
