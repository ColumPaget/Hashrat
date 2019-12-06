#include "DataProcessing.h"


#ifdef HAVE_LIBZ

#include <zlib.h>

typedef struct
{
    z_stream z_in;
    z_stream z_out;
} zlibData;
#endif



//Zlib is a little weird. It accepts a pointer to a buffer (next_in) and a buffer length (avail_in) to specify the input
//and another buffer (next_out) and length (avail_out) to write data into. When called it reads bytes from next_in, updates
//next_in to point to the end of what it read, and subtracts the number of bytes it read from avail_in so that avail_in
//now says how many UNUSED bytes there are pointed to by next_in. Similarly it writes to next_out, updating that pointer
//to point to the end of the write, and updating avail_out to say how much room is LEFT usused in the output buffer
//
//However, if zlib doesn't use all avail_in, then you can't mess with that buffer until it has. Hence you can't take the unusued
//data from next_in/avail_in and copy it to a new buffer and pass that buffer into deflate/inflate on the next call. If zlib
//doesn't use all the input the only way to handle it is to grow the output buffer and call inflate/deflate again, so that it
//can write into the expanded buffer until it's used up all input.
//
//Finally, when you've supplied all the input you've got, you have to call deflate with 'Z_FINISH' so that it knows there's no
//more data coming.

int zlibProcessorWrite(TProcessingModule *ProcMod, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen, int Flush)
{
    int wrote=0;
#ifdef HAVE_LIBZ
    int val=0;
    zlibData *ZData;

    if (ProcMod->Flags & DPM_WRITE_FINAL) return(STREAM_CLOSED);
    ZData=(zlibData *) ProcMod->Data;


    ZData->z_out.avail_in=InLen;
    ZData->z_out.next_in=(char *) InData;
    ZData->z_out.avail_out=*OutLen;
    ZData->z_out.next_out=*OutData;

    while ((ZData->z_out.avail_in > 0) || Flush)
    {
        if (Flush) val=deflate(& ZData->z_out, Z_FINISH);
        else val=deflate(& ZData->z_out, Z_NO_FLUSH);

        wrote=*OutLen-ZData->z_out.avail_out;
        if (val==Z_STREAM_END)
        {
            ProcMod->Flags |= DPM_WRITE_FINAL;
            break;
        }

        if ((ZData->z_out.avail_in > 0) || Flush)
        {
            *OutLen+=BUFSIZ;
            *OutData=(char *) realloc(*OutData,*OutLen);
            ZData->z_out.avail_out+=BUFSIZ;
        }

    }



#endif
    return(wrote);
}


int zlibProcessorRead(TProcessingModule *ProcMod, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen, int Flush)
{
    int wrote=0;

#ifdef HAVE_LIBZ
    int result=0;
    zlibData *ZData;

    if (ProcMod->Flags & DPM_READ_FINAL)
    {
        return(STREAM_CLOSED);
    }
    ZData=(zlibData *) ProcMod->Data;


    ZData->z_in.avail_in=InLen;
    ZData->z_in.next_in=(char *) InData;
    ZData->z_in.avail_out=*OutLen;
    ZData->z_in.next_out=*OutData;

    while ((ZData->z_in.avail_in > 0) || Flush)
    {
        if (Flush) result=inflate(& ZData->z_in, Z_FINISH);
        else result=inflate(& ZData->z_in, Z_NO_FLUSH);

        wrote=(*OutLen)-ZData->z_in.avail_out;

        if (result==Z_BUF_ERROR) break;
        switch (result)
        {
        case Z_DATA_ERROR:
            inflateSync(&ZData->z_in);
            break;
        case Z_ERRNO:
            if (Flush) ProcMod->Flags |= DPM_READ_FINAL;
            break;
        case Z_STREAM_ERROR:
        case Z_STREAM_END:
            ProcMod->Flags |= DPM_READ_FINAL;
            break;
        }

        if (ProcMod->Flags & DPM_READ_FINAL) break;
        if ((ZData->z_in.avail_in > 0) || Flush)
        {
            (*OutLen)+=BUFSIZ;
            *OutData=(char *) realloc(*OutData,*OutLen);
            ZData->z_in.next_out=(*OutData) + wrote;
            ZData->z_in.avail_out=(*OutLen) - wrote;
        }

    }

#endif
    return(wrote);
}




int zlibProcessorClose(TProcessingModule *ProcMod)
{
#ifdef HAVE_LIBZ
    zlibData *ZData;

    ZData=(zlibData *) ProcMod->Data;
    if (ZData)
    {
        inflateEnd(&ZData->z_in);
        deflateEnd(&ZData->z_out);

        free(ZData);
        ProcMod->Data=NULL;
    }
#endif
    return(TRUE);
}


#define COMP_ZLIB 0
#define COMP_GZIP 1

int zlibProcessorInit(TProcessingModule *ProcMod, const char *Args)
{
    int result=FALSE;

#ifdef HAVE_LIBZ
    zlibData *ZData;
    int CompressionLevel=5;
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    int Type=COMP_ZLIB;

    ptr=GetNameValuePair(Args,"\\S","=",&Name,&Value);
    while (ptr)
    {
        if (strcasecmp(Name,"Alg")==0)
        {
            if (strcasecmp(Value, "gzip")==0) Type=COMP_GZIP;
        }
        else if (strcasecmp(Name,"CompressionLevel")==0) CompressionLevel=atoi(Value);
        else if (strcasecmp(Name,"Level")==0) CompressionLevel=atoi(Value);
        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }


    ProcMod->ReadMax=4096;
    ProcMod->WriteMax=4096;


    ZData=(zlibData *) calloc(1,sizeof(zlibData));
    ZData->z_in.avail_in=0;
    ZData->z_in.avail_out=0;
    if (Type==COMP_GZIP) result=inflateInit2(&ZData->z_in,47);
    else result=inflateInit(&ZData->z_in);

    ZData->z_out.avail_in=0;
    ZData->z_out.avail_out=0;
    if (Type==COMP_GZIP) deflateInit2(&ZData->z_out,5,Z_DEFLATED,30,8,Z_DEFAULT_STRATEGY);
    else deflateInit(&ZData->z_out,CompressionLevel);

    ProcMod->Data=(void *) ZData;
    result=TRUE;

    ProcMod->Read=zlibProcessorRead;
    ProcMod->Write=zlibProcessorWrite;
    ProcMod->Close=zlibProcessorClose;

    DestroyString(Name);
    DestroyString(Value);

#endif
    return(result);
}




int CompressBytes(char **Out, const char *Alg, const char *In, unsigned long Len, int Level)
{
    TProcessingModule *Mod=NULL;
    char *Tempstr=NULL;
    unsigned long val;
    int result;

    Tempstr=FormatStr(Tempstr,"CompressionLevel=%d",Level);
    Mod=StandardDataProcessorCreate("compress",Alg,Tempstr);
    if (! Mod) return(-1);

    val=Len *2;
    *Out=SetStrLen(*Out,val);
    result=Mod->Write(Mod,In,Len,Out,&val,TRUE);

    DestroyString(Tempstr);
    DataProcessorDestroy(Mod);

    return(result);
}


int DeCompressBytes(char **Out, const char *Alg, const char *In, unsigned long Len)
{
    TProcessingModule *Mod=NULL;
    int result;
    unsigned long val;

    Mod=StandardDataProcessorCreate("decompress",Alg,"");
    if (! Mod) return(-1);

    val=Len *2;
    *Out=SetStrLen(*Out,val);
    result=Mod->Read(Mod,In,Len,Out,&val,TRUE);

    DataProcessorDestroy(Mod);

    return(result);
}

