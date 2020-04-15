/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_PROCESS_H
#define LIBUSEFUL_PROCESS_H

#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>

//Various functions related to a process

#define PROC_DAEMON 8
#define PROC_SETSID 16
#define PROC_CTRL_TTY 32
#define PROC_CHROOT 64
#define PROC_JAIL 128
#define PROC_SIGDEF 256
#define PROC_CONTAINER 512
#define PROC_CONTAINER_NET  1024
#define PROC_ISOCUBE  2048

//these must be compatible with PROC_ defines
#define SPAWN_NOSHELL 8192
#define SPAWN_TRUST_COMMAND 16384
#define SPAWN_ARG0 32768

#define PROC_SETUP_FAIL 65536


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SIGNAL_HANDLER_FUNC)(int sig);
void LU_DefaultSignalHandler(int sig);

// make the current process a background process/service, with no connection to a terminal.
// Beware, this will change its pid. Returns the new pid or '0' on failure
pid_t demonize();

//singply try to close all file descriptors from 3 to 1024, leaving stdin, stdout and stderr alone
void CloseOpenFiles();

//switch user or group by id (call SwitchGID first, or you may not have permissions to switch user)
int SwitchUID(int uid);
int SwitchGID(int gid);

//switch user or group by name (call SwitchGgroup first, or you may not have permissions to switch user)
int SwitchUser(const char *User);
int SwitchGroup(const char *Group);

//returns a string pointing to a users home directory. DO NOT FREE THIS STRING. Take a copy of it, as it's
//an internal buffer and will change on the next call to this function
char *GetCurrUserHomeDir();

//write a file containing the current pid. The file can either be an absolute path to a file anywhere
//or a relative path that creates a file in /var/run. The file is locked with flock
//returns a file descriptor to the current file
int WritePidFile(const char *ProgName);

int CreateLockFile(const char *FilePath, int Timeout);

//pass argv from main to this function if you want to rewrite the process name that you see in 'ps'
void ProcessTitleCaptureBuffer(char **argv);

//set the process name that you see in 'ps'. Works like sprintf, with a format string and a variable number of
//arguments. ProcessTitleCaptureBuffer must be called first
void ProcessSetTitle(const char *FmtStr, ...);


int ProcessApplyConfig(const char *Config);

#ifdef __cplusplus
}
#endif

#endif
