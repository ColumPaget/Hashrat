#include "includes.h"
#include "GeneralFunctions.h"
#include "DataProcessing.h"
#include "SpawnPrograms.h"
#include "Pty.h"
#include "URL.h"
#include "Expect.h"
#include "Http.h"
#include "Gemini.h"
#include "Ssh.h"
#include "Pty.h"
#include "String.h"
#include "Users.h"
#include "UnitsOfMeasure.h"
#include "FileSystem.h"
#include "WebSocket.h"
#include <sys/file.h>
#include "SecureMem.h"
#include <sys/mman.h>
#include <sys/ioctl.h>



//A difficult function to fit in order
int STREAMReadCharsToBuffer(STREAM *S);


typedef struct
{
    int size;
    int high;
    void *items;
    void *witems;
} TSelectSet;

#ifdef HAVE_POLL

#include <poll.h>
#include <math.h>

static void SelectAddFD(TSelectSet *Set, int type, int fd)
{
    struct pollfd *items;

    Set->size++;
    Set->items=realloc(Set->items, sizeof(struct pollfd) * Set->size);

    items=(struct pollfd *) Set->items;
    items[Set->size-1].fd=fd;
    items[Set->size-1].events=0;
    items[Set->size-1].revents=0;
    if (type & SELECT_READ) items[Set->size-1].events |= POLLIN;
    if (type & SELECT_WRITE) items[Set->size-1].events |= POLLOUT;
}




static int SelectWait(TSelectSet *Set, struct timeval *tv)
{
    long long timeout;
    uint64_t start, diff;
    int result;


    if (tv)
    {
        //convert to millisecs
        timeout=(tv->tv_sec * 1000) + (tv->tv_usec / 1000);
        start=GetTime(TIME_MILLISECS);
    }
    else timeout=-1;

    result=poll((struct pollfd *) Set->items, Set->size, timeout);

    if (tv)
    {
        diff=GetTime(TIME_MILLISECS) - start;
        if (diff > 0)
        {
            timeout-=diff;
            if (timeout > 0)
            {
                tv->tv_sec=(int) (timeout / 1000.0);
                tv->tv_usec=(timeout - (tv->tv_sec * 1000.0)) * 1000;
            }
            else
            {
                tv->tv_sec=0;
                tv->tv_usec=0;
            }
        }
    }

    return(result);
}

static int SelectCheck(TSelectSet *Set, int fd)
{
    int i, RetVal=0;
    struct pollfd *items;

    items=(struct pollfd *) Set->items;
    for (i=0; i < Set->size; i++)
    {
        if (items[i].fd==fd)
        {
            if (items[i].revents & (POLLIN | POLLHUP)) RetVal |= SELECT_READ;
            if (items[i].revents & POLLOUT) RetVal |= SELECT_WRITE;
            break;
        }
    }

    return(RetVal);
}
#else

static void SelectAddFD(TSelectSet *Set, int type, int fd)
{
    if (fd < FD_SETSIZE)
    {
        if (! Set->items) Set->items=calloc(1, sizeof(fd_set));

        if (type & SELECT_WRITE)
        {
            if (! Set->witems) Set->witems=calloc(1, sizeof(fd_set));
        }

        if (type & SELECT_READ) FD_SET(fd, (fd_set *) Set->items);
        if (type & SELECT_WRITE) FD_SET(fd, (fd_set *) Set->witems);
        Set->size++;
        if (fd > Set->high) Set->high=fd;
    }
    else RaiseError(ERRFLAG_ERRNO, "SelectAddFD", "File Descriptor '%d' is higher than FD_SETSIZE limit. Cannot add to select.", fd);
}

static int SelectWait(TSelectSet *Set, struct timeval *tv)
{
    return(select(Set->high+1, Set->items, Set->witems,NULL,tv));
}

static int SelectCheck(TSelectSet *Set, int fd)
{
    int RetVal=0;

    if (Set->items  && FD_ISSET(fd, (fd_set *) Set->items )) RetVal |= SELECT_READ;
    if (Set->witems && FD_ISSET(fd, (fd_set *) Set->witems)) RetVal |= SELECT_WRITE;

    return(RetVal);
}

#endif


static void SelectSetDestroy(TSelectSet *Set)
{
    Destroy(Set->items);
    Destroy(Set->witems);
    Destroy(Set);
}


int FDSelect(int fd, int Flags, struct timeval *tv)
{
    TSelectSet *Set;
    int result, RetVal=0;

    Set=(TSelectSet *) calloc(1,sizeof(TSelectSet));
    SelectAddFD(Set, Flags, fd);
    result=SelectWait(Set, tv);

    if ((result==-1) && (errno==EBADF)) RetVal=0;
    else if (result  > 0) RetVal=SelectCheck(Set, fd);

    SelectSetDestroy(Set);

    return(RetVal);
}


int FDIsWritable(int fd)
{
    struct timeval tv;

    tv.tv_sec=0;
    tv.tv_usec=0;
    if (FDSelect(fd, SELECT_WRITE, &tv) & SELECT_WRITE) return(TRUE);
    return(FALSE);
}



int FDCheckForBytes(int fd)
{
    struct timeval tv;

    tv.tv_sec=0;
    tv.tv_usec=0;
    if (FDSelect(fd, SELECT_READ, &tv) & SELECT_READ) return(TRUE);
    return(FALSE);
}



/*STREAM Functions */

void STREAMSetFlags(STREAM *S, int Set, int UnSet)
{
    int val;

    STREAMFlush(S);
    S->Flags &= ~UnSet;
    S->Flags |= Set;

    //yes, flags are RETURNED, argument ignored
    val=fcntl(S->in_fd,F_GETFL,NULL);

    //Handling nonblock flag involves setting nonblock on or off
    //sadly fcntl does not honor O_CLOEXEC flag, so we have to set
    //that using F_SETFD below
    if (S->Flags & SF_NONBLOCK) val |= O_NONBLOCK;
    else val &= (~O_NONBLOCK);

    //these two sets of flags should be the same
    fcntl(S->in_fd, F_SETFL, val);
    fcntl(S->out_fd, F_SETFL, val);


    //F_GETFD, NOT F_GETFL as above
    val=fcntl(S->in_fd,F_GETFD, NULL);
    //Handling close-on-exec flag
    if (S->Flags & SF_EXEC_INHERIT) val &= (~FD_CLOEXEC);
    else val |= FD_CLOEXEC;

    //NOTE F_SETFD IS NOT SAME AS F_SETFL ABOVE!
    //these two sets of flags should be the same
    fcntl(S->in_fd, F_SETFD, val);
    fcntl(S->out_fd, F_SETFD, val);

#ifdef USE_FSFLAGS
    //immutable and append only flags are a special case as
    //they are not io flags, but permanent file flags so we
    //only touch those if explicitly set in Set or UnSet
    if ((Set | UnSet) & (STREAM_IMMUTABLE | STREAM_APPENDONLY)) FileSystemSetSTREAMFlags(S->out_fd, Set, UnSet);
#endif
}


/*Set timeout for select calls within STREAM*/
void STREAMSetTimeout(STREAM *S, int val)
{
    S->Timeout=val;
}


/*Set flush type for STREAM*/
void STREAMSetFlushType(STREAM *S, int Type, int StartPoint, int BlockSize)
{
    S->Flags &= ~(FLUSH_ALWAYS | FLUSH_LINE | FLUSH_BLOCK | FLUSH_BUFFER);
    S->Flags |= Type;
    S->StartPoint=StartPoint;
    S->BlockSize=BlockSize;
}


/* This reads chunks from a file and when if finds a newline it resets */
/* the file pointer to that position */
void STREAMReAllocBuffer(STREAM *S, int size, int Flags)
{
    unsigned char *ibuf=NULL, *obuf=NULL;
    int RW;

    if (S->Flags & SF_MMAP) return;

    if ((Flags & SF_SECURE) && (! (Flags & SF_SECURE)))
    {
        ibuf=S->InputBuff;
        obuf=S->OutputBuff;
        S->InputBuff=NULL;
        S->OutputBuff=NULL;
    }


    // extract the 'readonly' and 'writeonly' flags
    RW=S->Flags & (SF_WRONLY | SF_RDONLY);

    //for HTTP 'readonly' and 'writeonly' have a different meaning, they mean
    //'get' and 'post'. HTTP is always bidirectional
    if (S->Type == STREAM_TYPE_HTTP) RW=0;

    if (S->Flags & SF_SECURE)
    {
        if (! (RW & SF_WRONLY)) SecureRealloc(&S->InputBuff, S->BuffSize, size, SMEM_SECURE);
        if (! (RW & SF_RDONLY)) SecureRealloc(&S->OutputBuff, S->BuffSize, size, SMEM_SECURE);
    }
    else
    {
        if (! (RW & SF_WRONLY)) S->InputBuff =(unsigned char *) realloc(S->InputBuff,size);
        if (! (RW & SF_RDONLY)) S->OutputBuff=(unsigned char *) realloc(S->OutputBuff,size);
    }

    if (ibuf)
    {
        S->InputBuff=memcpy(S->InputBuff,ibuf+S->InStart,S->InEnd-S->InStart);
        S->InEnd-=S->InStart;
        S->InStart=0;

        S->OutputBuff=memcpy(S->OutputBuff,obuf,S->OutEnd);

        free(ibuf);
        free(obuf);
    }

    S->BuffSize=size;

    if (S->InStart > S->BuffSize) S->InStart=0;
    if (S->InEnd > S->BuffSize) S->InEnd=0;
    if (S->OutEnd > S->BuffSize) S->OutEnd=0;
}



int STREAMCheckForBytes(STREAM *S)
{
    off_t pos;
    struct stat Stat;

    if (! S) return(FALSE);
    if (S->State & LU_SS_EMBARGOED) return(FALSE);
    if (S->InEnd > S->InStart) return(TRUE);
    if (S->in_fd==-1) return(FALSE);

    if ((S->Type == STREAM_TYPE_FILE) && (S->Flags & SF_FOLLOW))
    {
        while (1)
        {
            pos=STREAMTell(S);
            fstat(S->in_fd,&Stat);
            if (Stat.st_size > pos) return(TRUE);
        }
    }
    return(FDCheckForBytes(S->in_fd));
}



int STREAMCountWaitingBytes(STREAM *S)
{
    int read_result=0, result;

    if (! S) return(0);
    if (S->State & LU_SS_EMBARGOED) return(0);

    result=FDCheckForBytes(S->in_fd);
    if (result > 0) read_result=STREAMReadCharsToBuffer(S);
    else if (result < 0) read_result=STREAM_CLOSED;
    result=S->InEnd - S->InStart;

    if (result > 0) return(result);
    if (read_result==STREAM_CLOSED) return(STREAM_CLOSED);
    if (read_result==STREAM_DATA_ERROR) return(STREAM_CLOSED);
    return(0);
}






STREAM *STREAMSelect(ListNode *Streams, struct timeval *tv)
{
    TSelectSet *Set;
    STREAM *S;
    ListNode *Curr, *Last;
    int result;

    Set=(TSelectSet *) calloc(1,sizeof(TSelectSet));
    Curr=ListGetNext(Streams);
    while (Curr)
    {
        S=(STREAM *) Curr->Item;
        if (S && (! (S->State & LU_SS_EMBARGOED)))
        {
            //server type streams don't have buffers
            if ( (S->Type != STREAM_TYPE_UNIX_SERVER) && (S->Type != STREAM_TYPE_TCP_SERVER) )
            {
                //Pump any data in the stream
                STREAMFlush(S);

                //if there's stuff in buffer, then we don't need to select the file descriptor
                if (S->InEnd > S->InStart)
                {
                    SelectSetDestroy(Set);
                    return(S);
                }
            }

            SelectAddFD(Set, SELECT_READ, S->in_fd);
        }

        Curr=ListGetNext(Curr);
    }

    result=SelectWait(Set, tv);

    if (result > 0)
    {
        Curr=ListGetNext(Streams);
        while (Curr)
        {
            S=(STREAM *) Curr->Item;
            if (S && SelectCheck(Set, S->in_fd))
            {
                //this stream has had it's turn, move it to the bottom of the list
                //so it can't lock others out
                if (Curr->Next !=NULL)
                {
                    ListUnThreadNode(Curr);
                    Last=ListGetLast(Streams);
                    if (! Last) Last=Streams;
                    ListThreadNode(Last, Curr);
                }
                SelectSetDestroy(Set);
                return(S);
            }
            Curr=ListGetNext(Curr);
        }
    }

    SelectSetDestroy(Set);
    return(NULL);
}



int STREAMCheckForWaitingChar(STREAM *S,unsigned char check_char)
{
    int read_result=0, result;
    char *found_char;

    if (! S) return(0);
    if (S->State & LU_SS_EMBARGOED) return(0);

    result=FDCheckForBytes(S->in_fd);
    if (result > 0) read_result=STREAMReadCharsToBuffer(S);
    else if (result < 0) read_result=STREAM_CLOSED;

    if (S->InStart < S->InEnd)
    {
        found_char=memchr(S->InputBuff + S->InStart,check_char,S->InEnd - S->InStart);
        if (found_char > 0) return(TRUE);
    }

    if (read_result==STREAM_CLOSED) return(STREAM_CLOSED);
    if (read_result==STREAM_DATA_ERROR) return(STREAM_CLOSED);
    return(FALSE);
}



//this is the function that finally pushes bytes onto the wire for a basic file-descriptor
static int STREAMBasicSendBytes(STREAM *S, const char *Data, int DataLen)
{
    int result;
    fd_set selectset;
    struct timeval tv;

    //if the OS defines the PIPE_BUF variable that indicates how much data can
    //be written to a pipe in one read, and we are using a pipe, then we want to
    //limit our reads to that much
#ifdef PIPE_BUF
    if (S->Type == STREAM_TYPE_PIPE)
    {
        if (DataLen > PIPE_BUF) DataLen=PIPE_BUF;
    }
#endif

    //wait for fd to become writeable
    if (S->Timeout > 0)
    {
        FD_ZERO(&selectset);
        FD_SET(S->out_fd, &selectset);
        MillisecsToTV(S->Timeout * 10, &tv);
        result=select(S->out_fd+1,NULL,&selectset,NULL,&tv);
        if (result < 1)
        {
            if ((result == 0) || (errno==EAGAIN)) return(STREAM_TIMEOUT);
            else return(STREAM_CLOSED);
        }
    }

    //if we lock on write, do the lock
    if (S->Flags & SF_WRLOCK) flock(S->out_fd,LOCK_EX);

    //do the actual write! Whoohoo!
    result=write(S->out_fd, Data, DataLen);
    if (result < 0)
    {
        if (errno==EINTR) result=0;
        else if (errno==EAGAIN) result=STREAM_TIMEOUT;
        else result=STREAM_CLOSED;
    }

    //if we lock on write, do the unlock
    if (S->Flags & SF_WRLOCK) flock(S->out_fd,LOCK_UN);

    //yes, we neglect to do a sync. The idea here is to work opportunisitically, flushing out those pages
    //that have been written. We do a sync and another fadvise in STREAMClose
#ifdef POSIX_FADV_DONTNEED
    if (S->Flags & SF_NOCACHE) posix_fadvise(S->out_fd, 0,0,POSIX_FADV_DONTNEED);
#endif

    return(result);
}


//this is not static, because it is the entry point for protocols like websocket
//and is called in Websocket.c
int STREAMPushBytes(STREAM *S, const char *Data, int DataLen)
{

    if (S->State & LU_SS_SSL) return(OpenSSLSTREAMWriteBytes(S, Data, DataLen));
    return(STREAMBasicSendBytes(S, Data, DataLen));
}


static int STREAMInternalPushBytes(STREAM *S, const char *Data, int DataLen)
{
    int result=0, count=0;

    if (! S) return(STREAM_CLOSED);
    if (S->out_fd==-1) return(STREAM_CLOSED);

//if we are flushing blocks, then pad out to the blocksize
    if (S->Flags & FLUSH_BLOCK)
    {
        DataLen=S->BlockSize;
        if (DataLen > S->OutEnd) DataLen=S->OutEnd;

        /*
         	if (DataLen < S->BlockSize)
        	{
        	if (DataLen==0) DataLen=S->BlockSize;
        	else
        	{
        	len=(DataLen / S->BlockSize) * S->BlockSize;
        	if (S->OutEnd > len) len+=S->BlockSize;
        	memset(S->OutputBuff+S->OutEnd,0,len - S->OutEnd);
        	DataLen=len;
        	}
        	}
        */
    }

    while (count < DataLen)
    {
        switch (S->Type)
        {
        case STREAM_TYPE_WS:
        case STREAM_TYPE_WSS:
        case STREAM_TYPE_WS_SERVICE:
            result=WebSocketSendBytes(S, Data+count, DataLen-count);
            break;

        default:
            result=STREAMPushBytes(S, Data+count, DataLen-count);
            break;
        }
        if (result < 0) break;
        count+=result;
        if (S->Flags & SF_NONBLOCK) break;
    }

    S->BytesWritten+=count;
    //memmove any remaining data so that we add onto the end of it
    S->OutEnd -= count;
    if (S->OutEnd > 0) memmove(S->OutputBuff,S->OutputBuff+count, S->OutEnd);

    if (result < 0) return(result);

    return(count);
}




int STREAMFlush(STREAM *S)
{
    int val;

    val=STREAMInternalPushBytes(S, S->OutputBuff, S->OutEnd);

    if (S->Flags & SF_FULL_FLUSH)
    {
        switch (S->Type)
        {
        case STREAM_TYPE_FILE:
        case STREAM_TYPE_PIPE:
        case STREAM_TYPE_UNIX:
        case STREAM_TYPE_UNIX_DGRAM:
            fsync(S->out_fd);
            break;


        case STREAM_TYPE_TCP:
        case STREAM_TYPE_SSL:
        case STREAM_TYPE_HTTP:
        case STREAM_TYPE_CHUNKED_HTTP:
        case STREAM_TYPE_TPROXY:
        case STREAM_TYPE_WS:
        case STREAM_TYPE_WSS:
            //turning off Nagle's algorithm forces a flush of the buffer
            SockSetOptions(S->out_fd, SOCK_TCP_NODELAY, 1);
            //turn Nagle's back on, so we don't send partial data when we don't intend it
            SockSetOptions(S->out_fd, SOCK_TCP_NODELAY, 0);
            break;
        }
    }

    //if nothing left in stream (There shouldn't be) then wipe data because
    //there might have been passwords sent on the stream, and we don't want
    //that hanging about in memory
    if (S->OutputBuff && (S->OutEnd==0)) xmemset(S->OutputBuff, 0, S->BuffSize);
    return(val);
}

void STREAMClear(STREAM *S)
{
    STREAMFlush(S);
    S->InStart=0;
    S->InEnd=0;
    //clear input buffer, because anything might be hanging about in there and
    //we don't want sensitive data persisiting in memory
    if (S->InputBuff) xmemset(S->InputBuff, 0, S->BuffSize);
}


/*A stream can have a series of 'processor modules' associated with it' */
/*which do things to the data before it is read/written. This function  */
/*pumps the data through the processor list, and eventually writes it out */
int STREAMReadThroughProcessors(STREAM *S, char *Bytes, long InLen)
{
    TProcessingModule *Mod;
    ListNode *Curr;
    char *InBuff=NULL, *OutputBuff=NULL;
    char *p_Input;
    int state=0, result;
    unsigned long olen=0, len=0;


    p_Input=Bytes;
    Curr=ListGetNext(S->ProcessingModules);

    if (InLen < 0) state=InLen;
    else len=InLen;

    //this is used if InLen==-1 to trigger flush of modules. After first use it becomes the exit values of the
    //previous module
    result=InLen;

    while (Curr)
    {
        Mod=(TProcessingModule *) Curr->Item;
        if (len < BUFSIZ) olen=BUFSIZ;
        else olen=len * 8;

        if (Mod->Read)
        {
            //A 'DPM_PROGRESS' module doesn't do input output, it is purely used to
            //server a callback function that can output the write progress of a stream
            if (Mod->Flags & DPM_PROGRESS) Mod->Read(Mod,S->Path,S->BytesRead,NULL,&S->Size,0);
            else
            {
                OutputBuff=(char *) realloc(OutputBuff, olen);
                //if InLen < 0 then we are requesting a flush

                if (InLen < 0) result=Mod->Read(Mod, p_Input, len, &OutputBuff, &olen,  TRUE);
                else result=Mod->Read(Mod, p_Input, len, &OutputBuff, &olen,  FALSE);

                if (result > 0) state=0;


                if (result > 0)
                {
                    len=result;
                    InBuff=(char *) realloc(InBuff, len + 8);
                    memcpy(InBuff, OutputBuff, len);
                    p_Input=InBuff;
                }
                else len=0;
            }
        }

        Curr=ListGetNext(Curr);
    }

    if (
        (! (S->State & LU_SS_DATA_ERROR)) &&
        (len > 0)
    )
    {
//Whatever happened above, p_Input should now contain the data to be written!
//note that we resize buff to S->InEnd + len, where len is length of the new
//data. Even if S->InStart > 0 (meaning there are 'sent' bytes in the buffer)
//we consider S->InStart to be 0 as regards sizeing the buffer, because those
//sent bytes are still there.
        if ((S->InEnd + len) > S->BuffSize) STREAMResizeBuffer(S, S->InEnd + len);
        memcpy(S->InputBuff + S->InEnd, p_Input, len);
        S->InEnd+=len;
    }


    Destroy(OutputBuff);
    Destroy(InBuff);

    if (len==0)
    {
        //if state is STREAM_CLOSED or STREAM_MESSAGE_END then return that
        if (state < 0) return(state);
        if (S->State & LU_SS_DATA_ERROR) return(STREAM_DATA_ERROR);
    }

    // this indicates that there's still data in the processing chain
    return(1);
}





int STREAMLock(STREAM *S, int val)
{
    int result;

    result=flock(S->in_fd,val);

    if (result==0) return(TRUE);
    return(FALSE);
}


STREAM *STREAMCreate()
{
    STREAM *S;
    const char *ptr;

    S=(STREAM *) calloc(1,sizeof(STREAM));
    STREAMResizeBuffer(S,BUFSIZ);
    S->in_fd=-1;
    S->out_fd=-1;
    S->Timeout=3000;

    ptr=LibUsefulGetValue("STREAM:Timeout");
    if (StrValid(ptr)) STREAMSetTimeout(S, atoi(ptr));

    S->Flags |= FLUSH_ALWAYS;

    return(S);
}

STREAM *STREAMFromFD(int fd)
{
    STREAM *Stream;

    if (fd==-1) return(NULL);

    Stream=STREAMCreate();
    Stream->in_fd=fd;
    Stream->out_fd=fd;
    return(Stream);
}


STREAM *STREAMFromDualFD(int in_fd, int out_fd)
{
    STREAM *Stream;

    if ((in_fd==-1) && (out_fd==-1)) return(NULL);
    //if (out_fd==-1) return(NULL);

    Stream=STREAMCreate();
    Stream->in_fd=in_fd;
    Stream->out_fd=out_fd;
    return(Stream);
}



int STREAMOpenMMap(STREAM *S, int offset, int len, int Flags)
{
    unsigned char *ptr;
    int MProt=PROT_READ;

    if (S->InputBuff) free(S->InputBuff);
    if (Flags & (SF_WRONLY | SF_RDWR)) MProt |= PROT_WRITE;
    ptr=(unsigned char *) mmap(0, len, MProt, MAP_SHARED, S->in_fd, offset);
    if (ptr==MAP_FAILED) return(FALSE);
    S->InEnd=len;
    S->InStart=0;
    S->Flags |= SF_MMAP;
    S->InputBuff=ptr;
    if (Flags & SF_SECURE) mlock(ptr, len);

    return(TRUE);
}



static int STREAMAutoRecoverRequired(int Flags)
{
    if (
        (Flags & SF_AUTORECOVER) &&
        (Flags & SF_WRONLY) &&
        (! (Flags & (STREAM_APPEND | SF_RDONLY) ) )
    ) return(TRUE);

    return(FALSE);
}


void STREAMFileAutoRecover(const char *Path, int Flags)
{
    char *BackupPath=NULL;
    struct stat FStat;
    int result;

    BackupPath=MCopyStr(BackupPath, Path, ".autorecover", NULL);
    result=stat(BackupPath, &FStat);
    if (result==0)
    {
        //never use an autorecover that is zero bytes in size
        if (FStat.st_size == 0)
        {
            result=unlink(BackupPath);
            if (result != 0) RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "STREAMFileOpen", "zero-length autorecovery found, but cannot remove it %s", BackupPath);
        }
        // if we are writing to the file, and we are not appending to it, then we are going to blank the file down anyways, so no need to autorecover
        else if (! (Flags & (SF_RDONLY|STREAM_APPEND)) )
        {
            result=unlink(BackupPath);
            if (result != 0) RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "STREAMFileOpen", "autorecovery found, not needed, but cannot remove it %s", BackupPath);
        }
        else
        {
            RaiseError(ERRFLAG_DEBUG, "STREAMFileOpen", "autorecovery from %s", BackupPath);
            //first take a backup for the current 'live' file
            BackupPath=MCopyStr(BackupPath, Path, ".", GetDateStr("%Y-%m-%dT%H%M%S", NULL), ".error", NULL);
            result=rename(Path, BackupPath);
            if (result != 0) RaiseError(ERRFLAG_ERRNO | ERRFLAG_DEBUG, "STREAMFileOpen", "autorecovery cannot archive current file to %s", BackupPath);

            BackupPath=MCopyStr(BackupPath, Path, ".autorecover", NULL);
            result=rename(BackupPath, Path);
            if (result != 0) RaiseError(ERRFLAG_ERRNO | ERRFLAG_DEBUG, "STREAMFileOpen", "autorecovery cannot import %s", BackupPath);
        }
    }

    Destroy(BackupPath);
}


// do a bunch of preparation before opening the file
// firstly convert paths starting in ~/ to point to the user's home directory
// secondly handle taking/making a backup if that is requested
// This function always makes a copy of path, because even if no substitutions are
// required functions like mkostemp want to be able to change the file path
// and need a writeable copy to do that
static char *STREAMFileOpenPrepare(char *NewPath, const char *Path, int Flags)
{
    char *BackupPath=NULL;
    int result;

    //if path starts with a tilde, then it's the user's home directory
    if (strncmp(Path, "~/", 2) ==0)
    {
        //Path+1 so we get the / to make sure there is one after HomeDir
        NewPath=MCopyStr(NewPath, GetCurrUserHomeDir(), Path+1, NULL);
    }
    else NewPath=CopyStr(NewPath, Path);

    if (Flags & SF_AUTORECOVER)
    {
        //only take a backup if we are in write/truncate mode, not append or r/w
        if ( STREAMAutoRecoverRequired(Flags) )
        {
            BackupPath=MCopyStr(BackupPath, NewPath, ".autorecover", NULL);
            result=rename(NewPath, BackupPath);
            if (result != 0) RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "STREAMFileOpen", "failed to take backup of %s to %s", NewPath, BackupPath);
        }
        //if stream opened for read or append, then autorecover
        else STREAMFileAutoRecover(NewPath, Flags);
    }

    Destroy(BackupPath);

    return(NewPath);
}


STREAM *STREAMFileOpen(const char *Path, int Flags)
{
    int fd, Mode=0;
    STREAM *Stream;
    struct stat myStat;
    char *Tempstr=NULL, *NewPath=NULL;

    if (Flags & SF_WRONLY) Mode=O_WRONLY;
    else if (Flags & SF_RDONLY) Mode=O_RDONLY;
    else Mode=O_RDWR;

    if (Flags & STREAM_APPEND) Mode |=O_APPEND;
    if (Flags & SF_CREATE) Mode |=O_CREAT;
    if (Flags & SF_EXCL) Mode |=O_EXCL;

    //we take a copy of Path, because we may need to substitute ~/ for the user's home directory
    //or because functions like mkostemp want to modify the string they are passed
    NewPath=STREAMFileOpenPrepare(NewPath, Path, Flags);

    // '-' means stdin or stdout, depending on context
    if (CompareStr(NewPath,"-")==0)
    {
        if (Mode==O_RDONLY) fd=0;
        else fd=1;
    }
    // create a file with a tempoary name, the pattern 'XXXXXX' will be replaced with a unique
    // random string to create a unique filename
    else if (Flags & SF_TMPNAME)
    {
#ifdef HAVE_MKOSTEMP
        fd=mkostemp(NewPath, Mode);
#else
        fd=mkstemp(NewPath);
#endif
    }
    // otherwise just open the file as normal!
    else fd=open(NewPath, Mode, 0600);


    if (fd==-1)
    {
        if (Flags & SF_ERROR) RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "failed to open %s", NewPath);
        else RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "STREAMFileOpen", "failed to open %s", NewPath);
        Destroy(NewPath);
        return(NULL);
    }

    if (Flags & SF_WRLOCK)
    {
        if (flock(fd,LOCK_EX | LOCK_NB)==-1)
        {
            RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "file lock requested but failed %s", NewPath);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }
    }

    if (Flags & SF_RDLOCK)
    {
        if (flock(fd,LOCK_SH | LOCK_NB)==-1)
        {
            RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "file lock requested but failed %s", NewPath);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }
    }


    // check for symlink naughtyness. Basically a malicious user can
    // try to guess the name of the file we are going to open in order
    // to get us to write somewhere other than intended.


    if ((Mode != O_RDONLY) && (! (Flags & SF_FOLLOW)))
    {
        if (lstat(NewPath, &myStat) !=0)
        {
            RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "cannot stat %s", NewPath);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }

        if (S_ISLNK(myStat.st_mode))
        {
            RaiseError(0, "STREAMFileOpen", "%s is a symlink, but not not in 'symlink okay' mode", NewPath);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }
    }
    else
    {
        stat(NewPath, &myStat);
    }

    //CREATE THE STREAM OBJECT !!
    Stream=STREAMFromFD(fd);


    STREAMSetFlushType(Stream,FLUSH_FULL,0,0);
    Tempstr=FormatStr(Tempstr,"%d",myStat.st_size);
    STREAMSetValue(Stream, "FileSize", Tempstr);
    Stream->Size=myStat.st_size;
    Stream->Path=CopyStr(Stream->Path, NewPath);

    if ( (Flags & (SF_RDONLY | SF_MMAP)) == (SF_RDONLY | SF_MMAP) ) STREAMOpenMMap(Stream, 0, myStat.st_size, Flags);
    else
    {
        if (Flags & SF_TRUNC)
        {
            if (ftruncate(fd,0) != 0) RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "cannot ftruncate %s", NewPath);
            STREAMSetValue(Stream, "FileSize", "0");
        }
        if (Flags & STREAM_APPEND) lseek(fd, 0, SEEK_END);
    }

    STREAMSetFlags(Stream, Flags, 0);

    Destroy(Tempstr);
    Destroy(NewPath);

    return(Stream);
}





static int STREAMParseConfig(const char *Config)
{
    int Flags=SF_RDWR;
    const char *ptr;

    if (StrValid(Config))
    {
        //first read fopen-style 'open' flags like 'rw'
        ptr=Config;
        while (*ptr != '\0')
        {
            //any space indicates end of open flags
            if (isspace(*ptr)) break;


            switch (*ptr)
            {
            //these four are the most basic open flags, and can have meaning in ssh, http and file streams
            case 'r':
                if (Flags & SF_WRONLY) Flags &= ~(SF_RDONLY | SF_WRONLY);
                else Flags |= SF_RDONLY;
                break;
            case 'w':  //means 'POST method' for HTTP
                if (Flags & SF_RDONLY) Flags &= ~(SF_RDONLY | SF_WRONLY);
                else Flags |= SF_WRONLY | SF_CREATE| SF_TRUNC;
                break;
            //case 'W': //means 'PUT method' for HTTP
            case 'a':
                Flags |= STREAM_APPEND | SF_CREATE;
                break;
            case 'c':
                Flags |= SF_CREATE;
                break;

            case 'A': //is mapped to 'SOCK_TLS_AUTO' in Socket.c, as STREAM_APPENDONLY has no meaning for sockets and is ignored
                Flags |= STREAM_APPENDONLY;
                break;
            //case 'B': //is mapped to 'SOCK_BROADCAST' in Socket.c
            //case 'D': //is mapped to 'DELETE method' in http
            case 'e':
                Flags |= SF_ENCRYPT;
                break;
            case 'E':
                Flags |= SF_ERROR;
                break;
            case 'f':
                Flags |= SF_FULL_FLUSH;
                break;
            case 'F':  //is mapped to 'SOCK_TCP_FASTOPEN' in Socket.c as 'SF_FOLLOW' has no meaning for sockets and is ignored
                Flags |= SF_FOLLOW;
                break;
            //case 'k':  //is mapped to 'keep-alive' in Socket.c
            //case 'H':  //is mapped to 'HEAD method' in Http.c
            case 'l':
                Flags |= SF_RDLOCK;
                break;
            case 'L':
                Flags |= SF_WRLOCK;
                break;
            case 'm':
                Flags |= SF_MMAP;
                break;
            case 'n':
                Flags |= SF_NONBLOCK;
                break;
            //case 'P':  //is mapped to 'SOCK_REUSEPORT' in Socket.c, and 'PATCH method' in Http.c
            //case 'N':  //is mapped to 'SOC_TCP_NODELAY' in Socket.c
            case 's':
                Flags |= SF_SECURE;
                break;
            case 'i':
                Flags |= SF_EXEC_INHERIT;
                break;
            case 'I':
                Flags |= STREAM_IMMUTABLE;
                break;
            case 'R': //is mapped to 'SOCK_DONTROUTE' in Socket.c as 'SF_AUTORECOVER' has no meaning for sockets and is ignored
                Flags |= SF_AUTORECOVER;
                break;
            case 'S':
                Flags |= SF_SORTED;
                break;
            case 't':
                Flags |= SF_TMPNAME;
                break;
            case 'x':
                //for local files this is 'exclusive open' with O_EXCL.
                //for ssh connections this is the 'execute' flag that indicates
                //a command is to be run
                Flags |= SF_EXCL;
                break;
            case 'z':
                Flags |= SF_COMPRESSED;
                break;
            case '+':
                Flags &= ~(SF_RDONLY | SF_WRONLY);
                break;
            }
            ptr++;
        }
    }

    return(Flags);
}



//this function handles the situation where we have a 'chain' of URLs (network proxies)
//it extracts the 'real' or 'master' URL that specifies the actual connection, which
//will be the LAST one in the list
static const char *STREAMExtractMasterURL(const char *URL)
{
    const char *ptr;

    if (strncmp(URL, "cmd:",4) ==0) return(URL); //'cmd:' urls do not go through proxies!
    if (strncmp(URL, "file:",5) ==0) return(URL); //'file:' urls do not go through proxies!
    if (strncmp(URL, "mmap:",5) ==0) return(URL); //'mmap:' urls do not go through proxies!
    if (strncmp(URL, "stdin:",6) ==0) return(URL); //'stdin:' urls do not go through proxies!
    if (strncmp(URL, "stdout:",7) ==0) return(URL); //'stdout:' urls do not go through proxies!
    if (strncmp(URL, "stdio:",6) ==0) return(URL); //'stdio:' urls do not go through proxies!

    ptr=strrchr(URL, '|');
    if (ptr) ptr++;
    else ptr=URL;

    return(ptr);
}


//this is only used internally at current, users would call 'STREAMOpen', which itself then calls this
//this is so that 'STREAMOpen' can keep different STREAM types with diffferent Config's seperate
#define STREAMFileOpenWithConfig(url, config) STREAMFileOpen((url), STREAMParseConfig(config))



//Add Processor Modules. if any of these fail, then close the stream and return NULL
//so that we don't for instance, write to a file without encryption when that was asked for
static STREAM *STREAMSetupDataProcessorModules(STREAM *S, const char *Config)
{
//if no processors are needed, then we consider things 'good'
    int SetupGood=TRUE;

    if (S->Flags & SF_COMPRESSED)
    {
        // do not remove { braces in the below, otherwise you'll add the 'else if' to the
        // internal if of the first block. All hail astyle for helping me find this issue
        if (S->Flags & SF_RDONLY)
        {
            if (! STREAMAddStandardDataProcessor(S, "decompress", "auto", "")) SetupGood=FALSE;
        }
        else if (S->Flags & SF_WRONLY)
        {
            if (! STREAMAddStandardDataProcessor(S, "compress", "gzip", "")) SetupGood=FALSE;
        }
    }

    if (S->Flags & SF_ENCRYPT) if (! STREAMAddStandardDataProcessor(S, "crypto", "", Config)) SetupGood=FALSE;

    if (! SetupGood)
    {
        STREAMClose(S);
        S=NULL;
    }

    return(S);
}


//URL can be a file path or a number of different network/file URL types
STREAM *STREAMOpen(const char *URL, const char *Config)
{
    STREAM *S=NULL;
    char *Proto=NULL, *Host=NULL, *Token=NULL, *User=NULL, *Pass=NULL, *Path=NULL, *Args=NULL;
    const char *ptr;
    int Port=0;

    if (! StrValid(URL)) return(NULL);
    Proto=CopyStr(Proto,"");
    ptr=STREAMExtractMasterURL(URL);
    ParseURL(ptr, &Proto, &Host, &Token, &User, &Pass, &Path, &Args);
    if (StrValid(Token)) Port=strtoul(Token,NULL,10);

    switch (*Proto)
    {
    case 'c':
        if (strcasecmp(Proto,"cmd")==0) S=STREAMSpawnCommand(URL+4, Config);
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;

    case 'f':
        if (strcasecmp(Proto,"file")==0)
        {
            ptr=URL+5;

            //file protocol can have 3 '/' after file, like this file:///myfile.txt. So we strip off two of these
            //thus anything with 3 of them is a full path from /, anything with less than that is a relative path
            //from the current directory
            if (*ptr=='/') ptr++;
            if (*ptr=='/') ptr++;
            S=STREAMFileOpenWithConfig(URL, Config);
        }
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;

    case 'g':
        if (strcasecmp(Proto, "gemini")==0) S=GeminiOpen(URL, Config);
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;

    case 'h':
        if (
            (strcasecmp(Proto,"http")==0) ||
            (strcasecmp(Proto,"https")==0)
        ) S=HTTPWithConfig(URL, Config);
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;

    case 'm':
        if (strcasecmp(Proto,"mmap")==0) S=STREAMFileOpen(URL+5, STREAMParseConfig(Config) | SF_MMAP);
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;

    case 't':
    case 's':
    case 'u':
    case 'b': //b for 'bcast'
        if ( (CompareStr(URL,"-")==0) || (strcasecmp(URL,"stdio:")==0) ) S=STREAMFromDualFD(dup(0), dup(1));
        else if (strcasecmp(URL,"stdin:")==0) S=STREAMFromFD(dup(0));
        else if (strcasecmp(URL,"stdout:")==0) S=STREAMFromFD(dup(1));
        else if (strcasecmp(Proto,"ssh")==0) S=SSHOpen(Host, Port, User, Pass, Path, Config);
        else if (strcasecmp(Proto,"tty")==0)
        {
            S=STREAMFromFD(TTYConfigOpen(URL+4, Config));
            if (S)
            {
                S->Path=CopyStr(S->Path,URL);
                S->Type=STREAM_TYPE_TTY;
            }
        }
        else
        {
            S=STREAMCreate();
            S->Path=CopyStr(S->Path,URL);
            if (! STREAMConnect(S, URL, Config))
            {
                STREAMClose(S);
                S=NULL;
            }
        }
        break;


    case 'w':
        if (strcasecmp(Proto,"wss")==0) S=WebSocketOpen(URL, Config);
        else if (strcasecmp(Proto,"ws")==0) S=WebSocketOpen(URL, Config);
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;


    default:
        if (CompareStr(URL,"-")==0) S=STREAMFromDualFD(dup(0),dup(1));
        else S=STREAMFileOpenWithConfig(URL, Config);
        break;
    }


    if (S) S=STREAMSetupDataProcessorModules(S, Config);



    if (S)
    {
        if (S->Flags & SF_SECURE) STREAMResizeBuffer(S, S->BuffSize);


        switch (S->Type)
        {
        case STREAM_TYPE_TCP:
        case STREAM_TYPE_UDP:
        case STREAM_TYPE_SSL:
        case STREAM_TYPE_HTTP:
        case STREAM_TYPE_CHUNKED_HTTP:
        case STREAM_TYPE_WS:
        case STREAM_TYPE_WSS:
        case STREAM_TYPE_WS_SERVICE:

            ptr=LibUsefulGetValue("Net:Timeout");
            if (StrValid(ptr)) STREAMSetTimeout(S, atoi(ptr));
            break;
        }

    }

    Destroy(Token);
    Destroy(Proto);
    Destroy(Host);
    Destroy(User);
    Destroy(Pass);
    Destroy(Path);
    Destroy(Args);

    return(S);
}


//Destorys a STREAM object but does not close the associated file descriptor
//nor any associated streams
void STREAMDestroy(void *p_S)
{
    STREAM *S, *tmpS;
    ListNode *Curr, *Next;

    if (! p_S) return;

    S=(STREAM *) p_S;
    STREAMFlush(S);
    if (S->Flags & SF_SECURE)
    {
        SecureDestroy(S->InputBuff,S->BuffSize);
        SecureDestroy(S->OutputBuff,S->BuffSize);
    }
    else
    {
        //if SF_MMAP is set then S->InputBuff points to shared memory
        if (! (S->Flags & SF_MMAP)) Destroy(S->InputBuff);
        Destroy(S->OutputBuff);
    }

    //associate streams are streams that support other streams, like the ssh connection that
    //supports a port-forward through ssh. We close these down when the owner stream is closed
    Curr=ListGetNext(S->Items);
    while (Curr)
    {
        Next=ListGetNext(Curr);
        if (CompareStr(Curr->Tag, "LU:AssociatedStream")==0)
        {
            tmpS=(STREAM *) Curr->Item;
            STREAMClose(tmpS);
            ListDeleteNode(Curr);
        }
        else if (CompareStr(Curr->Tag, "HTTP:InfoStruct")==0)
        {
            HTTPInfoDestroy(Curr->Item);
            ListDeleteNode(Curr);
        }


        Curr=Next;
    }


    ListDestroy(S->Items, NULL);
    ListDestroy(S->Values,(LIST_ITEM_DESTROY_FUNC) Destroy);
    ListDestroy(S->ProcessingModules,DataProcessorDestroy);
    Destroy(S->Path);
    free(S);
}


void STREAMTruncate(STREAM *S, long size)
{
    if (ftruncate(S->out_fd, size) != 0) RaiseError(ERRFLAG_ERRNO, "STREAMTruncate", "cannot ftruncate %s", S->Path);
}



//Some special features specifically around closing files. Currently these mostly concern telling the OS that a file
//doesn't require caching (maybe becasue it's a logfile rather than data)
void STREAMCloseFile(STREAM *S)
{
    char *Tempstr=NULL;

    if (
        (StrValid(S->Path)) &&
        (CompareStr(S->Path,"-") !=0) //don't do this for stdin/stdout
    )
    {

        if (S->out_fd != -1)
        {
            //as we have closed the file sucessfully, we no longer need backup
            if (STREAMAutoRecoverRequired(S->Flags))
            {
                Tempstr=MCopyStr(Tempstr, S->Path, ".autorecover", NULL);
                unlink(Tempstr);
            }


            //if we don't need this file cached for future use, tell the os so when we close it
#ifdef POSIX_FADV_DONTNEED
            if (S->Flags & SF_NOCACHE)
            {
                fsync(S->out_fd);
                posix_fadvise(S->out_fd, 0,0,POSIX_FADV_DONTNEED);
            }
#endif

        }

        if (S->in_fd != -1)
        {
#ifdef POSIX_FADV_DONTNEED
            //if we don't need this input file cached for future use, tell the os so when we close it
            if (S->Flags & SF_NOCACHE) posix_fadvise(S->in_fd, 0,0,POSIX_FADV_DONTNEED);
#endif

        }
    }

    Destroy(Tempstr);
}



void STREAMShutdown(STREAM *S)
{
    ListNode *Curr;
    int val;

    if (! S) return;

    //-1 means 'FLUSH'
    STREAMReadThroughProcessors(S, NULL, -1);
    STREAMWriteBytes(S, NULL, 0); //means flush any processors
    STREAMFlush(S);

    switch (S->Type)
    {
    case STREAM_TYPE_SSH:
        SSHClose(S);
        break;

    case STREAM_TYPE_TTY:
        TTYHangUp(S->in_fd);
        break;

    case STREAM_TYPE_FILE:
        STREAMCloseFile(S);
        break;
    }


    //OpenSSLClose only closes things that the OpenSSL subsystem has created, so it's safe to call on all streams
    OpenSSLClose(S);

//For all streams we kill off any helper processes and close any associated streams
    Curr=ListGetNext(S->Values);
    while (Curr)
    {
        if (strncmp(Curr->Tag, "HelperPID", 9)==0)
        {
            val=atoi(Curr->Item);
            if (val > 1) kill(0-val, SIGKILL);
        }
        Curr=ListGetNext(Curr);
    }


    //now we actually close the file descriptors for this stream.
    if ((S->out_fd != S->in_fd) && (S->out_fd > -1)) close(S->out_fd);
    //out_fd is invalid now whether we closed it or not, so set it to -1
    //so that if STREAMClose gets called later (say, in garbage-collected environments)
    //we don't wind up closing another connection that has inhertied the file number
    S->out_fd=-1;

    if (S->in_fd > -1)
    {
        close(S->in_fd);
        S->in_fd=-1;
    }

    S->State=0;
}


void STREAMClose(STREAM *S)
{
    STREAMShutdown(S);
    STREAMDestroy(S);
}




//this function is used by STREAMReadCharsToBuffer. It checks for/ waits for input
int STREAMWaitForBytes(STREAM *S)
{
    int WaitForBytes=TRUE;
    int read_result, val;
    fd_set selectset;
    struct timeval tv;

    //if using SSL and already has bytes queued, don't do a wait on select
    if ( (S->State & LU_SS_SSL) && OpenSSLSTREAMCheckForBytes(S) ) WaitForBytes=FALSE;

    //if using Websockets and we are in the middle of a read, don't wait, as
    //the next call to websockets might return MESSAGE_END
    if (S->State & LU_SS_MSG_READ) WaitForBytes=FALSE;

    //must set this to 1 in case not doing a select, 'cos if S->Timeout is not set
    //then we won't wait at all, won't set read_result, so we must do it here
    read_result=1;

    //if we ned to wait, then do so
    if ((S->Timeout > 0) && WaitForBytes)
    {
        FD_ZERO(&selectset);
        FD_SET(S->in_fd, &selectset);
        //convert S->Timeout from centisecs number to a tv struct
        MillisecsToTV(S->Timeout * 10, &tv);

        //okay, wait for somethign to happen
        val=select(S->in_fd+1,&selectset,NULL,NULL,&tv);

        switch (val)
        {
        //we are only checking one FD, so should be 1
        case 1:
            read_result=1;
            break;

        case 0:
            errno=ETIMEDOUT;
            read_result=STREAM_TIMEOUT;
            break;

        default:
            if (errno==EINTR) read_result=STREAM_TIMEOUT;
            else read_result=STREAM_CLOSED;
            break;
        }

    }

    return(read_result);
}

static int STREAMReadCharsToBuffer_UDP(STREAM *S, char *Buffer, int Len)
{
    int bytes_read;
    char *Peer=NULL;

    bytes_read=UDPRecv(S->in_fd,  Buffer, Len, &Peer, NULL);
    //if select said there was stuff to read, but there wasn't, then the socket must be closed
    //sockets can return '0' when closed, so we normalize this to -1 here
    if (bytes_read < 1) bytes_read=-1;
    STREAMSetValue(S, "PeerIP", Peer);

    Destroy(Peer);

    return(bytes_read);
}


static int STREAMReadCharsToBuffer_Default(STREAM *S, char *Buffer, int Len)
{
    int bytes_read;

    if ((S->Type == STREAM_TYPE_FILE) && (S->Flags & SF_RDLOCK)) flock(S->in_fd,LOCK_SH);

    bytes_read=read(S->in_fd, Buffer, Len);

    //if select said there was stuff to read, but there wasn't, then the socket must be closed
    //sockets can return '0' when closed, so we normalize this to -1 here
    if (bytes_read < 1) bytes_read=-1;

    switch (S->Type)
    {
    case STREAM_TYPE_TCP:
    case STREAM_TYPE_SSL:
    case STREAM_TYPE_HTTP:
    case STREAM_TYPE_WS:
        if (S->Flags & SF_QUICKACK) SockSetOptions(S->in_fd, SOCK_TCP_QUICKACK, 1);
        break;

    case STREAM_TYPE_FILE:
        if (S->Flags & SF_RDLOCK) flock(S->in_fd,LOCK_UN);
        break;
    }

    return(bytes_read);
}

int STREAMPullBytes(STREAM *S, char *Buffer, int Len)
{
    if (S->State & LU_SS_SSL) return(OpenSSLSTREAMReadBytes(S, Buffer, Len));
    return(STREAMReadCharsToBuffer_Default(S, Buffer, Len));
}

int STREAMReadCharsToBuffer(STREAM *S)
{
    int val=0, read_result=0;
    long bytes_read=0;
    char *tmpBuff=NULL;

    if (! S) return(0);

//we don't read from and embargoed stream. Embargoed is a state that we
//use to indicate a stream must be ignored for a while
    if (S->State & LU_SS_EMBARGOED) return(0);

//number of bytes queued in STREAM
    val=S->InEnd-S->InStart;

    if (val > 0)
    {
        //for UDP we recv messages, and we must consume (or flush) all of one before getting the next
        if (S->Type==STREAM_TYPE_UDP) return(1);

        //we never read from an MMAP, it's just a block of memory that we copy bytes from
        if (S->Flags & SF_MMAP) return(1);
    }
    else
    {
        //if we traverse all bytes in an MMAP, then we return STREAM_CLOSED to tell that we're at the end
        if (S->Flags & SF_MMAP) return(STREAM_CLOSED);

        //if it's empty, then reset all the pointers for a fresh read
        S->InEnd=0;
        S->InStart=0;
    }


//if buffer is half full, or full except for space at the start, then make room
    if (
        (S->InStart > (S->BuffSize / 2)) ||
        ((S->InEnd >= S->BuffSize) && (S->InStart > 0))
    )
    {
        memmove(S->InputBuff,S->InputBuff + S->InStart, val);
        S->InStart=0;
        S->InEnd=val;
    }

//if no room in buffer, we can't read in more bytes
    if (S->InEnd >= S->BuffSize) return(1);

    read_result=STREAMWaitForBytes(S);

    //Here we perform the actual read
    if (read_result==1)
    {

        val=S->BuffSize - S->InEnd;
        if (val < 0) return(1);
        tmpBuff=(char *) realloc(tmpBuff, val);

        //OpenSSL can return 0 bytes, even if select said there was stuff to be read from the
        //socket, due to keepalives etc
        switch (S->Type)
        {
        case STREAM_TYPE_WS:
        case STREAM_TYPE_WSS:
        case STREAM_TYPE_WS_SERVICE:
            bytes_read=WebSocketReadBytes(S, tmpBuff, val);
            break;

        case STREAM_TYPE_UDP:
            bytes_read=STREAMReadCharsToBuffer_UDP(S, tmpBuff, val);
            break;

        default:
            bytes_read=STREAMPullBytes(S, tmpBuff, val);
            break;
        }

        if (bytes_read >= 0)
        {
            S->BytesRead+=bytes_read;
            read_result=1;
        }
        else read_result=bytes_read;
    }

//messing with this block tends to break STREAMSendFile
    if (read_result >= 0)
    {
        read_result=STREAMReadThroughProcessors(S, tmpBuff, bytes_read);
    }
    else //if (read_result == STREAM_CLOSED)
    {
        //-1 means 'FLUSH'
        //there's no bytes in tmpBuff in this situation
        bytes_read=STREAMReadThroughProcessors(S, NULL, -1);
        if (bytes_read > 0) read_result=bytes_read;
        //else read_result=STREAM_CLOSED;
    }


//We are not returning number of bytes read. We only return something if
//there is a condition (like socket close) where the thing we are waiting for
//may not appear

    Destroy(tmpBuff);

    return(read_result);
}



int STREAMTransferBytesOut(STREAM *S, char *Dest, int DestSize)
{
    int bytes;

    bytes=S->InEnd - S->InStart;

    if (bytes > DestSize) bytes=DestSize;
    if (bytes < 1) return(0);

    memcpy(Dest,S->InputBuff+S->InStart,bytes);
    S->InStart+=bytes;

    return(bytes);
}



int STREAMReadMessage(STREAM *S, char *Buffer, int Buffsize, int *BytesRead)
{
    int state=STREAM_NODATA, bytes=0, result=0, total=0;


    while (total < Buffsize)
    {
        if (S->InStart >= S->InEnd)
        {
            result=STREAMReadCharsToBuffer(S);
            if (result > 0) state=STREAM_BYTES_READ;
            else
            {
                state=result;
                break;
            }
        }

        total+=STREAMTransferBytesOut(S, Buffer+total, Buffsize-total);
        bytes=S->InEnd - S->InStart;
        //if no other bytes currently available, then drop out
        if (! FDCheckForBytes(S->in_fd)) break;
    }

    *BytesRead=total;
    return(state);
}



int STREAMReadBytes(STREAM *S, char *Buffer, int Buffsize)
{
    int state, bytes_read=0;
    state=STREAMReadMessage(S, Buffer, Buffsize, &bytes_read);

    if (state == STREAM_BYTES_READ) return(bytes_read);

//STREAM_NODATA and STREAM_CLOSE can both occur and still
//return some data, so if we had any bytes read, return that
    if (bytes_read > 0) return(bytes_read);

    return(state);
}

uint64_t STREAMTell(STREAM *S)
{
    uint64_t pos;

    if (S->Flags & SF_MMAP) return((uint64_t) S->InStart);
    if (S->OutEnd > 0) STREAMFlush(S);

#ifdef _LARGEFILE64_SOURCE
    pos=(uint64_t) lseek64(S->in_fd,0,SEEK_CUR);
#else
    pos=(uint64_t) lseek(S->in_fd,0,SEEK_CUR);
#endif
    pos-=(S->InEnd-S->InStart);

    return(pos);
}


uint64_t STREAMSeek(STREAM *S, int64_t offset, int whence)
{
    int64_t pos=0;
    int wherefrom;

    if (S->Flags & SF_MMAP)
    {
        switch (whence)
        {
        case SEEK_SET:
            S->InStart=offset;
            break;
        case SEEK_CUR:
            S->InStart += offset;
            break;
        case SEEK_END:
            S->InStart=S->InEnd-offset;
            break;
        }
        return((uint64_t) S->InStart);
    }

    if (S->OutEnd > 0) STREAMFlush(S);

    if (whence==SEEK_CUR)
    {
        pos=STREAMTell(S);
        pos+=offset;
        wherefrom=SEEK_SET;
    }
    else
    {
        pos=offset;
        wherefrom=whence;
    }
    S->InStart=0;
    S->InEnd=0;

#ifdef _LARGEFILE64_SOURCE
    pos=(uint64_t) lseek64(S->in_fd,(off64_t) pos, wherefrom);
#else
    pos=(uint64_t) lseek(S->in_fd,(off_t) pos, wherefrom);
#endif


    return(pos);
}



static int STREAMInternalPushProcessingModules(STREAM *S, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen)
{
    TProcessingModule *Mod;
    ListNode *Curr, *Next;
    int len, AllDataWritten=TRUE;
    char *TempBuff=NULL;
    const char *ptr;
    int Flush=FALSE;

    len=InLen;
    ptr=InData;
    if (InLen==0) Flush=TRUE;

    //Go through processing modules feeding the data from the previous one into them
    Curr=ListGetNext(S->ProcessingModules);
    while (Curr)
    {
        Next=ListGetNext(Curr);
        Mod=(TProcessingModule *) Curr->Item;

        if (Mod->Write && ((len > 0) || Flush))
        {
            len=Mod->Write(Mod, ptr, len, OutData, OutLen, Flush);
            if (Flush && (len !=STREAM_CLOSED)) AllDataWritten=FALSE;
            if (Next)
            {
                //BEWARE: OutLen can be changed, and made larger, by processing modules
                TempBuff=(char *) realloc(TempBuff, (*OutLen) + 8);

                memcpy(TempBuff, *OutData, *OutLen);
                ptr=TempBuff;
                len=*OutLen;
            }
        }
        Curr=Next;
    }

    Destroy(TempBuff);
    return(AllDataWritten);
}


//this function returns the number of bytes *queued*, not number
//written
static int STREAMInternalQueueBytes(STREAM *S, const char *Bytes, int Len)
{
    int o_len, queued=0, avail, val=0, result=0;
    const char *ptr;

    o_len=Len;
    ptr=Bytes;

    do
    {
        if (o_len > 0)
        {
            avail=S->BuffSize - S->OutEnd;
            if (avail > 0)
            {
                if (avail > o_len) avail=o_len;

                memcpy(S->OutputBuff+S->OutEnd,ptr,avail);
                ptr+=avail;
                o_len-=avail;
                S->OutEnd+=avail;

                queued+=avail;
            }
        }

        //Buffer Full, Write some bytes!!!
        if ( (S->OutEnd > S->StartPoint) &&
                (
                    (S->OutEnd >= S->BuffSize) ||
                    (S->Flags & FLUSH_ALWAYS) ||
                    ((S->Flags & FLUSH_BLOCK) && (S->OutEnd > S->BlockSize))
                )
           )
        {
            //Any decision about how much to write takes place in InternalPush
            result=STREAMInternalPushBytes(S, S->OutputBuff, S->OutEnd);
            if (result < 1) break;
            S->StartPoint=0;
        }
    }
    while (o_len > 0);

    if ((result < 0) && (result != STREAM_TIMEOUT)) return(result);
    return(queued);
}



/*A stream can have a series of 'processor modules' associated with it' */
/*which do things to the data before it is read/written. This function  */
/*pumps the data through the processor list, and eventually writes it out */
int STREAMWriteBytes(STREAM *S, const char *Data, int DataLen)
{
    const char *i_data;
    char *TempBuff=NULL;
    int result=0;
    unsigned long len;


    if (! S) return(STREAM_CLOSED);
    if (S->out_fd==-1) return(STREAM_CLOSED);
    if (S->State & LU_SS_WRITE_ERROR) return(STREAM_CLOSED);


    i_data=Data;
    len=DataLen;

    if (ListSize(S->ProcessingModules))
    {
        if (len < 4096) len=4096;

        //BEWARE: TempBuff can be changed in functions below, so always realloc it
        TempBuff=(char *) realloc(TempBuff, len * 2);

        len=0;
        STREAMInternalPushProcessingModules(S, i_data, DataLen, &TempBuff, &len);
        i_data=TempBuff;
    }

//This always queues all the data, though it may not flush it all
//thus the calling application always believes all data is written
//Thus we only report errors if len==0;
    if (len > 0) result=STREAMInternalQueueBytes(S, i_data, len);
    else if (S->OutEnd > S->StartPoint)
    {
        result=STREAMInternalPushBytes(S, S->OutputBuff, S->OutEnd);
    }

    Destroy(TempBuff);

    if (result < 0) return(result);
    else return(DataLen);
}




int STREAMWriteString(const char *Buffer, STREAM *S)
{
    int result;

    if (StrEnd(Buffer)) return(FALSE);
    result=STREAMWriteBytes(S,Buffer,strlen(Buffer));
    return(result);
}

int STREAMWriteLine(const char *Buffer, STREAM *S)
{
    int result;

    if (StrEnd(Buffer)) return(FALSE);
    result=STREAMWriteBytes(S,Buffer,strlen(Buffer));
    if (result < 0) return(result);
    if (S->Flags & FLUSH_LINE) result=STREAMFlush(S);
    return(result);
}


int STREAMReadChar(STREAM *S)
{
    unsigned char inchar;
    int result;

    result=STREAMReadBytes(S, &inchar,1);

    if (result < 0) return(result);
    if (result==0) return(STREAM_NODATA);
    return((int) inchar);
}

int STREAMReadUint32(STREAM *S, long *RetVal)
{
    uint32_t value;
    int result;

    result=STREAMReadBytes(S, (unsigned char *) &value,sizeof(uint32_t));
    if (result < 0) return(result);
    if (result==0) return(STREAM_NODATA);
    *RetVal=(long) value;
    return(result);
}

int STREAMReadUint16(STREAM *S, long *RetVal)
{
    uint16_t value;
    int result;

    result=STREAMReadBytes(S, (unsigned char *) &value,sizeof(uint16_t));
    if (result < 0) return(result);
    if (result==0) return(STREAM_NODATA);
    *RetVal=(long) value;
    return(result);
}

int STREAMPeekChar(STREAM *S)
{
    int result;

    if (S->InStart >= S->InEnd)
    {
        result=STREAMReadCharsToBuffer(S);
        if (result < 1) return(result);
    }

    return(* (S->InputBuff + S->InStart));
}


int STREAMPeekBytes(STREAM *S, char *Buffer, int Buffsize)
{
    int len=0, result=0;

    if (S->InStart >= S->InEnd)
    {
        result=STREAMReadCharsToBuffer(S);
        if (S->InStart >= S->InEnd)
        {
            if (result==STREAM_CLOSED) return(STREAM_CLOSED);
            if (result==STREAM_TIMEOUT) return(STREAM_TIMEOUT);
            if (result==STREAM_DATA_ERROR) return(STREAM_DATA_ERROR);
        }
    }

    len=S->InEnd-S->InStart;
    if (len > Buffsize) len=Buffsize;

    if (len > 0) memcpy(Buffer,S->InputBuff + S->InStart,len);

    return(len);
}


void STREAMInsertBytes(STREAM *S, const char *Bytes, int len)
{
    size_t val;

    if (S->InStart > len)
    {
        S->InStart-=len;
        memcpy(S->InputBuff + S->InStart, Bytes, len);
    }
    else
    {
        val=len + S->InEnd - S->InStart;
        if (val > S->BuffSize)
        {
            S->BuffSize=val;
            STREAMResizeBuffer(S, S->BuffSize);
        }
        memmove(S->InputBuff + S->InStart, S->InputBuff+len, S->InEnd - S->InStart);
        memcpy(S->InputBuff,Bytes,len);
        S->InStart=0;
        S->InEnd=val;
    }

}



int STREAMWriteChar(STREAM *S,unsigned char inchar)
{
    char tmpchar;

    tmpchar=(char) inchar;
    return(STREAMWriteBytes(S,&tmpchar,1));
}



int STREAMReadBytesToTerm(STREAM *S, char *Buffer, int BuffSize,unsigned char Term)
{
    int inchar, pos=0;

    inchar=STREAMReadChar(S);
    while (inchar != STREAM_CLOSED)
    {
        if (inchar > -1)
        {
            Buffer[pos]=inchar;
            pos++;
            if (inchar==Term) break;
            if (pos==BuffSize) break;
        }
        inchar=STREAMReadChar(S);
    }

    if ((pos==0) && (inchar==STREAM_CLOSED)) return(STREAM_CLOSED);
    return(pos);
}


char *STREAMReadToTerminator(char *Buffer, STREAM *S, unsigned char Term)
{
    int result, len=0, avail=0, bytes_read=0;
    char *RetStr=NULL;
    const unsigned char *p_Term;
    int IsClosed=FALSE;

    if (! S)
    {
        RaiseError(0, "STREAMReadToTerminator", "NULL stream object passed to function");
        Destroy(Buffer);
        return(NULL);
    }

    RetStr=CopyStr(Buffer,"");
    while (1)
    {
        len=0;
        avail=S->InEnd-S->InStart;

        //if this gets set we've found a terminator
        p_Term=NULL;

        //if we have bytes in buffer then check for terminator or buffer full
        //in either case set len to transfer bytes out
        if (avail > 0)
        {
            //if buffer full, or terminator char present, set len
            //memchr is wicked fast, so use it
            p_Term=memchr(S->InputBuff+S->InStart, Term, avail);
            if (p_Term) len=(p_Term+1)-(S->InputBuff+S->InStart);
            else if (IsClosed) len=avail;
            else if (S->InStart > 0) len=0; //call 'ReadCharsToBuffer' which will move 'InStart' to start of buffer
            else if (S->InEnd >= S->BuffSize) len=avail;
        }
        //if nothing in buffer and connection closed, return NULL
        else if (IsClosed)
        {
            if (bytes_read > 0) return(RetStr);
            Destroy(RetStr);
            return(NULL);
        }

        //if no terminator then read some more bytes. if we get 'stream closed' then set 'IsClosed'
        if (len==0)
        {
            result=STREAMReadCharsToBuffer(S);
            switch (result)
            {
            case STREAM_CLOSED:
                IsClosed=TRUE;
                break;
            //if we timeout then just return RetStr (which will be empty) this
            //distinguishes timeout conditions from the stream being closed, as when
            //the stream is closed we return NULL
            case STREAM_TIMEOUT:
                return(RetStr);
                break;
            }
        }
        //if len > 0 then we have a string to return!
        else
        {
            // RetStr will likely be an ascii string as we are reading to a
            // terminating character, but we can use realloc here as we
            // call StrLenCacheAdd below
            RetStr=(char *) realloc(RetStr, bytes_read + len + 8);

            len=STREAMTransferBytesOut(S, RetStr + bytes_read, len);
            bytes_read+=len;

            //cannot use StrTrunc here, just in case we have a binary or
            //partial string, which could confuse it. StrUnsafeTrunc is
            //suitable though, as it doesn't StrLen the string
            if (bytes_read > -1) StrUnsafeTrunc(RetStr, bytes_read);

            if (p_Term) return(RetStr);
        }

    }

//impossible to get here!
    return(NULL);
}



char *STREAMReadToMultiTerminator(char *Buffer, STREAM *S, char *Terms)
{
    int inchar, len=0;
    char *Tempptr;

    Tempptr=CopyStr(Buffer,"");

    inchar=STREAMReadChar(S);


//All error conditions are negative, but '0' can just mean
//no more data to read
    while (inchar > -1)
    {
        if (inchar > 0)
        {
            Tempptr=AddCharToBuffer(Tempptr,len,(char) inchar);
            len++;

            if (strchr(Terms,inchar)) break;
        }
        inchar=STREAMReadChar(S);
    }


    *(Tempptr+len)='\0';

//if ((inchar==STREAM_CLOSED) && (errno==ECONNREFUSED)) return(Tempptr);
    if (
        ((inchar==STREAM_CLOSED) || (inchar==STREAM_DATA_ERROR))
        &&
        (StrEnd(Tempptr))
    )
    {
        free(Tempptr);
        return(NULL);
    }

    return(Tempptr);
}




char *STREAMReadLine(char *Buffer, STREAM *S)
{
    return(STREAMReadToTerminator(Buffer,S,'\n'));
}


void STREAMResetInputBuffers(STREAM *S)
{
    double pos;

    pos=STREAMTell(S);
    S->InStart=0;
    S->InEnd=0;
    STREAMSeek(S,pos,SEEK_SET);
}



int STREAMReadToString(STREAM *S, char **RetStr, int *len, const char *Term)
{
    int match=0, termlen=0, inchar;

    if (len) *len=0;
    //a terminator is usually short, so don't use StrLenFromCache
    termlen=StrLen(Term);
    inchar=STREAMReadChar(S);

    while (inchar !=EOF)
    {

        if (RetStr && len && (inchar > -1)) *RetStr=AddCharToBuffer(*RetStr, (*len)++, inchar);

        if (termlen > 0)
        {
            //if the current value does not equal where we are in the string
            //we have to consider whether it is the first character in the string
            if (Term[match]!=inchar) match=0;

            if (Term[match]==inchar)
            {
                match++;
                if (match==termlen) return(TRUE);
            }
        }
        inchar=STREAMReadChar(S);
    }

    return(FALSE);
}



char *STREAMReadDocument(char *RetStr, STREAM *S)
{
    char *Tempstr=NULL;
    const char *ptr;
    int result=0, bytes_read=0, new_bytes=0;
    int size, max;

    //for documents where we know the size, e.g. HTTP documents where we've had 'Content-Length'
    //there will be a size booked against the stream 'S'
    if ( (S->Size > 0) && (! (S->State & LU_SS_COMPRESSED)) ) size=S->Size;

    //we can set a program-wide max document size
    max=LibUsefulGetInteger("MaxDocumentSize");
    if (max > 0) size=max;

    //we don't need to use SetStrLen here, as we use StrLenCacheAdd
    //before the end of this function to update the length of this object
    //we do need to use realloc rather than malloc though, as we've been
    //passed in RetStr which is of unknown size.
    RetStr=(char *) realloc(RetStr, size +8);

    while (bytes_read < size)
    {
        new_bytes=0;
        result=STREAMReadMessage(S, RetStr+bytes_read, size - bytes_read, &new_bytes);
        bytes_read+=new_bytes;

        if (result != STREAM_BYTES_READ) break;
    }

    //don't trust StrTrunc here, as we might have read
    //binary data that can include '\0' characters
    //StrTrunc(RetStr,bytes_read);

    //instead of StrTrunc use StrUnsafeTrunc to save a StrLen call
    StrUnsafeTrunc(RetStr, bytes_read);


    /*
        //if result > 0 then we didn't break out on reading a 'STREAM_CLOSED' or other close condition, we broke out because we had hit the size limit
        if ((bytes_read==size) && (new_bytes > 0))
        {
            if (bytes_read==max) RaiseError(0, "STREAMReadDocument", "Document size is greater than Max Document Size of %s bytes", ToIEC(max, 1));
        }
    */

    /*
        }
        else
        {

            Tempstr=STREAMReadLine(Tempstr, S);
            while (Tempstr)
            {
                RetStr=CatStr(RetStr, Tempstr);
                Tempstr=STREAMReadLine(Tempstr, S);
            }
        }
    */

    Destroy(Tempstr);
    return(RetStr);
}




char *STREAMGetValue(STREAM *S, const char *Name)
{
    ListNode *Curr;

    if ((!S) || (! S->Values)) return(NULL);
    Curr=ListFindNamedItem(S->Values,Name);
    if (Curr) return(Curr->Item);
    return(NULL);
}


void STREAMSetValue(STREAM *S, const char *Name, const char *Value)
{
    if (! S) return;
    if (! S->Values) S->Values=ListCreate();
    SetVar(S->Values,Name,Value);
}

void *STREAMGetItem(STREAM *S, const char *Name)
{
    ListNode *Curr;

    if ((!S) || (! S->Items)) return(NULL);
    Curr=ListFindNamedItem(S->Items,Name);
    if (Curr) return(Curr->Item);
    return(NULL);
}


void STREAMSetItem(STREAM *S, const char *Name, void *Value)
{
    ListNode *Curr;

    if (! S) return;
    if (! S->Items) S->Items=ListCreate();
    Curr=ListFindNamedItem(S->Items,Name);
    if (Curr) Curr->Item=Value;
    else ListAddNamedItem(S->Items,Name,Value);
}


int STREAMFindCompare(const char *Line, const char *Item, const char *Delimiter, char **RetStr)
{
    char *ptr=NULL;
    int result;

    if (StrValid(Delimiter)) ptr=strstr(Line, Delimiter);

    if (ptr) result=strncmp(Item, Line, ptr-Line);
    else result=CompareStr(Item, Line);

    if (RetStr && (result==0)) *RetStr=CopyStr(*RetStr, Line);

    return(result);
}



int STREAMFindBinarySearch(STREAM *S, const char *Item, const char *Delimiter, char **RetStr)
{
    char *Tempstr=NULL;
    struct stat Stat;
    double Size, delta, pos;
    int result, inchar;


    if (S->Flags & SF_MMAP) Size=S->InEnd;
    else
    {
        fstat(S->in_fd, &Stat);
        Size=Stat.st_size;
    }

    delta=Size / 2;
    pos=delta;
    while (delta > 100)
    {
        STREAMSeek(S, pos, SEEK_SET);
        inchar=STREAMReadChar(S);
        while ((inchar != '\n') && (inchar != EOF)) inchar=STREAMReadChar(S);
        if (inchar==EOF) return(FALSE);
        Tempstr=STREAMReadLine(Tempstr, S);
        StripTrailingWhitespace(Tempstr);
        result=STREAMFindCompare(Tempstr, Item, Delimiter, RetStr);
        if (result ==0) break;

        delta /= 2;
        if (result > 0) pos+=delta;
        else pos-=delta;
    }

//rewind if we need to
    while ((result < 0) && (pos > 0))
    {
        pos-=1024;
//if we're near the start of the file, then just seek back to the start
        if (pos < 1024) pos=0;
        STREAMSeek(S, pos, SEEK_SET);
        if (pos > 0)
        {
            inchar=STREAMReadChar(S);
            while ((inchar != '\n') && (inchar != EOF)) inchar=STREAMReadChar(S);
        }

        Tempstr=STREAMReadLine(Tempstr, S);
        StripTrailingWhitespace(Tempstr);

        result=STREAMFindCompare(Tempstr, Item, Delimiter, RetStr);
    }

    while (Tempstr)
    {
        result=STREAMFindCompare(Tempstr, Item, Delimiter, RetStr);
        if (result < 1)
        {
            Destroy(Tempstr);
            if (result==0) return(TRUE);
            return(FALSE);
        }
        Tempstr=STREAMReadLine(Tempstr, S);
        StripTrailingWhitespace(Tempstr);
    }

    Destroy(Tempstr);
    return(FALSE);
}


int STREAMFind(STREAM *S, const char *Item, const char *Delimiter, char **RetStr)
{
    char *Tempstr=NULL;

    if (!S) return(FALSE);

    if (S->Flags & SF_SORTED) return(STREAMFindBinarySearch(S, Item, Delimiter, RetStr));

    Tempstr=STREAMReadLine(Tempstr, S);
    while (Tempstr)
    {
        StripTrailingWhitespace(Tempstr);
        if (STREAMFindCompare(Tempstr, Item, Delimiter, RetStr)==0)
        {
            Destroy(Tempstr);
            return(TRUE);
        }
        Tempstr=STREAMReadLine(Tempstr, S);
    }

    Destroy(Tempstr);
    return(FALSE);
}


static int UseKernelSendFile(STREAM *In, STREAM *Out, int Flags)
{
    if (! (Flags & SENDFILE_KERNEL)) return(FALSE);
    switch (In->Type)
    {
    case STREAM_TYPE_FILE:
    case STREAM_TYPE_PIPE:
        break;

    default:
        return(FALSE);
        break;
    }

    if  (ListSize(In->ProcessingModules) > 0) return(FALSE);
    if  (ListSize(Out->ProcessingModules) > 0) return(FALSE);

    return(TRUE);
}


unsigned long STREAMSendFile(STREAM *In, STREAM *Out, unsigned long Max, int Flags)
{
    unsigned long bytes_transferred=0;
    int result;
    int UseSendFile=FALSE;

//val has to be as long as possible, because it will hold the difference
//between two long values. However, use of 'long long' resulted in an
//unsigned value, which caused all manner of problems, so a long is the
//best we can manage
    long val, len, towrite;


    if (Max==0) len=BUFSIZ;
    else len=(long) Max;


#ifdef HAVE_SENDFILE
#include <sys/sendfile.h>

//if we are not using ssl and not using processor modules, we can use
//kernel-level copy!

    if (UseKernelSendFile(In, Out, Flags))
    {
        UseSendFile=TRUE;
        STREAMFlush(Out);
    }
#endif


    while (len > 0)
    {
#ifdef HAVE_SENDFILE
        if (UseSendFile)
        {
            result=sendfile(Out->out_fd, In->in_fd,0,len);
            if (result < 1)
            {
                if (bytes_transferred==0) UseSendFile=FALSE;
                result=STREAM_CLOSED;
            }
            else
            {
                //if used sendfile we need to do this because we didn't call
                //lower level functions like 'STREAMReadCharsToBuffer' that would
                //update these
                In->BytesRead+=result;
                Out->BytesWritten+=result;
            }
        }
#endif

        if (! UseSendFile)
        {
            //How much do we have queued in the in-stream?
            towrite=In->InEnd - In->InStart;


            //only read if we don't already have some in inbuff
            if ((towrite < len) && (! (Flags & SENDFILE_NOREAD)))
            {
                result=STREAMReadCharsToBuffer(In);
                towrite=In->InEnd - In->InStart;
                if ((result==STREAM_CLOSED) && (towrite==0))
                {
                    STREAMFlush(Out);
                    return(bytes_transferred);
                }
            }

            //if it's more than we've been told to get then adjust
            if (towrite > len) towrite=len;

            //if outbuff hasn't enough space to take the transfer, then do some flushing
            val=Out->BuffSize - Out->OutEnd;
            if (val < 1)
            {
                val=BUFSIZ;
                if (val > Out->OutEnd) val=Out->OutEnd;
                STREAMInternalPushBytes(Out, Out->OutputBuff, val);
                sleep(0);
                val=Out->BuffSize - Out->OutEnd;
            }


            //if we still haven't got enough room then adjust our expectations
            if (towrite > val) towrite=val;
            result=0;


            result=STREAMWriteBytes(Out,In->InputBuff+In->InStart,towrite);

            //write failed with 'STREAM_CLOSED'
            if (result==STREAM_CLOSED) break;
            else if (result > 0)
            {
                In->InStart+=result;
                bytes_transferred+=result;
            }
        }

        if (Flags & SENDFILE_FLUSH) STREAMFlush(Out);
        if (! (Flags & SENDFILE_LOOP)) break;

        if (Max==0) len=BUFSIZ;
        else len=(long) (Max-bytes_transferred);
    }

    return(bytes_transferred);
}


unsigned long STREAMCopy(STREAM *Src, const char *DestPath)
{
    STREAM *Dest;
    unsigned long result;

    Dest=STREAMOpen(DestPath,"wc");
    if (! Dest) return(0);

    result=STREAMSendFile(Src, Dest, 0, SENDFILE_LOOP);
// | SENDFILE_KERNEL);
    STREAMClose(Dest);
    return(result);
}


int STREAMCommit(STREAM *S)
{
    HTTPInfoStruct *Item;

    if (! (S->Flags & SF_RDONLY) )
    {
        Item=(HTTPInfoStruct *) STREAMGetItem(S, "HTTP:InfoStruct");
        if ( Item && (HTTPTransact(Item) !=NULL) ) return(TRUE);
    }

    //for streams where we are talking to someting on pipes
    //(usually cmd: type streams where we are talking to a command on it's stdin)
    // close our stdout
    if (S->Type==STREAM_TYPE_PIPE)
    {
        STREAMFlush(S);
        close(S->out_fd);
        S->out_fd=-1;
        return(TRUE);
    }


    return(FALSE);
}
