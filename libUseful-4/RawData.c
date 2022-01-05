#include "RawData.h"
#include "Encodings.h"


int RAWDATAAppend(RAWDATA *RD, const char *Data, const char *Encoding, int MaxBuffLen)
{
    int encode;

    if (StrValid(Data))
    {
        if (StrValid(Encoding))
        {
            encode=EncodingParse(Encoding);
            if (encode < 0) return(0);
            RD->DataLen=DecodeBytes(&RD->Buffer, Data, encode);
            RD->BuffLen=RD->DataLen;
        }
        else
        {
            RD->BuffLen=MaxBuffLen;
            RD->DataLen=MaxBuffLen;
            RD->Buffer=SetStrLen(RD->Buffer,MaxBuffLen);
            memcpy(RD->Buffer, Data, MaxBuffLen);
        }
    }

    return(RD->DataLen);
}


RAWDATA *RAWDATACreate(const char *Data, const char *Encoding, int MaxBuffLen)
{
    RAWDATA *Item;
    int encode;

    Item=(RAWDATA *) calloc(1,sizeof(RAWDATA));
    if (StrValid(Data))
    {
        if (RAWDATAAppend(Item, Data, Encoding, MaxBuffLen) ==0)
        {
            free(Item);
            return(NULL);
        }
    }
    else
    {
        Item->BuffLen=MaxBuffLen;
        Item->Buffer=SetStrLen(Item->Buffer,MaxBuffLen);
    }

    return(Item);
}



void RAWDATADestroy(void *p_RD)
{
    RAWDATA *RD;

    if (! p_RD) return;

    RD=(RAWDATA *) p_RD;
    free(RD->Buffer);
    free(RD);
}


RAWDATA *RAWDATACopy(RAWDATA *RD, size_t offset, size_t len)
{
    if (len==0) len=RD->DataLen;
    if ((offset+len) > RD->BuffLen) len=RD->BuffLen-offset;
    return(RAWDATACreate(RD->Buffer+offset, "", len));
}



int RAWDATAReadAt(RAWDATA *RD, STREAM *S, size_t offset, size_t size)
{
    int len;

    if (size==0) size=S->Size;
    if ((offset+size) > RD->BuffLen)
    {
        RD->BuffLen=offset+size;
        RD->Buffer=SetStrLen(RD->Buffer,RD->BuffLen);
    }

    if (size > RD->BuffLen)
    {
        RD->Buffer=SetStrLen(RD->Buffer, size);
        RD->BuffLen=size;
    }

    len=STREAMReadBytes(S, RD->Buffer+offset, size);
    if (len > 0) offset+=len;
    RD->DataLen=offset;
    return(len);
}



int RAWDATAWrite(RAWDATA *RD, STREAM *S, size_t offset, size_t size)
{

    if (offset > RD->BuffLen) return(FALSE);
    if (size==0) size=RD->DataLen;
    if ((offset+size) > RD->BuffLen) size=RD->DataLen-offset;

    return(STREAMWriteBytes(S, RD->Buffer+offset, size));
}

char *RAWDATAEncode(char *RetStr, RAWDATA *RD, char *Encoding, size_t offset, size_t size)
{
    int encode;

    encode=EncodingParse(Encoding);
    if (encode < 0)
    {
        DestroyString(RetStr);
        return(NULL);
    }

    if (offset > RD->BuffLen) return(NULL);
    if (size==0) size=RD->DataLen;
    if ((offset+size) > RD->BuffLen) size=RD->DataLen-offset;

    return(EncodeBytes(RetStr, RD->Buffer+offset, size, encode));
}


char RAWDATAGetChar(RAWDATA *RD, size_t pos)
{
    if (pos==-1) pos=RD->pos;
    if (pos > RD->BuffLen) return(0);
    RD->pos=pos+1;
    return(RD->Buffer[pos]);
}

int RAWDATASetChar(RAWDATA *RD, size_t pos, char value)
{
    int epos;

    if (pos==-1) pos=RD->pos;
    epos=pos+sizeof(char);
    if (epos > RD->BuffLen) return(FALSE);
    RD->Buffer[pos]=value;
    RD->pos=pos+1;
    if (RD->DataLen <= epos) RD->DataLen=epos+1;
    return(TRUE);
}

int16_t RAWDATAGetInt16(RAWDATA *RD, size_t pos)
{
    if (pos==-1) pos=RD->pos;
    if (pos > RD->BuffLen) return(0);
    RD->pos=pos+sizeof(int16_t);
    return(* (int16_t *) (RD->Buffer+pos));
}

int RAWDATASetInt16(RAWDATA *RD, size_t pos, int16_t value)
{
    int epos;

    if (pos==-1) pos=RD->pos;
    epos=pos+sizeof(int16_t);
    if (epos > RD->BuffLen) return(FALSE);
    RD->pos=pos+sizeof(int16_t);
    *(int16_t *) (RD->Buffer+pos)=value;
    if (RD->DataLen <= epos) RD->DataLen=epos+1;
    return(TRUE);
}

int32_t RAWDATAGetInt32(RAWDATA *RD, size_t pos)
{
    if (pos==-1) pos=RD->pos;
    if (pos > RD->BuffLen) return(FALSE);
    RD->pos=pos+sizeof(int32_t);
    return(* (int32_t *) (RD->Buffer+pos));
}

int RAWDATASetInt32(RAWDATA *RD, size_t pos, int32_t value)
{
    int epos;

    if (pos==-1) pos=RD->pos;
    epos=pos+sizeof(int32_t);
    if (epos > RD->BuffLen) return(FALSE);
    RD->pos=pos+sizeof(int32_t);
    *(int32_t *) (RD->Buffer+pos)=value;
    if (RD->DataLen <= epos) RD->DataLen=epos+1;
    return(TRUE);
}


long RAWDATAFindChar(RAWDATA *RD, size_t offset, char Char)
{
    const char *ptr;

    if (offset==-1) offset=RD->pos;
    if (offset > RD->DataLen) return(-1);
    ptr=memchr(RD->Buffer+offset, Char, RD->DataLen-offset);
    if (! ptr) return(-1);
    return(ptr-RD->Buffer);
}

char *RAWDATACopyStr(char *RetStr, RAWDATA *RD)
{
    const char *ptr, *end;
    int len=0;

    RetStr=CopyStr(RetStr, "");
    end=RD->Buffer+RD->DataLen;
    for (ptr=RD->Buffer + RD->pos; ptr < end; ptr++)
    {
        if (*ptr == '\0') break;
        RetStr=AddCharToBuffer(RetStr, len++, *ptr);
    }

    return(RetStr);
}

char *RAWDATACopyStrLen(char *RetStr, RAWDATA *RD, int max)
{
    const char *ptr, *end;
    int len=0;

    RetStr=CopyStr(RetStr, "");
    ptr=RD->Buffer+RD->pos;
    if (max > RD->DataLen) max=RD->DataLen;
    end=RD->Buffer + max;
    for (ptr=RD->Buffer + RD->pos; ptr < end; ptr++)
    {
        RetStr=AddCharToBuffer(RetStr, len++, *ptr);
    }

    return(RetStr);

}

