#ifndef LIBUSEFUL_GENERAL_H
#define LIBUSEFUL_GENERAL_H

#include <stdio.h>
#include "defines.h"



#ifdef __cplusplus
extern "C" {
#endif

void WritePidFile(char *ProgName);
int HexStrToBytes(char **Buffer, char *HexStr);
char *BytesToHexStr(char *Buffer, char *Bytes, int len);
int demonize();

void SwitchProgram(char *CommandLine);
int Spawn(char *CommandLine);
int SpawnWithIO(char *CommandLine, int StdIn, int StdOut, int StdErr);
int ForkWithIO(int StdIn, int StdOut, int StdErr);
int PipeSpawnFunction(int *infd, int *outfd, int *errfd, BASIC_FUNC Func, void *Data);
int PipeSpawn(int *infd, int *outfd, int *errfd, char *CommandLine);
int FileExists(char *);
int LogToFile(char *,char *,...);
int LogFileSetValues(char *FileName, int Flags, int MaxSize, int FlushInterval);
void LogFileFlushAll(int ForceFlush);
void ColLibDefaultSignalHandler(int sig);
void SetTimeout(int timeout);
int CreateLockFile(char *FilePath,int Timeout);
char *GetDateStr(char *Format, char *Timezone);
char *GetDateStrFromSecs(char *Format, time_t Secs, char *Timezone);
time_t DateStrToSecs(char *Format, char *Str);
double EvaluateMathStr(char *String);
int MakeDirPath(char *Path, int DirMask);

int SwitchUser(char *User);
int SwitchGroup(char *Group);
char *GetCurrUserHomeDir();
char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value);

void SetVar(ListNode *Vars, char *Name, char *Data);
char *GetVar(ListNode *Vars, char *Name);
void UnsetVar(ListNode *Vars,char *VarName);
void ClearVars(ListNode *Vars);
void CopyVars(ListNode *Dest,ListNode *Source);

char *SubstituteVarsInString(char *Buffer, char *Fmt, ListNode *Vars, int Flags);
int ExtractVarsFromString(char *Data, char *FormatStr, ListNode *Vars);
char *GetRandomData(char *RetBuff, int len, char *AllowedChars);
char *GetRandomHexStr(char *RetBuff, int len);
char *GetRandomAlphabetStr(char *RetBuff, int len);

void CloseOpenFiles();
int ChangeFileExtension(char *FilePath, char *NewExt);

int BASIC_FUNC_EXEC_COMMAND(void *Data);

char *XMLGetTag(char *Input, char **TagType, char **TagData);
char *XMLDeQuote(char *RetStr, char *Data);


double ParseHumanReadableDataQty(char *Data, int Type);
char *GetHumanReadableDataQty(double Size, int Type);
char *FindFileInPath(char *InBuff, char *File, char *Path);


#ifdef __cplusplus
}
#endif


#endif
