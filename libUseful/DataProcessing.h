/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_DATA_PROCESSING_H
#define LIBUSEFUL_DATA_PROCESSING_H

#include "includes.h"

/*
Most users do not need to know about this module. It concerns 'DataProcessor' items that can be added to a stream to
process data as it is read. This is mostly used by libUseful's HTTP functionality to put processors like 'gzip decompress'
or 'ChunkedTransferEncoding' into a stream object to make these encodings transparent to the user/programmer.

DataProcessorModules have an 'Init' function that is called to set them up, then seperate 'Read' and 'Write' functions
so that one processor could be used to alter both directions of a stream, e.g., compressing bytes on write, inflating
them on read. The read/write functions (DATA_PROCESS_RW_FUNC below) are passed a char ** to 'OutBuff' and an
unsigned long * to 'OutBuffLen'. if 'OutBuffLen' bytes are not enough to store their output, they can realloc new
space with realloc or SetStrLen (see String.h) and update OutBuffLen to the new length.

Processing modules return the number of bytes read/written. When there is no more input, the argument 'Flush' will be
true so they know to return all the data they have in hand. If 'Flush' is set, and the Module returns any value of
zero or less (so zero, STREAM_CLOSED, etc) the module will be taken to have finished it's work and be unloaded.

Processing Modules can be added to a stream with STREAMAddStandardDataProcessor. They have a class (e.g. 'Compression')
and a name. They can be deleted by class+name using STREAMDeleteDataProcessor. STREAMClearDataProcessors removes all
Processors from a STREAM object. STREAMAddProgressCallback accepts a function pointer and adds a processor that
calls this function every time some bytes are read, passing the STREAM path, the number of bytes read, and the
number expected, which allows an application to display progress info to the user.

An unusual scenario that occurs with HTTP is that processors have to be added mid way through a datastream being read.
Having read the HTTP headers we learn that the rest of the datastream will be gzip compressed, for instance. But
the STREAM object has already read that data into it's buffers. 'STREAMRebuildDataProcessors' handles this situation.
Add all the new Processing Modules and then call 'STREAMReBuildDataProcessors' and the data currently in the buffers
will be refed back thourgh the Processing chain.

*/


#ifdef __cplusplus
extern "C" {
#endif


typedef struct t_dpmod TProcessingModule;

typedef int (*DATA_PROCESS_INIT_FUNC)(TProcessingModule *Mod, const char *Args);
typedef int (*DATA_PROCESS_RW_FUNC)(TProcessingModule *Mod, const char *Data, unsigned long InLen, char **OutBuff, unsigned long *OutBuffLen, int Flush);
typedef int (*DATA_PROCESS_CLOSE_FUNC)(TProcessingModule *Mod);
typedef void (*DATA_PROGRESS_CALLBACK)(const char *Path, int bytes, int total);

#define DPM_READ_FINAL 1
#define DPM_WRITE_FINAL 2
#define DPM_NOPAD_DATA 4
#define DPM_PROGRESS 8
#define DPM_COMPRESS 16

struct t_dpmod
{
    char *Name;
    char *Args;
    int Flags;
    char *ReadBuff, *WriteBuff;
    int ReadSize, WriteSize;
    int ReadUsed, WriteUsed;
    int ReadMax, WriteMax;
    ListNode *Values;
    DATA_PROCESS_INIT_FUNC Init;
    DATA_PROCESS_RW_FUNC Read;
    DATA_PROCESS_RW_FUNC Write;
    DATA_PROCESS_CLOSE_FUNC Close;
    void *Data;
};


TProcessingModule *StandardDataProcessorCreate(const char *Class, const char *Name, const char *Arg);
int DataProcessorInit(TProcessingModule *ProcMod, const char *Key, const char *InputVector);
void DataProcessorDestroy(void *ProcMod);
char *DataProcessorGetValue(TProcessingModule *M, const char *Name);
void DataProcessorSetValue(TProcessingModule *M, const char *Name, const char *Value);


int DataProcessorAvailable(const char *Class, const char *Name);
int STREAMAddStandardDataProcessor(STREAM *S, const char *Class, const char *Name, const char *Args);
int STREAMAddProgressCallback(STREAM *S, DATA_PROGRESS_CALLBACK CallBack);
void STREAMClearDataProcessors(STREAM *S);
int STREAMAddDataProcessor(STREAM *S, TProcessingModule *Mod, const char *Args);
int STREAMReBuildDataProcessors(STREAM *S);
int STREAMDeleteDataProcessor(STREAM *S, char *Class, char *Name);


#ifdef __cplusplus
}
#endif



#endif
