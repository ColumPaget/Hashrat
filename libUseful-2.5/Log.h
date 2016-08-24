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
#define LOGFILE_TIMESTAMP 32
#define LOGFILE_MILLISECS 64
#define LOGFILE_ROTATE_NUMBERS 128
#define LOGFILE_ROTATE_MINUS 256

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  char *Path;
  int Flags;
  int MaxSize;
  int MaxRotate;
  int LogFacility;
  int LastFlushTime;
  int FlushInterval;
	char *LastMessage;
	int RepeatMessage;
  STREAM *S;
} TLogFile;

TLogFile *LogFileGetEntry(const char *FileName);
int LogFileSetDefaults(int Flags, int MaxSize, int MaxRotate, int FlushInterval);
void LogFileSetValues(TLogFile *LogFile, int Flags, int MaxSize, int MaxRotate, int FlushInterval);
int LogFileFindSetValues(const char *FileName, int Flags, int MaxSize, int MaxRotate, int FlushInterval);
void LogFileFlushAll(int Force);
void LogFileClose(const char *Path);
int LogFileAppendTempLog(const char *LogPath, const char *TmpLogPath);
void LogFileCheckRotate(const char *FileName);


int LogToSTREAM(STREAM *S, int Flags, const char *Str);
int LogWrite(TLogFile *Log, const char *Str);
int LogToFile(const char *FileName, const char *fmt, ...);

#ifdef __cplusplus
}
#endif


#endif
