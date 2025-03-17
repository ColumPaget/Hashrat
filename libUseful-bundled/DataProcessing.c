#include "DataProcessing.h"
#include "SpawnPrograms.h"
#include "Compression.h"
#include "FileSystem.h"
#include "Hash.h"
#include "Entropy.h"
#include "Stream.h"
#include "Encryption.h"





//this function is in Stream.c but we do not want to declare it in Stream.h and expose it to the user, so it's prototyped here
extern int STREAMReadThroughProcessors(STREAM *S, char *Bytes, int InLen);



TProcessingModule *DataProcessorCreate(const char *Name, const char *Args)
{
    TProcessingModule *Mod;

    Mod=(TProcessingModule *) calloc(1,sizeof(TProcessingModule));
    Mod->Args=CopyStr(Mod->Args,Args);
    Mod->Name=CopyStr(Mod->Name,Name);

    return(Mod);
}



void DataProcessorDestroy(void *In)
{
    TProcessingModule *Mod;

    Mod=(TProcessingModule *) In;
    if (! Mod) return;
    if (Mod->Close) Mod->Close(Mod);

    DestroyString(Mod->Name);
    DestroyString(Mod->Args);
    DestroyString(Mod->ReadBuff);
    DestroyString(Mod->WriteBuff);
    free(Mod);
}


static int PipeCommandProcessorInit(TProcessingModule *ProcMod, const char *Args, unsigned char **Header, int *HeadLen)
{
    int result=FALSE;
    char *Tempstr=NULL;
    char *Name=NULL, *Value=NULL, *Cmd=NULL;
    const char *ptr;
    STREAM *S;

    ptr=GetNameValuePair(Args,"\\S","=",&Name,&Value);
    while (ptr)
    {
        if (strcasecmp(Name,"Command")==0) Tempstr=CopyStr(Tempstr,Value);

        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }

    if (StrValid(Tempstr) )
    {
        ptr=GetToken(Tempstr,"\\S",&Name,0);
        Value=FindFileInPath(Value,Name,getenv("PATH"));
        Cmd=MCopyStr(Cmd, Value, " ", ptr, NULL);

//by pipe
        if (StrValid(Cmd) )
        {
            S=STREAMSpawnCommand(Cmd, "");
            STREAMSetTimeout(S, 0);
            ProcMod->Data=(void *) S;
            result=TRUE;
        }
    }

    DestroyString(Cmd);
    DestroyString(Name);
    DestroyString(Value);
    DestroyString(Tempstr);

    return(result);
}


static int PipeCommandProcessorTransact(TProcessingModule *ProcMod, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen, int Flush)
{
    STREAM *S;

    S=(STREAM *) ProcMod->Data;
    if (InLen > 0)
    {
        STREAMWriteBytes(S,InData,InLen);
        STREAMFlush(S);
    }

    if (Flush)
    {
        if (S->out_fd > -1) close(S->out_fd);
        S->out_fd=-1;
    }
    else if (! STREAMCheckForBytes(S)) return(0);
    return(STREAMReadBytes(S,*OutData,*OutLen));
}


static int PipeCommandProcessorClose(TProcessingModule *ProcMod)
{
    STREAMClose((STREAM *) ProcMod->Data);
    ProcMod->Data=NULL;

    return(TRUE);
}



TProcessingModule *PipeCommandProcessorCreate(const char *Name, const char *Args)
{
    TProcessingModule *Mod;

    Mod=DataProcessorCreate(Name, Args);

    Mod->Init=PipeCommandProcessorInit;
    Mod->Read=PipeCommandProcessorTransact;
    Mod->Write=PipeCommandProcessorTransact;
    Mod->Close=PipeCommandProcessorClose;

    return(Mod);
}


char *DataProcessorGetValue(TProcessingModule *M, const char *Name)
{
    ListNode *Curr;

    if (! M->Values) return(NULL);
    Curr=ListFindNamedItem(M->Values,Name);
    if (Curr) return(Curr->Item);
    return(NULL);
}


void DataProcessorSetValue(TProcessingModule *M, const char *Name, const char *Value)
{
    ListNode *Curr;

    if (! M->Values) M->Values=ListCreate();
    Curr=ListFindNamedItem(M->Values,Name);
    if (Curr) Curr->Item = (void *) CopyStr( (char *) Curr->Item, Value);
    else ListAddNamedItem(M->Values,Name,CopyStr(NULL,Value));
}





TProcessingModule *StandardDataProcessorCreate(const char *Class, const char *Name, const char *Args, unsigned char **Header, int *HeadLen)
{
    TProcessingModule *Mod=NULL;

    if (strcasecmp(Class,"crypto")==0) Mod=libCryptoProcessorCreate();
    if (strcasecmp(Class,"compress")==0) Mod=LU_CompressionModuleCreate(Name, Args);

    if (
        (strcasecmp(Class,"uncompress")==0) ||
        (strcasecmp(Class,"decompress")==0)
    ) Mod=LU_DeCompressionModuleCreate(Name, Args);

    if (Mod && Mod->Init && Mod->Init(Mod, Mod->Args, Header, HeadLen)) return(Mod);

    DataProcessorDestroy(Mod);
    return(NULL);
}





int STREAMAddDataProcessor(STREAM *S, TProcessingModule *Mod)
{
    ListNode *Curr;
    char *Tempstr=NULL;
    int len;

    STREAMFlush(S);

    if (! S->ProcessingModules) S->ProcessingModules=ListCreate();
    Tempstr=MCopyStr(Tempstr,Mod->Name,NULL);
    ListAddNamedItem(S->ProcessingModules,Tempstr,Mod);

    Curr=ListGetNext(Mod->Values);
    while (Curr)
    {
        STREAMSetValue(S,Curr->Tag,(char *) Curr->Item);
        Curr=ListGetNext(Curr);
    }

    DestroyString(Tempstr);
    return(TRUE);
}



int STREAMReBuildDataProcessors(STREAM *S)
{
    int len;
    char *Tempstr=NULL;

    len=S->InEnd - S->InStart;
    if (len > 0)
    {
        Tempstr=SetStrLen(Tempstr, len);
        memcpy(Tempstr, S->InputBuff + S->InStart,len);
        S->InStart=0;
        S->InEnd=0;

        STREAMReadThroughProcessors(S, Tempstr, len);
    }

    DestroyString(Tempstr);

    return(TRUE);
}

int STREAMDeleteDataProcessor(STREAM *S, char *Class, char *Name)
{
    ListNode *Curr;
    char *Tempstr=NULL;

    STREAMFlush(S);

    Tempstr=MCopyStr(Tempstr,Class,":",Name,NULL);
    Curr=ListFindNamedItem(S->ProcessingModules,Tempstr);
    ListDeleteNode(Curr);

    DestroyString(Tempstr);
    return(TRUE);
}



int DataProcessorAvailable(const char *Class, const char *Name)
{
    int result=FALSE;
    TProcessingModule *Mod;

    Mod=StandardDataProcessorCreate(Class,Name,"",NULL,NULL);
    if (Mod) result=TRUE;

    DataProcessorDestroy(Mod);
    return(result);
}


int STREAMAddStandardDataProcessor(STREAM *S, const char *Class, const char *iName, const char *iArgs)
{
    TProcessingModule *Mod=NULL;
    char *Args=NULL, *Name=NULL, *Tempstr=NULL;
    unsigned char *Header=NULL;
    int HeadLen=0, RetVal=FALSE, ReadOffset=0;

    Args=CopyStr(Args, iArgs);
    Name=CopyStr(Name, iName);
    if ( (S->Flags & SF_RDONLY) && (strcasecmp(Class, "crypto")==0) )
    {
        if (S->Flags & SF_WRONLY)
        {
            RaiseError(0, "STREAMAddStandardDataProcessor", "attempt to use encryption on a read+write file, only read or write is supported");
            Destroy(Args);
            return(FALSE);
        }

        Header=SetStrLen(Header, 16);
        STREAMReadBytes(S, Header, 16);
        if (strncmp(Header, "Salted__", 8)==0)
        {
            Tempstr=EncodeBytes(Tempstr, Header+8, 8, ENCODE_HEX);
            Args=MCatStr(Args, " encrypt_hexsalt=", Tempstr, NULL);
            ReadOffset=16;
        }

        STREAMSeek(S, ReadOffset, SEEK_SET);
    }


    if ( (S->Flags & SF_RDONLY) &&
            ( (strcasecmp(Class, "uncompress")==0) || (strcasecmp(Class, "decompress")==0) )
       )
    {
        if (strcasecmp(Name, "auto")==0)
        {
            Name=CopyStr(Name, STREAMDetectCompression(S));
            //if we can't detect compression, assume that this file is 
            //uncompressed, and so return TRUE to indicate setup went okay
            if (! StrValid(Name)) RetVal=TRUE;
        }
    }

    Mod=StandardDataProcessorCreate(Class,Name,Args,&Header,&HeadLen);
    if (Mod)
    {
        if ( (S->Flags & SF_WRONLY) && (HeadLen > 0) )
        {
            STREAMWriteBytes(S, Header, HeadLen);
            STREAMFlush(S);
        }
        STREAMAddDataProcessor(S, Mod);
        if (Mod->Flags & DPM_COMPRESS) S->State |= LU_SS_COMPRESSED;
        RetVal=TRUE;
    }

    Destroy(Tempstr);
    Destroy(Header);
    Destroy(Name);
    Destroy(Args);

    return(RetVal);
}




int ProgressProcessorRead(TProcessingModule *ProcMod, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen, int Flush)
{
    ((DATA_PROGRESS_CALLBACK) ProcMod->Data)(InData,InLen,*OutLen);
    return(TRUE);
}

int STREAMAddProgressCallback(STREAM *S, DATA_PROGRESS_CALLBACK Callback)
{
    TProcessingModule *Mod=NULL;

    Mod=(TProcessingModule *) calloc(1,sizeof(TProcessingModule));
    if (Mod)
    {
        Mod->Flags |= DPM_PROGRESS;
        Mod->Data=Callback;
        Mod->Read=ProgressProcessorRead;
        STREAMAddDataProcessor(S, Mod);
        return(TRUE);
    }

    return(FALSE);
}


void STREAMClearDataProcessors(STREAM *S)
{
    STREAMFlush(S);
    STREAMResetInputBuffers(S);
    ListDestroy(S->ProcessingModules, DataProcessorDestroy);
}

