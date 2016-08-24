#ifndef LIBUSEFUL_FILE_H
#define LIBUSEFUL_FILE_H

#include <fcntl.h>
#include <sys/time.h> //for 'struct timeval'
#include "list.h"


#define STREAM_FLUSH -1

#define STREAM_CLOSED -1
#define STREAM_NODATA -2
#define STREAM_TIMEOUT -3
#define STREAM_DATA_ERROR -4


/*
#define O_ACCMODE      00000003
#define O_RDONLY       00000000
#define O_WRONLY       00000001
#define O_RDWR         00000002
#define O_CREAT        00000100 
#define O_EXCL         00000200        
#define O_NOCTTY       00000400
#define O_TRUNC        00001000
#define O_APPEND       00002000
#define O_NONBLOCK     00004000
#define O_DSYNC        00010000
#define O_DIRECT       00040000
#define O_LARGEFILE    00100000
#define O_DIRECTORY    00200000
#define O_NOFOLLOW     00400000
#define O_NOATIME      01000000
#define O_CLOEXEC      02000000
*/

//Flags that alter stream behavior
#define FLUSH_FULL 0
#define FLUSH_LINE 1
#define FLUSH_BLOCK 2
#define FLUSH_ALWAYS 4
#define FLUSH_BUFFER 8

#define SF_RDWR 0 //is the default
//FLUSH_ flags go in this gap
#define SF_RDONLY 16
#define SF_WRONLY 32
#define SF_CREAT 64
#define SF_CREATE 64
#define SF_APPEND 128
#define SF_TRUNC 256
#define SF_MMAP  512
#define SF_WRLOCK 1024
#define SF_RDLOCK 2048
#define SF_FOLLOW 4096
#define SF_SECURE 8192
#define SF_NONBLOCK 16384
#define SF_EXEC_INHERIT 131072
#define SF_SYMLINK_OK 262144
#define SF_NOCACHE 524288
#define SF_SORTED  1048576


//Stream state values
#define SS_CONNECTING 1
#define SS_CONNECTED 2
#define SS_HANDSHAKE_DONE 4
#define SS_DATA_ERROR 8
#define SS_WRITE_ERROR 16
#define SS_EMBARGOED 32
#define SS_SSL  4096
#define SS_AUTH 8192
#define SS_USER1 268435456
#define SS_USER2 536870912
#define SS_USER3 1073741824
#define SS_USER4 2147483648

#define STREAM_TYPE_FILE 0
#define STREAM_TYPE_UNIX 1
#define STREAM_TYPE_UNIX_DGRAM 2
#define STREAM_TYPE_TCP 3
#define STREAM_TYPE_UDP 4
#define STREAM_TYPE_SSL 5
#define STREAM_TYPE_HTTP 6
#define STREAM_TYPE_CHUNKED_HTTP 7
#define STREAM_TYPE_MESSAGEBUS 8

#define O_LOCK O_NOCTTY


#define SELECT_READ 1
#define SELECT_WRITE 2	


#define SENDFILE_KERNEL 1
#define SENDFILE_LOOP 2

typedef struct
{
int Type;
int in_fd, out_fd;
unsigned int Flags;
unsigned int State;
unsigned int Timeout;
unsigned int BlockSize;
unsigned int BuffSize;
unsigned int StartPoint;

unsigned int InStart, InEnd;
unsigned int OutEnd;
char *InputBuff;
char *OutputBuff;

unsigned int BytesRead;
unsigned int BytesWritten;
char *Path;
ListNode *ProcessingModules;
ListNode *Values;
ListNode *Items;
} STREAM;


#ifdef __cplusplus
extern "C" {
#endif


int FDSelect(int fd, int Flags, struct timeval *tv);
int FDIsWritable(int);
int FDCheckForBytes(int);

void STREAMSetFlags(STREAM *S, int Set, int UnSet);
void STREAMSetTimeout(STREAM *, int);
void STREAMSetFlushType(STREAM *Stream, int Type, int StartPoint, int BlockSize);


STREAM *STREAMCreate();
STREAM *STREAMOpenFile(const char *Path, int Mode);
STREAM *STREAMClose(STREAM *Stream);
int STREAMLock(STREAM *S, int val);
int STREAMFlush(STREAM *Stream);
void STREAMClear(STREAM *Stream);
double STREAMTell(STREAM *Stream);
double STREAMSeek(STREAM *Stream, double, int whence);
void STREAMResizeBuffer(STREAM *, int);
int STREAMReadChar(STREAM *);
int STREAMWriteChar(STREAM *,unsigned char);
char* STREAMReadLine(char *Buffer, STREAM *);
int STREAMReadBytesToTerm(STREAM *S, char *Buffer, int BuffSize,unsigned char Term);
char* STREAMReadToTerminator(char *Buffer, STREAM *,unsigned char Term);
char* STREAMReadToMultiTerminator(char *Buffer, STREAM *, char *Terms);
int STREAMWriteString(const char *Buffer, STREAM *);
int STREAMWriteLine(const char *Buffer, STREAM *);
STREAM *STREAMFromFD(int fd);
STREAM *STREAMFromDualFD(int in_fd, int out_fd);

int STREAMDisassociateFromFD(STREAM *Stream);
int STREAMPeekChar(STREAM *);
int STREAMPeekBytes(STREAM *S, char *Buffer, int Buffsize);
void STREAMResetInputBuffers(STREAM *S);
int STREAMReadThroughProcessors(STREAM *S, char *Bytes, int InLen);

int STREAMReadBytes(STREAM *, char *Buffer, int Bytes);
int STREAMWriteBytes(STREAM *, const char *Buffer, int Bytes);
int STREAMCheckForBytes(STREAM *);
int STREAMCheckForWaitingChar(STREAM *S,unsigned char check_char);
int STREAMCountWaitingBytes(STREAM *);
STREAM *STREAMSelect(ListNode *Streams, struct timeval *timeout);

void STREAMSetValue(STREAM *S, const char *Name, const char *Value);
char *STREAMGetValue(STREAM *S, const char *Name);
void STREAMSetItem(STREAM *S, const char *Name, void *Item);
void *STREAMGetItem(STREAM *S, const char *Name);

unsigned long STREAMSendFile(STREAM *In, STREAM *Out, unsigned long Max, int Flags);

#ifdef __cplusplus
}
#endif



#endif
