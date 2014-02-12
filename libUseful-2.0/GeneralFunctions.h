#ifndef LIBUSEFUL_GENERAL_H
#define LIBUSEFUL_GENERAL_H

#include <stdio.h>
#include "defines.h"



#ifdef __cplusplus
extern "C" {
#endif

int WritePidFile(char *ProgName);
int HexStrToBytes(char **Buffer, char *HexStr);
char *BytesToHexStr(char *Buffer, char *Bytes, int len);

void SwitchProgram(char *CommandLine);
int SwitchUser(char *User);
int SwitchGroup(char *Group);
char *GetCurrUserHomeDir();


void ColLibDefaultSignalHandler(int sig);
int CreateLockFile(char *FilePath,int Timeout);

char *GetRandomData(char *RetBuff, int len, char *AllowedChars);
char *GetRandomHexStr(char *RetBuff, int len);
char *GetRandomAlphabetStr(char *RetBuff, int len);

void CloseOpenFiles();

int BASIC_FUNC_EXEC_COMMAND(void *Data);


char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value);
double ParseHumanReadableDataQty(char *Data, int Type);
char *GetHumanReadableDataQty(double Size, int Type);

void EraseString(char *Buff, char *Target);


#ifdef __cplusplus
}
#endif


#endif
