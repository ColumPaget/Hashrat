#include "Log.h"
#include <syslog.h>


char *G_LogFilePath;


//When logs are used by 'child processes', it's only the 'parent' process that should be
//able to delete/reopen the log. Otherwise the child can delete a log that the parent
//keeps writing to, and keeps handing to child processes. Thus we record the parent pid
pid_t ParentPID=0;


typedef struct
{
	char *Path;
	int Flags;
	int MaxSize;
	STREAM *S;
	int LogFacility;
	int LastFlushTime;
	int FlushInterval;
} TLogFile;


ListNode *LogFiles=NULL;
TLogFile *LogFileDefaults=NULL;


void LogFileSetupDefaults()
{
	LogFileDefaults=(TLogFile *) calloc(1,sizeof(TLogFile));
	LogFileDefaults->MaxSize=100000000;
	LogFileDefaults->Flags |= LOGFILE_FLUSH | LOGFILE_LOGPID | LOGFILE_LOGUSER;
	LogFileDefaults->LogFacility=LOG_USER;
	if (ParentPID==0) ParentPID=getpid();
}

TLogFile *LogFileGetEntry(char *FileName)
{
	ListNode *Node;
	TLogFile *LogFile=NULL;
	STREAM *S=NULL;

	if (! StrLen(FileName)) return(NULL);
	if (! LogFiles) LogFiles=ListCreate();
	if (! LogFileDefaults) LogFileSetupDefaults();

	Node=ListFindNamedItem(LogFiles,FileName);
	if (Node) LogFile=(TLogFile *) Node->Item;
	else
	{
		if (strcmp(FileName,"STDOUT")==0) S=STREAMFromFD(1);
		else if (strcmp(FileName,"STDERR")==0) S=STREAMFromFD(2);
		else if (strcmp(FileName,"SYSLOG")==0) S=STREAMCreate();
		else
		{
			 S=STREAMOpenFile(FileName,O_CREAT | O_APPEND | O_WRONLY);
			 S->Flags &= ~FLUSH_ALWAYS;
		}

		if (S)
		{
			LogFile=(TLogFile *) calloc(1,sizeof(TLogFile));
			LogFile->Path=CopyStr(LogFile->Path,FileName);
			LogFile->LogFacility=LogFileDefaults->LogFacility;
			LogFile->Flags=LogFileDefaults->Flags;
			LogFile->MaxSize=LogFileDefaults->MaxSize;
			LogFile->S=S;
			if (strcmp(FileName,"SYSLOG")==0) LogFile->Flags |= LOGFILE_SYSLOG;
			ListAddNamedItem(LogFiles,FileName,LogFile);
			STREAMSetItem(S,"TLogFile",LogFile);
		}
	}

	return(LogFile);
}

void LogFileClose(char *Path)
{
ListNode *Node;
TLogFile *LogFile;

Node=ListFindNamedItem(LogFiles,Path);
if (Node)
{
LogFile=(TLogFile *) Node->Item;
ListDeleteNode(Node);
DestroyString(LogFile->Path);
STREAMClose(LogFile->S);
free(LogFile);
}
}

void LogFileInternalDoRotate(TLogFile *LogFile)
{
  struct stat FStat;
  char *Tempstr=NULL;

	if (! LogFile) return;
	if (strcmp(LogFile->Path,"SYSLOG")==0) return;
	if (strcmp(LogFile->Path,"STDOUT")==0) return;
	if (strcmp(LogFile->Path,"STDERR")==0) return;
	if (getpid() != ParentPID) return;

  if (LogFile->MaxSize > 0)
  {
  fstat(LogFile->S->out_fd,&FStat);
  if (FStat.st_size > LogFile->MaxSize)
  {
    Tempstr=MCopyStr(Tempstr,LogFile->Path,"-",NULL);
    rename(LogFile->Path,Tempstr);
    STREAMClose(LogFile->S);
    LogFile->S=STREAMOpenFile(LogFile->Path,O_CREAT | O_APPEND | O_WRONLY);
  }
  }

  DestroyString(Tempstr);
}



int LogFileSetValues(char *FileName, int Flags, int MaxSize, int FlushInterval)
{
	TLogFile *LogFile;

	if (! LogFileDefaults) LogFileSetupDefaults();
	if (ParentPID==0) ParentPID=getpid();
	if (StrLen(FileName)==0) LogFile=LogFileDefaults;
	else LogFile=LogFileGetEntry(FileName);

	if (LogFile)
	{
		LogFile->MaxSize=MaxSize;
		LogFile->FlushInterval=FlushInterval;
		LogFile->Flags=Flags;
		if (strcmp(FileName,"SYSLOG")==0) LogFile->Flags |= LOGFILE_SYSLOG;
	}
	return(TRUE);
}


int LogFileInternalWrite(STREAM *S,int LogLevel, int Flags, char *Str)
{
	char *Tempstr=NULL, *LogStr=NULL;
	struct timeval Now;
	struct tm *TimeStruct;
	int result=FALSE;
	TLogFile *Log;


		if (ParentPID==0) ParentPID=getpid();
		gettimeofday(&Now,NULL);
		TimeStruct=localtime(&Now.tv_sec);
		LogStr=SetStrLen(LogStr,40);
		strftime(LogStr,20,"%Y/%m/%d %H:%M:%S",TimeStruct);

		if (Flags & LOGFILE_MILLISECS) 
		{
	    Tempstr=FormatStr(Tempstr,".%03d ",Now.tv_usec / 1000);
			LogStr=CatStr(LogStr,Tempstr);
		}
		else LogStr=CatStr(LogStr," ");

		if (Flags & LOGFILE_LOGPID)
		{
			Tempstr=FormatStr(Tempstr,"[%d] ",getpid());
			LogStr=CatStr(LogStr,Tempstr);
		}

		if (Flags & LOGFILE_LOGUSER)
		{
			Tempstr=FormatStr(Tempstr,"user=%d ",getuid());
			LogStr=CatStr(LogStr,Tempstr);
		}
		LogStr=MCatStr(LogStr,Str,"\n",NULL);

		if (S)
		{

			Log=STREAMGetItem(S,"TLogFile");
			if (Log)
			{
			if (Log->Flags & LOGFILE_LOCK) S->Flags |= SF_WRLOCK;
			else S->Flags &= ~SF_WRLOCK;
			}

			STREAMWriteLine(LogStr,S);

			if (
					(! Log) ||
					((Flags & LOGFILE_FLUSH) || ((Now.tv_sec-Log->LastFlushTime) > Log->FlushInterval)) 
				)
			{
				STREAMFlush(S);
        if (Log) Log->LastFlushTime=Now.tv_sec;
			}
		
			result=TRUE;
		}

	if (Flags & LOGFILE_SYSLOG)
	{
		syslog(LOG_INFO,"%s",LogStr);
		result=TRUE;
	}

	DestroyString(Tempstr);
	DestroyString(LogStr);

	return(result);
}






int LogToSTREAM(STREAM *S, int Flags, char *Str)
{
if (! S) return(FALSE);

return(LogFileInternalWrite(S, 0, LOGFILE_FLUSH, Str));
}




void LogFileFlushAll(int Force)
{
	time_t Now;
	ListNode *Curr;
	TLogFile *Log;

	time(&Now);

	Curr=ListGetNext(LogFiles);
	while (Curr)
	{
	        Log=(TLogFile *) Curr->Item;

	        if (Force)
	        {
                STREAMFlush(Log->S);
                Log->LastFlushTime=Now;
	        }
	        else if ((Now - Log->LastFlushTime) > Log->FlushInterval)
	        {
                STREAMFlush(Log->S);
                Log->LastFlushTime=Now;
	        }
		Curr=ListGetNext(Curr);
	}
}


int LogToFile(char *FileName,char *fmt, ...)
{
	char *Tempstr=NULL;
	va_list args;
	int result=FALSE;
	TLogFile *LogFile;

	LogFile=LogFileGetEntry(FileName);
	if (LogFile)
	{
	LogFileInternalDoRotate(LogFile);

	va_start(args,fmt);
	Tempstr=VFormatStr(Tempstr,fmt,args);
	va_end(args);
	StripTrailingWhitespace(Tempstr);
	result=LogFileInternalWrite(LogFile->S,LOG_INFO, LogFile->Flags, Tempstr);
	}

DestroyString(Tempstr);
return(result);
}

int LogFileAppendTempLog(char *LogPath, char *TmpLogPath)
{
TLogFile *LogFile;
char *Tempstr=NULL;
STREAM *S;

    LogFile=LogFileGetEntry(LogPath);
    LogFileClose(TmpLogPath);
    S=STREAMOpenFile(TmpLogPath,O_RDONLY);
    if (LogFile && S)
    {

            STREAMLock(LogFile->S,LOCK_EX);
            Tempstr=STREAMReadLine(Tempstr,S);
            while(Tempstr)
            {
            STREAMWriteLine(Tempstr,LogFile->S);
            Tempstr=STREAMReadLine(Tempstr,S);
            }
            if (LogFile->Flags & LOGFILE_FLUSH) STREAMFlush(LogFile->S);
            STREAMLock(LogFile->S,LOCK_UN);
            unlink(TmpLogPath);
    }
		if (S) STREAMClose(S);

DestroyString(Tempstr);
}


void LogFileCheckRotate(char *FileName)
{
TLogFile *LogFile;

  LogFile=LogFileGetEntry(FileName);
  if (LogFile)
  {
  LogFileInternalDoRotate(LogFile);
	}
}
