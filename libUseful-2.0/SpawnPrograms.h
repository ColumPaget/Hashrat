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
int ForkWithContext();
/* This function turns our process into a demon */
int demonize();
int ForkWithIO(int StdIn, int StdOut, int StdErr);
int SpawnWithIO(char *CommandLine, int StdIn, int StdOut, int StdErr);
int Spawn(char *ProgName);
/* This creates a child process that we can talk to using a couple of pipes*/
int PipeSpawnFunction(int *infd,int  *outfd,int  *errfd, BASIC_FUNC Func, void *Data );
int PipeSpawn(int *infd,int  *outfd,int  *errfd, const char *Command);
int PseudoTTYSpawn(int *pty, const char *Command, int Flags);
STREAM *STREAMSpawnCommand(const char *Command, int Type);

#ifdef __cplusplus
}
#endif


#endif
