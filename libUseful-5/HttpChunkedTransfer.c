
//These functions relate to adding a 'Data processor' to the stream that
//will decode chunked HTTP transfers

#include "HttpChunkedTransfer.h"
#include "Http.h"
#include "DataProcessing.h"

typedef struct
{
    char *Buffer;
    size_t ChunkSize;
    size_t BuffLen;
} THTTPChunk;



static int HTTPChunkedInit(TProcessingModule *Mod, const char *Args)
{
    Mod->Data=(THTTPChunk *) calloc(1, sizeof(THTTPChunk));

    return(TRUE);
}



static int HTTPChunkedRead(TProcessingModule *Mod, const char *InBuff, unsigned long InLen, char **OutBuff, unsigned long *OutLen, int Flush)
{
    size_t len=0, bytes_out=0;
    THTTPChunk *Chunk;
    char *ptr, *vptr, *end;

    Chunk=(THTTPChunk *) Mod->Data;
    if (InLen > 0)
    {
        len=Chunk->BuffLen+InLen;
        Chunk->Buffer=SetStrLen(Chunk->Buffer,len+10);
        ptr=Chunk->Buffer+Chunk->BuffLen;
        memcpy(ptr,InBuff,InLen);
        StrTrunc(Chunk->Buffer, len);
        Chunk->BuffLen=len;
    }
    else len=Chunk->BuffLen;

    end=Chunk->Buffer+Chunk->BuffLen;

    if (Chunk->ChunkSize==0)
    {
        //if chunksize == 0 then read the size of the next chunk

        //if there's nothing in our buffer, and nothing being added, then
        //we've already finished!
        if ((Chunk->BuffLen==0) && (InLen < 1)) return(STREAM_CLOSED);

        vptr=Chunk->Buffer;
        //skip past any leading '\r' or '\n'
        if (*vptr=='\r') vptr++;
        if (*vptr=='\n') vptr++;
        ptr=memchr(vptr,'\n', end-vptr);

        //sometimes people seem to miss off the final '\n', so if we get told there's no more data
        //we should use a '\r' if we've got one
        if ((! ptr) && (InLen < 1))
        {
            ptr=memchr(vptr,'\r',end-vptr);
            if (! ptr) ptr=end;
        }


        if (ptr)
        {
            StrTrunc(Chunk->Buffer, ptr - Chunk->Buffer);
            ptr++;
        }
        else return(0);

        Chunk->ChunkSize=strtol(vptr,NULL,16);
        //if we got chunksize of 0 then we're done, return STREAM_CLOSED
        if (Chunk->ChunkSize==0) return(STREAM_CLOSED);

        Chunk->BuffLen=end - ptr;

        if (Chunk->BuffLen > 0)	memmove(Chunk->Buffer, ptr, Chunk->BuffLen);
        //in case it went negative in the above calcuation
        else Chunk->BuffLen=0;

        //maybe we have a full chunk already? Set len to allow us to use it
        len=Chunk->BuffLen;
    }


//either path we've been through above can result in a full chunk in the buffer
    if ((len >= Chunk->ChunkSize))
    {
        bytes_out=Chunk->ChunkSize;
        //We should really grow OutBuff to take all the data
        //but for the sake of simplicity we'll just use the space
        //supplied
        if (bytes_out > *OutLen) bytes_out=*OutLen;
        memcpy(*OutBuff,Chunk->Buffer,bytes_out);

        ptr=Chunk->Buffer + bytes_out;
        Chunk->BuffLen   -= bytes_out;
        Chunk->ChunkSize -= bytes_out;
        memmove(Chunk->Buffer, ptr, end-ptr);
    }

    if (Chunk->ChunkSize < 0) Chunk->ChunkSize=0;

    return(bytes_out);
}



static int HTTPChunkedClose(TProcessingModule *Mod)
{
    THTTPChunk *Chunk;

    Chunk=(THTTPChunk *) Mod->Data;
    DestroyString(Chunk->Buffer);
    free(Chunk);

    return(TRUE);
}


void HTTPAddChunkedProcessor(STREAM *S)
{
    TProcessingModule *Mod=NULL;

    Mod=(TProcessingModule *) calloc(1,sizeof(TProcessingModule));
    Mod->Name=CopyStr(Mod->Name,"HTTP:Chunked");
    Mod->Init=HTTPChunkedInit;
    Mod->Read=HTTPChunkedRead;
    Mod->Close=HTTPChunkedClose;

    Mod->Init(Mod, "");
    STREAMAddDataProcessor(S,Mod,"");
}

