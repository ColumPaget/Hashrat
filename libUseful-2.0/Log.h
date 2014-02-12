#ifndef LIBUSEFUL_LOG_H
#define LIBUSEFUL_LOG_H

#include "includes.h"
#include "defines.h"
#include "file.h"

#define LOGFILE_FLUSH 1
#define LOGFILE_SYSLOG 2
#define LOGFILE_LOGPID 4
#define LOGFILE_LOGUSER 8
#define LOGFILE_LOCK 16
#define LOGFILE_MILLISECS 32

extern char *G_LogFilePath;

#ifdef __cplusplus
extern "C" {
#endif

int LogFileSetValues(char *FileName, int Flags, int MaxSize, int FlushInterval);
int LogToSTREAM(STREAM *S, int Flags, char *Str);
void LogFileFlushAll(int Force);
int LogToFile(char *FileName,char *fmt, ...);
void LogFileClose(char *Path);
int LogFileAppendTempLog(char *LogPath, char *TmpLogPath);
void LogFileCheckRotate(char *FileName);


#ifdef __cplusplus
}
#endif


#endif
