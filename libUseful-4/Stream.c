#include "includes.h"
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
#include <sys/file.h>
#include "SecureMem.h"
#include <sys/mman.h>
#include <sys/ioctl.h>

#ifdef linux
#include <linux/fs.h>
#endif




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

static void *SelectAddFD(TSelectSet *Set, int type, int fd)
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
#include <math.h>

static int SelectWait(TSelectSet *Set, struct timeval *tv)
{
    long long timeout, next;
    double start, diff, val;
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

    //immutable and append only flags are a special case as
    //they are not io flags, but permanent file flags so we
    //only touch those if explicitly set in Set or UnSet
    if ((Set | UnSet) & (STREAM_IMMUTABLE | STREAM_APPENDONLY))
    {
#ifdef FS_IOC_SETFLAGS
        ioctl(S->out_fd, FS_IOC_GETFLAGS, &val);

#ifdef FS_IMMUTABLE_FL
        if (Set & STREAM_IMMUTABLE) val |= FS_IMMUTABLE_FL;
        else if (UnSet & STREAM_IMMUTABLE) val &= ~FS_IMMUTABLE_FL;
#endif

#ifdef FS_APPEND_FL
        if (Set & STREAM_APPENDONLY) val |= FS_APPEND_FL;
        else if (UnSet & STREAM_APPENDONLY) val |= FS_APPEND_FL;
#endif

        ioctl(S->out_fd, FS_IOC_SETFLAGS, &val);
#endif
    }
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
    char *ibuf=NULL, *obuf=NULL;

    if (S->Flags & SF_MMAP) return;

    if ((Flags & SF_SECURE) && (! (Flags & SF_SECURE)))
    {
        ibuf=S->InputBuff;
        obuf=S->OutputBuff;
        S->InputBuff=NULL;
        S->OutputBuff=NULL;
    }

    if (S->Flags & SF_SECURE)
    {
        if (! (S->Flags & SF_WRONLY)) SecureRealloc(&S->InputBuff, S->BuffSize, size, SMEM_SECURE);
        if (! (S->Flags & SF_RDONLY)) SecureRealloc(&S->OutputBuff, S->BuffSize, size, SMEM_SECURE);
    }
    else
    {
        if (! (S->Flags & SF_WRONLY)) S->InputBuff =(char *) realloc(S->InputBuff,size);
        if (! (S->Flags & SF_RDONLY)) S->OutputBuff=(char *) realloc(S->OutputBuff,size);
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
    if (S->State & SS_EMBARGOED) return(FALSE);
    if (S->InEnd > S->InStart) return(TRUE);
    if (S->in_fd==-1) return(FALSE);

    if (S->Flags & SF_FOLLOW)
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
    if (S->State & SS_EMBARGOED) return(0);

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
        if (S && (! (S->State & SS_EMBARGOED)))
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
    if (S->State & SS_EMBARGOED) return(0);

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



static int STREAMInternalFinalWriteBytes(STREAM *S, const char *Data, int DataLen)
{
    fd_set selectset;
    int result=0, count=0, len;
    struct timeval tv;
    void *vptr;

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
        if (S->State & SS_SSL) result=OpenSSLSTREAMWriteBytes(S, Data+count, DataLen-count);
        else
        {
            if (S->Timeout > 0)
            {
                FD_ZERO(&selectset);
                FD_SET(S->out_fd, &selectset);
                MillisecsToTV(S->Timeout * 10, &tv);
                result=select(S->out_fd+1,NULL,&selectset,NULL,&tv);
                if (result < 1)
                {
                    if ((result == 0) || (errno==EAGAIN)) result=STREAM_TIMEOUT;
                    else result=STREAM_CLOSED;
                    break;
                }
            }

            if (S->Flags & SF_WRLOCK) flock(S->out_fd,LOCK_EX);
            result=DataLen-count;
            //if (S->BlockSize && (S->BlockSize < (DataLen-count))) result=S->BlockSize;
            result=write(S->out_fd, Data + count, result);
            if (result < 0)
            {
                if (errno==EINTR) result=0;
                else if (errno==EAGAIN) result=STREAM_TIMEOUT;
                else result=STREAM_CLOSED;
            }
            if (S->Flags & SF_WRLOCK) flock(S->out_fd,LOCK_UN);

            //yes, we neglect to do a sync. The idea here is to work opportunisitically, flushing out those pages
            //that have been written. We do a sync and another fadvise in STREAMClose
#ifdef POSIX_FADV_DONTNEED
            if (S->Flags & SF_NOCACHE) posix_fadvise(S->out_fd, 0,0,POSIX_FADV_DONTNEED);
#endif

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
    val=STREAMInternalFinalWriteBytes(S, S->OutputBuff, S->OutEnd);
    //if nothing left in stream (There shouldn't be) then wipe data because
    //there might have been passwords sent on the stream, and we don't want
    //that hanging about in memory
    if (S->OutEnd==0) xmemset(S->OutputBuff,0,S->BuffSize);

    return(val);
}

void STREAMClear(STREAM *S)
{
    STREAMFlush(S);
    S->InStart=0;
    S->InEnd=0;
    //clear input buffer, because anything might be hanging about in there and
    //we don't want sensitive data persisiting in memory
    xmemset(S->InputBuff,0,S->BuffSize);
}


/*A stream can have a series of 'processor modules' associated with it' */
/*which do things to the data before it is read/written. This function  */
/*pumps the data through the processor list, and eventually writes it out */
int STREAMReadThroughProcessors(STREAM *S, char *Bytes, int InLen)
{
    TProcessingModule *Mod;
    ListNode *Curr;
    char *InBuff=NULL, *OutputBuff=NULL;
    char *p_Input;
    int state=STREAM_CLOSED, result;
    unsigned long olen=0, len=0;


    p_Input=Bytes;
    Curr=ListGetNext(S->ProcessingModules);
    if (InLen > 0) len=InLen;

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
                OutputBuff=SetStrLen(OutputBuff,olen);
                //if InLen == -1 then we are requesting a flush
                if (result < 0) result=Mod->Read(Mod,p_Input,len,&OutputBuff,&olen, TRUE);
                else result=Mod->Read(Mod,p_Input,len,&OutputBuff,&olen, FALSE);
                if (result != STREAM_CLOSED) state=0;

                if (result > 0)
                {
                    len=result;
                    InBuff=SetStrLen(InBuff,len);
                    memcpy(InBuff,OutputBuff,len);
                    p_Input=InBuff;
                }
                else len=0;
            }
        }

        Curr=ListGetNext(Curr);
    }

    if (
        (! (S->State & SS_DATA_ERROR)) &&
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
        if (state==STREAM_CLOSED) return(state);
        if (S->State & SS_DATA_ERROR) return(STREAM_DATA_ERROR);
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

    S=(STREAM *) calloc(1,sizeof(STREAM));
    STREAMResizeBuffer(S,BUFSIZ);
    S->in_fd=-1;
    S->out_fd=-1;
    S->Timeout=3000;
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

    if (in_fd==-1) return(NULL);
    if (out_fd==-1) return(NULL);

    Stream=STREAMCreate();
    Stream->in_fd=in_fd;
    Stream->out_fd=out_fd;
    return(Stream);
}



int STREAMOpenMMap(STREAM *S, int offset, int len, int Flags)
{
    char *ptr;
    int MProt=PROT_READ;

    if (S->InputBuff) free(S->InputBuff);
    if (Flags & (SF_WRONLY | SF_RDWR)) MProt |= PROT_WRITE;
    ptr=(char *) mmap(0, len, MProt, MAP_SHARED, S->in_fd, offset);
    if (ptr==MAP_FAILED) return(FALSE);
    S->InEnd=len;
    S->InStart=0;
    S->Flags |= SF_MMAP;
    S->InputBuff=ptr;
    if (Flags & SF_SECURE) mlock(ptr, len);

    return(TRUE);
}


STREAM *STREAMFileOpen(const char *Path, int Flags)
{
    int fd, Mode=0;
    STREAM *Stream;
    struct stat myStat;
    char *Tempstr=NULL, *NewPath=NULL;
    const char *p_Path, *ptr;

    p_Path=Path;
    if (Flags & SF_WRONLY) Mode=O_WRONLY;
    else if (Flags & SF_RDONLY) Mode=O_RDONLY;
    else Mode=O_RDWR;

    if (Flags & STREAM_APPEND) Mode |=O_APPEND;
    if (Flags & SF_CREATE) Mode |=O_CREAT;

    if (strcmp(Path,"-")==0)
    {
        if (Mode==O_RDONLY) fd=0;
        else fd=1;
    }
    else if (Flags & SF_TMPNAME)
    {
        //Must make a copy because mkostemp internally alters path
        NewPath=CopyStr(NewPath, Path);
#ifdef HAVE_MKOSTEMP
        fd=mkostemp(NewPath, Mode);
#else
        fd=mkstemp(NewPath);
#endif
        p_Path=NewPath;
    }
    //if path starts with a tilde, then it's the user's home directory
    else if (strncmp(Path, "~/", 2) ==0)
    {
        //Path+1 so we get the / to make sure there is one after HomeDir
        NewPath=MCopyStr(NewPath, GetCurrUserHomeDir(), Path+1, NULL);
        fd=open(NewPath, Mode, 0600);
        p_Path=NewPath;
    }
    else
    {
        fd=open(Path, Mode, 0600);
        p_Path=Path;
    }


    if (fd==-1)
    {
        if (Flags & SF_ERROR) RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "failed to open %s", p_Path);
        else RaiseError(ERRFLAG_ERRNO|ERRFLAG_DEBUG, "STREAMFileOpen", "failed to open %s", p_Path);
        Destroy(NewPath);
        return(NULL);
    }

    if (Flags & SF_WRLOCK)
    {
        if (flock(fd,LOCK_EX | LOCK_NB)==-1)
        {
            RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "file lock requested but failed %s", p_Path);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }
    }

    if (Flags & SF_RDLOCK)
    {
        if (flock(fd,LOCK_SH | LOCK_NB)==-1)
        {
            RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "file lock requested but failed %s", p_Path);
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
        if (lstat(p_Path, &myStat) !=0)
        {
            RaiseError(ERRFLAG_ERRNO, "STREAMFileOpen", "cannot stat %s", p_Path);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }

        if (S_ISLNK(myStat.st_mode))
        {
            RaiseError(0, "STREAMFileOpen", "%s is a symlink, but not not in 'symlink okay' mode", p_Path);
            close(fd);
            Destroy(NewPath);
            return(NULL);
        }
    }
    else
    {
        stat(p_Path, &myStat);
    }

    //CREATE THE STREAM OBJECT !!
    Stream=STREAMFromFD(fd);

    ptr=LibUsefulGetValue("STREAM:Timeout");
    if (StrValid(ptr)) STREAMSetTimeout(Stream, atoi(ptr));

    STREAMSetFlushType(Stream,FLUSH_FULL,0,0);
    Tempstr=FormatStr(Tempstr,"%d",myStat.st_size);
    STREAMSetValue(Stream, "FileSize", Tempstr);
    Stream->Size=myStat.st_size;
    Stream->Path=CopyStr(Stream->Path, p_Path);

    if ( (Flags & (SF_RDONLY | SF_MMAP)) == (SF_RDONLY | SF_MMAP) ) STREAMOpenMMap(Stream, 0, myStat.st_size, Flags);
    else
    {
        if (Flags & SF_TRUNC)
        {
            ftruncate(fd,0);
            STREAMSetValue(Stream, "FileSize", "0");
        }
        if (Flags & STREAM_APPEND) lseek(fd,0,SEEK_END);
    }


    Destroy(Tempstr);
    Destroy(NewPath);

    return(Stream);
}



int STREAMParseConfig(const char *Config)
{
    const char *ptr;
    int Flags=SF_RDWR;

    if (StrValid(Config))
    {
        ptr=Config;
        while (*ptr != '\0')
        {
            if (*ptr==' ') break;
            switch (*ptr)
            {
            case 'c':
                Flags |= SF_CREATE;
                break;
            case 'r':
                if (Flags & SF_WRONLY) Flags &= ~(SF_RDONLY | SF_WRONLY);
                else Flags |= SF_RDONLY;
                break;
            case 'w':
                if (Flags & SF_RDONLY) Flags &= ~(SF_RDONLY | SF_WRONLY);
                else Flags |= SF_WRONLY | SF_CREATE| SF_TRUNC;
                break;
            case 'a':
                Flags |= STREAM_APPEND | SF_CREATE;
                break;
            case 'E':
                Flags |= SF_ERROR;
                break;
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
            case 's':
                Flags |= SF_SECURE;
                break;
            case 'i':
                Flags |= SF_EXEC_INHERIT;
                break;
            case 'A':
                Flags |= STREAM_APPENDONLY;
                break;
            case 'I':
                Flags |= STREAM_IMMUTABLE;
                break;
            case 'F':
                Flags |= SF_FOLLOW;
                break;
            case 'S':
                Flags |= SF_SORTED;
                break;
            case 't':
                Flags |= SF_TMPNAME;
                break;
            case 'T':
                Flags |= SF_TLS_AUTO;
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



//URL can be a file path or a number of different network/file URL types
STREAM *STREAMOpen(const char *URL, const char *Config)
{
    STREAM *S=NULL;
    char *Proto=NULL, *Host=NULL, *Token=NULL, *User=NULL, *Pass=NULL, *Path=NULL, *Args=NULL;
    const char *ptr;
    int Port=0, Flags=0;


    Proto=CopyStr(Proto,"");
    ptr=STREAMExtractMasterURL(URL);
    ParseURL(ptr, &Proto, &Host, &Token, &User, &Pass, &Path, &Args);
    if (StrValid(Token)) Port=strtoul(Token,NULL,10);

    ptr=GetToken(Config,"\\S",&Token,0);
    Flags=STREAMParseConfig(Token);


    switch (*Proto)
    {
    case 'c':
        if (strcasecmp(Proto,"cmd")==0) S=STREAMSpawnCommand(URL+4, Config);
        else S=STREAMFileOpen(URL, Flags);
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
            S=STREAMFileOpen(ptr, Flags);
        }
        else S=STREAMFileOpen(URL, Flags);
        break;

    case 'g':
        if (strcasecmp(Proto, "gemini")==0) S=GeminiOpen(URL, Config);
        else S=STREAMFileOpen(URL, Flags);
        break;

    case 'h':
        if (
            (strcasecmp(Proto,"http")==0) ||
            (strcasecmp(Proto,"https")==0)
        )
        {
            S=HTTPWithConfig(URL, Config);
            //the 'write only' and 'read only' flags normally result in one or another
            //buffer not being allocated (as it's not expected to be needed). However
            //with HTTP 'write' means 'POST', and we still need both read and write
            //buffers to read from and to the server, so we must unset these flags
            Flags &= ~(SF_WRONLY | SF_RDONLY);
        }
        else S=STREAMFileOpen(URL, Flags);
        break;

    case 'm':
        if (strcasecmp(Proto,"mmap")==0) S=STREAMFileOpen(URL+5, Flags | SF_MMAP);
        else S=STREAMFileOpen(URL, Flags);
        break;

    case 't':
    case 's':
    case 'u':
        if ( (strcmp(URL,"-")==0) || (strcasecmp(URL,"stdio:")==0) ) S=STREAMFromDualFD(0,1);
        else if (strcasecmp(URL,"stdin:")==0) S=STREAMFromFD(0);
        else if (strcasecmp(URL,"stdout:")==0) S=STREAMFromFD(1);
        else if (strcasecmp(Proto,"ssh")==0) S=SSHOpen(Host, Port, User, Pass, Path, Flags);
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

    default:
        if (strcmp(URL,"-")==0) S=STREAMFromDualFD(0,1);
        else S=STREAMFileOpen(URL, Flags);
        break;
    }

    if (S)
    {
        S->Flags |= Flags;
        if (S->Flags & SF_SECURE) STREAMResizeBuffer(S, S->BuffSize);
        STREAMSetFlags(S, S->Flags, 0);
        if (Flags & SF_COMPRESSED)
        {
            if (Flags & SF_RDONLY) STREAMAddStandardDataProcessor(S, "decompress", "gzip", "");
            else if (Flags & SF_WRONLY) STREAMAddStandardDataProcessor(S, "compress", "gzip", "");
        }

        STREAMSetTimeout(S, LibUsefulGetInteger("STREAM:Timeout"));

        switch (S->Type)
        {
        case STREAM_TYPE_TCP:
        case STREAM_TYPE_UDP:
        case STREAM_TYPE_SSL:
        case STREAM_TYPE_HTTP:
        case STREAM_TYPE_CHUNKED_HTTP:
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
        if (strcmp(Curr->Tag, "LU:AssociatedStream")==0)
        {
            tmpS=(STREAM *) Curr->Item;
            STREAMClose(tmpS);
            ListDeleteNode(Curr);
        }
        else if (strcmp(Curr->Tag, "HTTP:InfoStruct")==0)
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
    ftruncate(S->out_fd, size);
}



//Some special features specifically around closing files. Currently these mostly concern telling the OS that a file
//doesn't require caching (maybe becasue it's a logfile rather than data)
void STREAMCloseFile(STREAM *S)
{
    if (
        (StrEnd(S->Path)) ||
        (strcmp(S->Path,"-") !=0) //don't do this for stdin/stdout
    )
    {
        if (S->out_fd != -1)
        {
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
}



void STREAMShutdown(STREAM *S)
{
    ListNode *Curr;
    int val;

    if (! S) return;

    //-1 means 'FLUSH'
    STREAMReadThroughProcessors(S, NULL, -1);
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
        if (strncmp(Curr->Tag,"HelperPID",9)==0)
        {
            val=atoi(Curr->Item);
            if (val > 1) kill(val, SIGKILL);
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


int STREAMReadCharsToBuffer(STREAM *S)
{
    fd_set selectset;
    int val=0, read_result=0, WaitForBytes=TRUE, saved_errno;
    long bytes_read;
    struct timeval tv;
    char *tmpBuff=NULL, *Peer=NULL;

    if (! S) return(0);

//we don't read from and embargoed stream. Embargoed is a state that we
//use to indicate a stream must be ignored for a while
    if (S->State & SS_EMBARGOED) return(0);

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

//if buffer is half full, or full 'cept for space at the start, then make room
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

    //if using SSL and already has bytes  queued, don't do a wait on select
    if ( (S->State & SS_SSL) && OpenSSLSTREAMCheckForBytes(S) ) WaitForBytes=FALSE;

    //must set this to 1 in case not doing a select, 'cos S->Timeout not set
    read_result=1;

    if ((S->Timeout > 0) && WaitForBytes)
    {
        FD_ZERO(&selectset);
        FD_SET(S->in_fd, &selectset);
        MillisecsToTV(S->Timeout * 10, &tv);
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


    //Here we perform the actual read
    if (read_result==1)
    {
        val=S->BuffSize - S->InEnd;
        tmpBuff=SetStrLen(tmpBuff,val);

        if (S->State & SS_SSL) bytes_read=OpenSSLSTREAMReadBytes(S, tmpBuff, val);
        else if (S->Type==STREAM_TYPE_UDP)
        {
            bytes_read=UDPRecv(S->in_fd,  tmpBuff, val, &Peer, NULL);
            saved_errno=errno;
            STREAMSetValue(S, "Peer", Peer);
            Destroy(Peer);
        }
        else
        {
            if (S->Flags & SF_RDLOCK) flock(S->in_fd,LOCK_SH);
            bytes_read=read(S->in_fd, tmpBuff, val);
            saved_errno=errno;
            if (S->Flags & SF_RDLOCK) flock(S->in_fd,LOCK_UN);
        }

        if (bytes_read > 0)
        {
            S->BytesRead+=bytes_read;
            read_result=1;
        }
        //Timeouts, EAGAIN, etc are handled via select, so if we
        //get here the stream has closed, even if we're being told
        //EAGAIN
        else read_result=STREAM_CLOSED;
    }

//messing with this block tends to break STREAMSendFile
    if (read_result > 0)
    {
        read_result=STREAMReadThroughProcessors(S, tmpBuff, bytes_read);
    }
    else if (read_result == STREAM_CLOSED)
    {
        //-1 means 'FLUSH'
        //there's no bytes in tmpBuff in this situation
        bytes_read=STREAMReadThroughProcessors(S, NULL, -1);
        if (bytes_read > 0) read_result=bytes_read;
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



int STREAMReadBytes(STREAM *S, char *Buffer, int Buffsize)
{
    int bytes=0, result=0, total=0;


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

    while (total < Buffsize)
    {
        total+=STREAMTransferBytesOut(S, Buffer+total, Buffsize-total);

        bytes=S->InEnd - S->InStart;

        if (bytes < 1)
        {
            //in testing, the best way to prevent doing constant checking for new bytes,
            //and so filling up the buffer, was to only check for new bytes if
            //we didn't have enough to satisfy another read like the one we just had

            //We must check for '< 1' rather than '-1' because
            if (S->Flags & SF_MMAP) result=-1;
            else result=FDCheckForBytes(S->in_fd);

            if (result ==-1)
            {
                if (total==0) total=STREAM_CLOSED;
                break;
            }
            if (result < 1) break;

            result=STREAMReadCharsToBuffer(S);
            if (result < 1)
            {
                if (total > 0) return(total);
                else return(result);
            }
        }


    }
    return(total);
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
            len=Mod->Write(Mod,ptr,len,OutData,OutLen,Flush);
            if (Flush && (len !=STREAM_CLOSED)) AllDataWritten=FALSE;
            if (Next)
            {
                TempBuff=SetStrLen(TempBuff, *OutLen);
                memcpy(TempBuff,*OutData,*OutLen);
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
            //Any decision about how much to write takes place in InternalFinalWrite
            result=STREAMInternalFinalWriteBytes(S, S->OutputBuff, S->OutEnd);
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
    if (S->State & SS_WRITE_ERROR) return(STREAM_CLOSED);


    i_data=Data;
    len=DataLen;

    if (ListSize(S->ProcessingModules))
    {
        STREAMInternalPushProcessingModules(S, i_data, len, &TempBuff, &len);
        i_data=TempBuff;
    }

//This always queues all the data, though it may not flush it all
//thus the calling application always believes all data is written
//Thus we only report errors if len==0;
    if (len > 0) result=STREAMInternalQueueBytes(S, i_data, len);
    else if (S->OutEnd > S->StartPoint) result=STREAMInternalFinalWriteBytes(S, S->OutputBuff, S->OutEnd);

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
            RetStr=SetStrLen(RetStr,bytes_read + len);
            len=STREAMTransferBytesOut(S, RetStr+bytes_read, len);
            bytes_read+=len;
            *(RetStr+bytes_read)='\0';

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
        //if ((len % 100)== 0) Tempptr=realloc(Tempptr,(len/100 +1) *100 +2);
        // *(Tempptr+len)=inchar;

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
    int result, bytes_read=0;

    if ( (S->Size > 0) && (! (S->State & SS_COMPRESSED)) )
    {
        RetStr=SetStrLen(RetStr, S->Size);
        while (bytes_read < S->Size)
        {
            result=STREAMReadBytes(S, RetStr+bytes_read,S->Size - bytes_read);
            if (result > 0) bytes_read+=result;
            else break;
        }
        RetStr[bytes_read]='\0';
    }
    else
    {
        RetStr=CopyStr(RetStr,"");
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            RetStr=CatStr(RetStr, Tempstr);
            Tempstr=STREAMReadLine(Tempstr, S);
        }
    }

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
    else result=strcmp(Item, Line);

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


#ifdef USE_SENDFILE
#include <sys/sendfile.h>

//if we are not using ssl and not using processor modules, we can use
//kernel-level copy!

    if (
        (Flags & SENDFILE_KERNEL) &&
        (! (In->State & SS_SSL)) &&
        (! (Out->State & SS_SSL)) &&
        (ListSize(In->ProcessingModules)==0) &&
        (ListSize(Out->ProcessingModules)==0)
    )
    {
        UseSendFile=TRUE;
        STREAMFlush(Out);
    }
#endif


    while (len > 0)
    {
#ifdef USE_SENDFILE
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
                STREAMInternalFinalWriteBytes(Out, Out->OutputBuff, val);
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
    void *Item;

    Item=STREAMGetItem(S, "HTTP:InfoStruct");
    if (Item)
    {
        if (HTTPTransact((HTTPInfoStruct *) Item) != NULL) return(TRUE);
    }

    return(FALSE);
}
