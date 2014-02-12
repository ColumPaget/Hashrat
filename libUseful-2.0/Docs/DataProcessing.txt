#ifndef LIBUSEFUL_DATA_PROCESSING_H
#define LIBUSEFUL_DATA_PROCESSING_H

#include "includes.h"



#ifdef __cplusplus
extern "C" {
#endif


typedef struct t_dpmod TProcessingModule;

typedef int (*DATA_PROCESS_INIT_FUNC)(TProcessingModule *Mod, const char *Args);
typedef int (*DATA_PROCESS_WRITE_FUNC)(TProcessingModule *, const char *Data, int len, char *OutBuff, int OutBuffLen);
typedef int (*DATA_PROCESS_CLOSE_FUNC)(TProcessingModule *Mod);

#define DPM_READ_FINAL 1
#define DPM_WRITE_FINAL 2
#define DPM_NOPAD_DATA 4

struct t_dpmod
{
char *Name;
char *Args;
int Flags;
char *Buffer;
int BuffSize;
ListNode *Values;
DATA_PROCESS_INIT_FUNC Init;
DATA_PROCESS_WRITE_FUNC Read;
DATA_PROCESS_WRITE_FUNC Write;
DATA_PROCESS_WRITE_FUNC Flush;
DATA_PROCESS_CLOSE_FUNC Close;
void *Data;
};


TProcessingModule *StandardDataProcessorCreate(const char *Class, const char *Name, const char *Arg);
int DataProcessorInit(TProcessingModule *ProcMod, const char *Key, const char *InputVector);
int DataProcessorWrite(TProcessingModule *ProcMod, const char *InData, int InLen, char *OutData, int OutLen);
void DataProcessorDestroy(void *ProcMod);
char *DataProcessorGetValue(TProcessingModule *M, const char *Name);
void DataProcessorSetValue(TProcessingModule *M, const char *Name, const char *Value);


void InitialiseEncryptionComponents(const char *Args, char **CipherName, char **InputVector, int *InputVectorLen, char **Key, int *KeyLen, int *Flags);


int STREAMAddDataProcessor(STREAM *S, TProcessingModule *Mod, const char *Args);
int DataProcessorAvailable(const char *Class, const char *Name);
int STREAMAddStandardDataProcessor(STREAM *S, const char *Class, const char *Name, const char *Args);
void STREAMClearDataProcessors(STREAM *S);
int STREAMDeleteDataProcessor(STREAM *S, char *Class, char *Name);


#ifdef __cplusplus
}
#endif



#endif
