#include "includes.h"
#include "DataProcessing.h"
#include "pty.h"
#include "expect.h"

#ifdef HAVE_LIBSSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif


int FDSelect(int fd, int Flags, struct timeval *tv)
{
fd_set *readset=NULL, *writeset=NULL;
int result, RetVal=0;


if (Flags & SELECT_READ)
{
  readset=(fd_set *) calloc(1,sizeof(fd_set));
  FD_ZERO(readset);
  FD_SET(fd, readset);
}

if (Flags & SELECT_WRITE)
{
  writeset=(fd_set *) calloc(1,sizeof(fd_set));
  FD_ZERO(writeset);
  FD_SET(fd, writeset);
}


result=select(fd+1,readset,writeset,NULL,tv);
if ((result==-1) && (errno==EBADF)) RetVal=0;
else if (result  > 0)
{
    if (readset && FD_ISSET(fd, readset)) RetVal |= SELECT_READ;
    if (writeset && FD_ISSET(fd, writeset)) RetVal |= SELECT_WRITE;
}

if (readset) free(readset);
if (writeset) free(writeset);

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

/*Set non blocking IO on a stream*/
void STREAMSetNonBlock(STREAM *S, int val)
{
int flags=0;

	if (val) S->Flags |= SF_NONBLOCK;
  else S->Flags &= (~SF_NONBLOCK);


  fcntl(S->in_fd,F_GETFL,&flags);
  if (val) flags |= O_NONBLOCK;
  else flags &= (~O_NONBLOCK);

  fcntl(S->in_fd, F_SETFL, flags);
}


/*Set timeout for select calls within STREAM*/
void STREAMSetTimeout(STREAM *S, int val)
{
S->Timeout=val;
}


/*Set flush type for STREAM*/
void STREAMSetFlushType(STREAM *S, int Type, int val)
{
S->Flags &= ~(FLUSH_ALWAYS | FLUSH_LINE | FLUSH_BLOCK);
S->Flags |= Type;
S->BlockSize=val;
}

/* This reads chunks from a file and when if finds a newline it resets */
/* the file pointer to that position */
void STREAMResizeBuffer(STREAM *Stream, int size)
{
   Stream->InputBuff=(char *) realloc(Stream->InputBuff,size);
   Stream->OutputBuff=(char *) realloc(Stream->OutputBuff,size);
   Stream->BuffSize=size;
   if (Stream->InStart > Stream->BuffSize) Stream->InStart=0;
   if (Stream->InEnd > Stream->BuffSize) Stream->InEnd=0;
   if (Stream->OutEnd > Stream->BuffSize) Stream->OutEnd=0;
}




int STREAMCheckForBytes(STREAM *S)
{
  if (! S) return(0);
	if (S->State & SS_EMBARGOED) return(0);
  if (S->InEnd > S->InStart) return(1);
  if (S->in_fd==-1) return(0);
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
	if (read_result==STREAM_CLOSED) return(EOF);
	if (read_result==STREAM_DATA_ERROR) return(EOF);
	return(0);
}

STREAM *STREAMSelect(ListNode *Streams, struct timeval *tv)
{
 fd_set SelectSet;
 STREAM *S;
 ListNode *Curr;
 int highfd=0, result;
 
 FD_ZERO(&SelectSet);
 
 Curr=ListGetNext(Streams);
 while (Curr)
 {
  S=(STREAM *) Curr->Item;
	if (! (S->State & SS_EMBARGOED))
	{
	//Pump any data in the stream
	 STREAMFlush(S);
   if (S->InEnd > S->InStart) return(S);
   FD_SET(S->in_fd,&SelectSet);
   if (S->in_fd > highfd) highfd=S->in_fd;
	}

  Curr=ListGetNext(Curr);
 }
   
 result=select(highfd+1,&SelectSet,NULL,NULL,tv);
 
 if (result > 0)
 {
   Curr=ListGetNext(Streams);
   while (Curr)
   {
     S=(STREAM *) Curr->Item;
    if (FD_ISSET(S->in_fd,&SelectSet)) return(S);
     Curr=ListGetNext(Curr);
   }
 }
 
 return(NULL);
}


int STREAMCheckForWaitingChar(STREAM *S,unsigned char check_char)
{
int read_result=0, result, trash;
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

if (read_result==STREAM_CLOSED) return(EOF);
if (read_result==STREAM_DATA_ERROR) return(EOF);
return(FALSE);
}



int STREAMInternalFinalWriteBytes(STREAM *S, const char *Data, int DataLen)
{
fd_set selectset;
int result, count=0;
struct timeval tv;

if (! S) return(STREAM_CLOSED);
if (S->out_fd==-1) return(STREAM_CLOSED);


while (count < DataLen)
{
if (S->Flags & SF_SSL)
{
#ifdef HAVE_LIBSSL

result=SSL_write((SSL *) STREAMGetItem(S,"LIBUSEFUL-SSL-CTX"), Data + count, DataLen - count);
#endif
}
else
{
	if (S->Timeout > 0)
	{
		FD_ZERO(&selectset);
		FD_SET(S->out_fd, &selectset);
		tv.tv_sec=S->Timeout;
		tv.tv_usec=0;
		result=select(S->out_fd+1,NULL,&selectset,NULL,&tv);

		if (result==-1)  return(STREAM_CLOSED);
		if ((result == 0) || (! FD_ISSET(S->out_fd, &selectset))) return(STREAM_TIMEOUT);
	}

	if (S->Flags & SF_WRLOCK) flock(S->out_fd,LOCK_EX);
  result=write(S->out_fd, Data + count, DataLen - count);
	if (S->Flags & SF_WRLOCK) flock(S->out_fd,LOCK_UN);
}

  if (result < 1 && ((errno !=EINTR) && (errno !=EAGAIN)) ) break;
  if (result < 0) result=0;
  count+=result;
  S->BytesWritten+=result;
}


//memmove any remaining data so that we add onto the end of it
S->OutEnd -= count;
if (S->OutEnd > 0) memmove(S->OutputBuff,S->OutputBuff+count, S->OutEnd);


return(count);
}




int STREAMFlush(STREAM *S)
{
	STREAMWriteBytes(S,NULL,0);
}

void STREAMClear(STREAM *S)
{
 STREAMFlush(S);
 S->InStart=0;
}


/*A stream can have a series of 'processor modules' associated with it' */
/*which do things to the data before it is read/written. This function  */
/*pumps the data through the processor list, and eventually writes it out */
int STREAMReadThroughProcessors(STREAM *S, char *Bytes, int InLen)
{
TProcessingModule *Mod;
ListNode *Curr;
char *InBuff=NULL, *OutputBuff=NULL;
int len=0, olen=0, state=STREAM_CLOSED;


len=InLen;

if (InLen > 0)
{
InBuff=SetStrLen(InBuff,len+1);
memcpy(InBuff,Bytes ,len);
}

Curr=ListGetNext(S->ProcessingModules);
while (Curr)
{
	Mod=(TProcessingModule *) Curr->Item;
	if (len < BUFSIZ) olen=BUFSIZ;
	else olen=len * 8;

	OutputBuff=SetStrLen(OutputBuff,olen);

	if (Mod->Read) 
	{
		len=Mod->Read(Mod,InBuff,len,&OutputBuff,&olen,FALSE);
		if (len != EOF) state=0;

		if (len > 0)
		{
		InBuff=SetStrLen(InBuff,len);
		memcpy(InBuff,OutputBuff,len);
		}
	}

	Curr=ListGetNext(Curr);
}

if (
			(! (S->State & SS_DATA_ERROR)) &&
			(len > 0)
		)
{
//Whatever happened above, InBuff should now contain the data to be written!
//note that we resize buff to S->InEnd + len, where len is length of the new
//data. Even if S->InStart > 0 (meaning there are 'sent' bytes in the buffer)
//we consider S->InStart to be 0 as regards sizeing the buffer, because those
//sent bytes are still there.
S->InputBuff=SetStrLen(S->InputBuff,len + S->InEnd );
memcpy(S->InputBuff + S->InEnd, InBuff, len);
S->InEnd+=len;
}

DestroyString(OutputBuff);
DestroyString(InBuff);

len=S->InEnd - S->InStart;

if (len==0) 
{
	if (state ==STREAM_CLOSED) return(STREAM_CLOSED);
	if (S->State & SS_DATA_ERROR) return(STREAM_DATA_ERROR);
}
return(len);
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
STREAMResizeBuffer(S,4096);
S->in_fd=-1;
S->out_fd=-1;
S->Timeout=30;
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


STREAM *STREAMOpenFile(const char *FilePath, int Flags)
{
int fd, Mode=FALSE;
STREAM *Stream;
struct stat myStat;

Mode = Flags & ~(O_LOCK|O_TRUNC);

if (strcmp(FilePath,"-")==0)
{
Stream=STREAMFromDualFD(0,1);
Stream->Path=CopyStr(Stream->Path,FilePath);
return(Stream);
}

fd=open(FilePath, Mode, 0600);
if (fd==-1) return(NULL);

if (Flags & O_LOCK)
{
	if (flock(fd,LOCK_EX | LOCK_NB)==-1)
	{
		close(fd);
		return(NULL);
	}

}

// check for symlink naughtyness. Basically a malicious user can
// try to guess the name of the file we are going to open in order
// to get us to write somewhere other than intended.


if (lstat(FilePath, &myStat) !=0)
{
  close(fd);
  return(NULL);
}

if (S_ISLNK(myStat.st_mode))
{
	syslog(LOG_USER | LOG_WARNING, "STREAMOpenFile Opened symlink when trying to open %s. Possible DOS attack?",FilePath);
  close(fd);
  return(NULL);
}

if (Flags & O_TRUNC) ftruncate(fd,0);

Stream=STREAMFromFD(fd);
Stream->Path=CopyStr(Stream->Path,FilePath);
STREAMSetTimeout(Stream,0);
STREAMSetFlushType(Stream,FLUSH_FULL,0);

return(Stream);
}


STREAM *STREAMClose(STREAM *S)
{
ListNode *Curr;
int len;

if (! S) return(NULL);
len=S->OutEnd; 

STREAMReadThroughProcessors(S, NULL, 0);
STREAMFlush(S);

if ( 
		(StrLen(S->Path)==0) ||
	 (strcmp(S->Path,"-") !=0)
	)
{
if ((S->out_fd != -1) && (S->out_fd != S->in_fd)) close(S->out_fd);
if (S->in_fd != -1) close(S->in_fd);
}

Curr=ListGetNext(S->Values);
while (Curr)
{
if (strncmp(Curr->Tag,"HelperPID",9)==0) kill(atoi(Curr->Item),SIGKILL);
Curr=ListGetNext(Curr);
}


ListDestroy(S->Values,(LIST_ITEM_DESTROY_FUNC)DestroyString);
ListDestroy(S->ProcessingModules,DataProcessorDestroy);
DestroyString(S->InputBuff);
DestroyString(S->OutputBuff);
DestroyString(S->Path);
free(S);

return(NULL);
}


int STREAMDisassociateFromFD(STREAM *Stream)
{
int fd;

if (! Stream) return(-1);
fd=Stream->in_fd;
STREAMFlush(Stream);
DestroyString(Stream->InputBuff);
DestroyString(Stream->OutputBuff);
DestroyString(Stream->Path);
free(Stream);
return(fd);
}




int STREAMReadCharsToBuffer(STREAM *S)
{
fd_set selectset;
int result=0, diff, read_result=0, WaitForBytes=TRUE;
struct timeval tv;
char *tmpBuff=NULL;
int v1, v2,v3;
void *SSL_CTX=NULL;

if (! S) return(0);

if (S->State & SS_EMBARGOED) return(0);

SSL_CTX=STREAMGetItem(S,"LIBUSEFUL-SSL-CTX");

if (S->InStart >= S->InEnd)
{
S->InEnd=0;
S->InStart=0;
}
diff=S->InEnd-S->InStart;

if (S->InStart > (S->BuffSize / 2))
{
  memmove(S->InputBuff,S->InputBuff + S->InStart,diff);
  S->InStart=0;
  S->InEnd=diff;
}

v1=S->InStart; v2=S->InEnd; v3=S->BuffSize;

//if no room in buffer, we can't read in more bytes
if (S->InEnd >= S->BuffSize) return(1);


//if there are bytes available in the internal OpenSSL buffers, when we don't have to 
//wait on a select, we can just go straight through to SSL_read
#ifdef HAVE_LIBSSL
if (S->Flags & SF_SSL)
{
if (SSL_pending((SSL *) SSL_CTX) > 0) WaitForBytes=FALSE;
}
//else
#endif


//if ((S->Timeout > 0) && (! (S->Flags & SF_NONBLOCK)) && WaitForBytes)
if ((S->Timeout > 0) && WaitForBytes)
{
   FD_ZERO(&selectset);
   FD_SET(S->in_fd, &selectset);
   tv.tv_sec=S->Timeout;
   tv.tv_usec=0;
   result=select(S->in_fd+1,&selectset,NULL,NULL,&tv);


	switch (result)
	{
		//we are only checking one FD, so should be 1
		case 1:
		 read_result=0;
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

//must do this, as we need it to be 0 if we don't do the reads
result=0;

if (read_result==0)
{
	tmpBuff=SetStrLen(tmpBuff,S->BuffSize-S->InEnd);

	#ifdef HAVE_LIBSSL
	if (S->Flags & SF_SSL)
	{
		read_result=SSL_read((SSL *) SSL_CTX, tmpBuff, S->BuffSize-S->InEnd);
	}
	else
	#endif
	{
		if (S->Flags & SF_RDLOCK) flock(S->in_fd,LOCK_SH);
		read_result=read(S->in_fd, tmpBuff, S->BuffSize-S->InEnd);
		if (S->Flags & SF_RDLOCK) flock(S->in_fd,LOCK_UN);
	}

	if (read_result > 0) 
	{
		result=read_result;
		S->BytesRead+=read_result;
	}
	else
	{
        if ((read_result == -1) && (errno==EAGAIN)) read_result=STREAM_NODATA;
        else read_result=STREAM_CLOSED;
        result=0;
	}
}

if (result !=0) read_result=result;
if (result < 0) result=0;
read_result=STREAMReadThroughProcessors(S, tmpBuff, result);
//if (result==STREAM_DATA_ERROR) read_result=STREAM_DATA_ERROR;

//We are not returning number of bytes read. We only return something if
//there is a condition (like socket close) where the thing we are waiting for 
//may not appear

DestroyString(tmpBuff);
return(read_result);
}






int STREAMReadBytes(STREAM *S, char *Buffer, int Buffsize)
{
char *ptr=NULL;
int bytes=0, result=0, total=0;

ptr=Buffer;

if (S->InStart >= S->InEnd) 
{
  result=STREAMReadCharsToBuffer(S);
	if (S->InStart >= S->InEnd)
	{
	  if (result==STREAM_CLOSED) return(EOF);
	  if (result==STREAM_TIMEOUT) return(STREAM_TIMEOUT);
	  if (result==STREAM_DATA_ERROR) return(STREAM_DATA_ERROR);
	}
}

while (total < Buffsize)
{

	bytes=S->InEnd - S->InStart;
	
	
	if (bytes > (Buffsize-total)) bytes=(Buffsize-total);
	
	memcpy(ptr+total,S->InputBuff+S->InStart,bytes);
	S->InStart+=bytes;
	total+=bytes;
	
	bytes=S->InEnd - S->InStart;
	
	
	if (bytes < 1) 
	{
		//in testing, the best way to prevent doing constant checking for new bytes,
		//and so filling up the buffer, was to only check for new bytes if
		//we didn't have enough to satisfy another read like the one we just had
	
		//We must check for '< 1' rather than '-1' because 
		result=FDCheckForBytes(S->in_fd);

		if (result ==-1) 
		{
			if (total==0) total=EOF;
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



double STREAMTell(STREAM *S)
{
double pos;

if (S->OutEnd > 0) STREAMFlush(S);

#ifdef _LARGEFILE64_SOURCE
pos=(double) lseek64(S->in_fd,0,SEEK_CUR);
#else
pos=(double) lseek(S->in_fd,0,SEEK_CUR);
#endif
pos-=(S->InEnd-S->InStart);

return(pos);
}


double STREAMSeek(STREAM *S, double offset, int whence)
{
double pos;
int wherefrom;

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
pos=(double) lseek64(S->in_fd,(off64_t) pos, wherefrom);
#else
pos=(double) lseek(S->in_fd,(off_t) pos, wherefrom);
#endif


return(pos);
}

/*A stream can have a series of 'processor modules' associated with it' */
/*which do things to the data before it is read/written. This function  */
/*pumps the data through the processor list, and eventually writes it out */
int STREAMWriteBytes(STREAM *S,const char *Data, int DataLen)
{
TProcessingModule *Mod;
ListNode *Curr;
const char *i_data;
char *TempBuff=NULL, *ptr;
int TempLen=BUFSIZ;
int len, i_len, o_len, written=0, result=0;
int AllDataWritten=FALSE, Flush=FALSE;


if (! S) return(STREAM_CLOSED);
if (S->out_fd==-1) return(STREAM_CLOSED);
if (S->State & SS_WRITE_ERROR) return(STREAM_CLOSED);

if (DataLen==0) Flush=TRUE;

i_data=Data;
i_len=DataLen;

//if there are no processing modules, then this will be untouched, and will be our output
if (DataLen > TempLen) TempLen=DataLen;
TempBuff=SetStrLen(TempBuff,TempLen);
if (DataLen > 0) memcpy(TempBuff,Data,DataLen);
o_len=DataLen;


while (! AllDataWritten)
{
  AllDataWritten=TRUE;

	//Go through processing modules feeding the data from the previous one into them
  Curr=ListGetNext(S->ProcessingModules);
  while (Curr)
  {
     Mod=(TProcessingModule *) Curr->Item;


     	if (Mod->Write && ((i_len > 0) || Flush)) 
			{	
				o_len=Mod->Write(Mod,i_data,i_len,&TempBuff,&TempLen,Flush);
		 		if (Flush && (o_len !=EOF)) AllDataWritten=FALSE;
			}
		 i_data=TempBuff;
		 i_len=o_len;

     Curr=ListGetNext(Curr);
  }


	//Whatever happened above, o_data/o_len should hold data to be written
	ptr=TempBuff;
	
	while (o_len > 0)
	{
		len=S->BuffSize - S->OutEnd;
		if (len > o_len) len =o_len;
	
		memcpy(S->OutputBuff+S->OutEnd,ptr,len);
		ptr+=len;
		o_len-=len;
		S->OutEnd+=len;
	
		//Buffer Full, Write some bytes!!!
		if ((S->OutEnd >= S->BuffSize) || (S->BlockSize && (S->OutEnd > S->BlockSize)))
		{
			if (S->BlockSize) len=(S->OutEnd / S->BlockSize) * S->BlockSize;
			else len=S->OutEnd;
	
	    if (len > 0) result=STREAMInternalFinalWriteBytes(S, S->OutputBuff, len);
		  if (result==0) written=STREAM_TIMEOUT;
	    if (result < 1)
	    {
	         S->State |= SS_WRITE_ERROR;
					 written=STREAM_CLOSED;
	         break;
	    }
	
			written+=result;	
		}
	}
	
	//Must do this to avoid sending data into the queue multiple times!
	i_len=0;
	
}


//We always claim to have written the data that we've accepted
//Though this can be overridden below
written=DataLen;



//if we are told to write zero bytes, that's a flush
if ((S->OutEnd > 0) && ((DataLen==0) || (S->Flags & FLUSH_ALWAYS)))
{
	//if we are flushing blocks, then pad out to the blocksize
	if (S->Flags & FLUSH_BLOCK) 
	{
		len=(S->OutEnd / S->BlockSize) * S->BlockSize;
		if (S->OutEnd > len) len+=S->BlockSize;
		memset(S->OutputBuff+S->OutEnd,0,len - S->OutEnd);
		S->OutEnd=len;
	}

	result=STREAMInternalFinalWriteBytes(S, S->OutputBuff, S->OutEnd);
	if (result < 0) written=result;
}


DestroyString(TempBuff);


return(written);
}




int STREAMWriteString(const char *Buffer, STREAM *S)
{
int result;

if (StrLen(Buffer) < 1) return(FALSE);
result=STREAMWriteBytes(S,Buffer,strlen(Buffer));
return(result);
}

int STREAMWriteLine(const char *Buffer, STREAM *S)
{
int result;

if (StrLen(Buffer) < 1) return(FALSE);
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


int STREAMWriteChar(STREAM *S,unsigned char inchar)
{
char tmpchar;

tmpchar=inchar;
return(STREAMWriteBytes(S,&tmpchar,1));
}



int STREAMReadBytesToTerm(STREAM *S, char *Buffer, int BuffSize,unsigned char Term)
{
int inchar, pos=0;

inchar=STREAMReadChar(S);
while (inchar != EOF) 
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

if ((pos==0) && (inchar==EOF)) return(EOF);
return(pos);
}


char *STREAMReadToTerminator(char *Buffer, STREAM *S,unsigned char Term)
{
int inchar, len=0;
char *Tempptr;

Tempptr=CopyStr(Buffer,"");

inchar=STREAMReadChar(S);
while (inchar != EOF)
{
	//if ((len % 100)== 0) Tempptr=realloc(Tempptr,(len/100 +1) *100 +2);
	//*(Tempptr+len)=inchar;

		if (inchar >= 0)
		{
    	Tempptr=AddCharToBuffer(Tempptr,len,(char) inchar);
			len++;
		}
		else if (inchar !=STREAM_NODATA) break;
    if (inchar==Term) break;
    inchar=STREAMReadChar(S);
}


*(Tempptr+len)='\0';
//if ((inchar==EOF) && (errno==ECONNREFUSED)) return(Tempptr);
if (
	((inchar==EOF) || (inchar==STREAM_DATA_ERROR))
	&& 
	(StrLen(Tempptr)==0)
   )
{
  free(Tempptr);
  return(NULL);
}

return(Tempptr);
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
	//*(Tempptr+len)=inchar;

		if (inchar > 0)
		{
    	Tempptr=AddCharToBuffer(Tempptr,len,(char) inchar);
			len++;

    	if (strchr(Terms,inchar)) break;
		}
    inchar=STREAMReadChar(S);
}


*(Tempptr+len)='\0';

//if ((inchar==EOF) && (errno==ECONNREFUSED)) return(Tempptr);
if (
	((inchar==EOF) || (inchar==STREAM_DATA_ERROR))
	&& 
	(StrLen(Tempptr)==0)
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



char *STREAMGetValue(STREAM *S, const char *Name)
{
ListNode *Curr;

if (! S->Values) return(NULL);
Curr=ListFindNamedItem(S->Values,Name);
if (Curr) return(Curr->Item);
return(NULL);
}


void STREAMSetValue(STREAM *S, const char *Name, const char *Value)
{

if (! S->Values) S->Values=ListCreate();
SetVar(S->Values,Name,Value);
}

void *STREAMGetItem(STREAM *S, const char *Name)
{
ListNode *Curr;

if (! S->Items) return(NULL);
Curr=ListFindNamedItem(S->Items,Name);
if (Curr) return(Curr->Item);
return(NULL);
}


void STREAMSetItem(STREAM *S, const char *Name, void *Value)
{
ListNode *Curr;

if (! S->Items) S->Items=ListCreate();
Curr=ListFindNamedItem(S->Items,Name);
if (Curr) Curr->Item=Value;
else ListAddNamedItem(S->Items,Name,Value);
}
