/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_LOG_H
#define LIBUSEFUL_LOG_H

#include "includes.h"
#include "defines.h"
#include "Stream.h"


/*
This module provides log file functions. It keeps an internal list of log files so that you can just write to them without
worrying about opening or closeing them. These are separate log files from the syslog system, though these functions can
write to syslog. These logfiles can auto-rotate when they reach a certain size. 

For most people only the function LogToFile will be relevant and perhaps LogFileSetDefaults and LogFileFindSetValues.
*/


//Flags passed to LogFileCreate and other functions that have a 'Flags' argument
#define LOGFILE_DEFAULTS 0 
#define LOGFILE_FLUSH 1        // flush on every write
#define LOGFILE_SYSLOG 2       // send a copy of the log message to syslog
#define LOGFILE_LOGPID 4       // include the pid  in the log message
#define LOGFILE_LOGUSER 8      // include the user of the current process in the log message
#define LOGFILE_LOCK 16        // lock the file on every write
#define LOGFILE_TIMESTAMP 32   // include a timestamp in the log message
#define LOGFILE_MILLISECS 64   // include milliseconds in the timestamp
#define LOGFILE_HARDEN 512     // harden logfile against tampering (uses appendonly and immutable flags on filesystems that support it)
#define LOGFILE_REPEATS   4096  // don't write repeat lines, instead count them and write 'last message repeated %d times'
#define LOGFILE_CACHETIME 8192  // use cached time in logfiles. This means user must called 'GetTime(0)' themselves to update time
#define LOGFILE_PLAIN 536870912

#define LOGFLUSH_FORCE 1
#define LOGFLUSH_REOPEN 2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    char *Path;
    int Flags;
    uint64_t MaxSize;
    int MaxRotate;
    int LogFacility;
    time_t LastMessageTime;
    time_t LastFlushTime;
    time_t FlushInterval;
    time_t NextMarkTime;
    time_t MarkInterval;
    char *LastMessage;
    int RepeatCount;
    STREAM *S;
} TLogFile;


//Get list of currently active log files
ListNode *LogFilesGetList();

//create a new log file object. If 'FileName' is SYSLOG, then log to syslog instead of a file. If 'FileName' is STDOUT then log to standard out,
//if it's STDERR then log to standard error
TLogFile *LogFileCreate(const char *FileName, int Flags);

//Get a particular log object
TLogFile *LogFileGetEntry(const char *FileName);

//Set the default values for all logfiles. 'MaxSize' is the maxium size a file can be before it's rotated. 'MaxRotate' is the number of
//rotated archive files to keep. 'FlushInterval' is the maximum number of seconds that a log message should be held in buffers until
//it's written to disk.
void LogFileSetDefaults(int Flags, int MaxSize, int MaxRotate, int FlushInterval);

//set values for a specific logfile
void LogFileSetValues(TLogFile *LogFile, int Flags, int MaxSize, int MaxRotate, int FlushInterval);

//find a logfile by name and set its values
int LogFileFindSetValues(const char *FileName, int Flags, int MaxSize, int MaxRotate, int FlushInterval);

//append a file (could be another logfile) to a logfile
int LogFileAppendTempLog(const char *LogPath, const char *TmpLogPath);

//Flush all logfiles. 
void LogFileFlushAll(int Force);

//Close a specific logfile if it's open. 
void LogFileClose(const char *Path);

//Close all currently open logfiles
void LogFileCloseAll();


//This writes a log message to a stream, rather than an internally managed logfile object. 
int LogToSTREAM(STREAM *S, int Flags, const char *Str);

//log message to a TLogFile object
int LogWrite(TLogFile *Log, const char *Str);

//this is the main function that you'll call to log to a logfile. 
int LogToFile(const char *FileName, const char *fmt, ...);

//check if a logfile needs to be rotated and do so if it does
void LogFileCheckRotate(const char *FileName);

//check all known logfiles to be rotated or other scheduled tasks (e.g. 'MARK' lines)
void LogFilesHousekeep();

#ifdef __cplusplus
}
#endif


#endif
