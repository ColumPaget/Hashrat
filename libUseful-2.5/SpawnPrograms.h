#ifndef LIBUSEFUL_SPAWN_H
#define LIBUSEFUL_SPAWN_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COMMS_BY_PIPE 0
#define COMMS_BY_PTY 1
#define SPAWN_TRUST_COMMAND 2
#define COMMS_COMBINE_STDERR 4

//up to 128, beyond that is TTYFLAG_

#define SHELLSAFE_BLANK 1

char *MakeShellSafeString(char *RetStr, const char *String, int SafeLevel);
void SwitchProgram(const char *CommandLine, const char *User, const char *Group, const char *Dir);
int ForkWithContext();
/* This function turns our process into a demon */
int demonize();
int ForkWithIO(int StdIn, int StdOut, int StdErr);
int SpawnWithIO(const char *CommandLine, int StdIn, int StdOut, int StdErr);
int Spawn(const char *ProgName, const char *User, const char *Group, const char *Dir);
/* This creates a child process that we can talk to using a couple of pipes*/
int PipeSpawnFunction(int *infd,int  *outfd,int  *errfd, BASIC_FUNC Func, void *Data, const char *User, const char *Group);
int PipeSpawn(int *infd,int  *outfd,int  *errfd, const char *Command, const char *User, const char *Group);
int PseudoTTYSpawn(int *pty, const char *Command, const char *User, const char *Group, int Flags);
STREAM *STREAMSpawnCommand(const char *Command, const char *User, const char *Group, int Type);
STREAM *STREAMSpawnFunction(BASIC_FUNC Func, void *Data);

#ifdef __cplusplus
}
#endif


#endif
