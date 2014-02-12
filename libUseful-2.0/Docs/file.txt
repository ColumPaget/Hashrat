#ifndef LIBUSEFUL_FILE_H
#define LIBUSEFUL_FILE_H

#include <fcntl.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif


#define STREAM_CLOSED -1
#define STREAM_NODATA -2
#define STREAM_TIMEOUT -3
#define STREAM_DATA_ERROR -4

#define FLUSH_FULL 0
#define FLUSH_LINE 1
#define SF_AUTH 2
#define SF_SYMLINK_OK 4
#define SF_CONNECTING 8
#define SF_CONNECTED 16
#define SF_HANDSHAKE_DONE 32
#define SF_WRONLY 64
#define SF_RDONLY 128
#define SF_RDWR 256
#define SF_SSL  512
#define SF_DATA_ERROR 1024
#define SF_WRITE_ERROR 2048
#define SF_NONBLOCK 4096

#define STREAM_TYPE_FILE 0
#define STREAM_TYPE_UNIX 1
#define STREAM_TYPE_UNIX_DGRAM 2
#define STREAM_TYPE_TCP 3
#define STREAM_TYPE_UDP 4
#define STREAM_TYPE_SSL 5
#define STREAM_TYPE_HTTP 6
#define STREAM_TYPE_CHUNKED_HTTP 7

#define O_LOCK O_NOCTTY

#define COMMS_BY_PIPE 0
#define COMMS_BY_PTY 1
	
typedef struct
{
char *InputBuff;
char *OutputBuff;
int Timeout;
int in_fd, out_fd;
int Type;
int Flags;
unsigned int BuffSize;
unsigned int InStart, InEnd;
unsigned int OutStart, OutEnd;
unsigned int BytesRead;
unsigned int BytesWritten;
char *Path;
void *Extra;
ListNode *Values;
ListNode *ProcessingModules;
ListNode *Index;
} STREAM;


STREAM *STREAMCreate();
STREAM *STREAMOpenFile(char *Path, int Mode);
STREAM *STREAMClose(STREAM *Stream);
void STREAMSetNonBlock(STREAM *S, int val);
int STREAMLock(STREAM *S, int val);
int STREAMFlush(STREAM *Stream);
void STREAMClear(STREAM *Stream);
int STREAMTell(STREAM *Stream);
int STREAMSeek(STREAM *Stream, off_t offset, int whence);
void STREAMResizeBuffer(STREAM *, int);
void STREAMSetTimeout(STREAM *, int);
void STREAMSetFlushType(STREAM *Stream, int Type, int val);
int STREAMReadChar(STREAM *);
int STREAMWriteChar(STREAM *, char);
char* STREAMReadLine(char *Buffer, STREAM *);
int ReadBytesToTerm(STREAM *S, char *Buffer, int BuffSize, char Term);
char* STREAMReadToTerminator(char *Buffer, STREAM *, char Term);
char* STREAMReadToMultiTerminator(char *Buffer, STREAM *, char *Terms);
int STREAMWriteString(char *Buffer, STREAM *);
int STREAMWriteLine(char *Buffer, STREAM *);
STREAM *STREAMFromFD(int fd);
STREAM *STREAMFromDualFD(int in_fd, int out_fd);
STREAM *STREAMSpawnCommand(char *Command, int type);

 int STREAMDisassociateFromFD(STREAM *Stream);
 int STREAMPeekChar(STREAM *);

int STREAMReadBytes(STREAM *, char *Buffer, int Bytes);
int STREAMWriteBytes(STREAM *, char *Buffer, int Bytes);
int FDCheckForBytes(int);
int STREAMCheckForBytes(STREAM *);
int STREAMCheckForWaitingChar(STREAM *S, char check_char);
int STREAMCountWaitingBytes(STREAM *);


int IndexedFileLoad(STREAM *S);
int IndexedFileFind(STREAM *S, char *Key);
int IndexedFileWrite(STREAM *S, char *Line);


#ifdef __cplusplus
}
#endif



#endif
