#include "Log.h"
#include "Time.h"
#include <syslog.h>



//When logs are used by 'child processes', it's only the 'parent' process that should be
//able to delete/reopen the log. Otherwise the child can delete a log that the parent
//keeps writing to, and keeps handing to child processes. Thus we record the parent pid
pid_t ParentPID=0;



ListNode *LogFiles=NULL;
TLogFile *LogFileDefaults=NULL;


STREAM *LogFileInternalDoRotate(TLogFile *LogFile);


int LogFileSetDefaults(int Flags, int MaxSize, int MaxRotate, int FlushInterval)
{
	LogFileDefaults=(TLogFile *) calloc(1,sizeof(TLogFile));
	LogFileDefaults->Flags=Flags;
	LogFileDefaults->MaxSize=MaxSize;
	LogFileDefaults->MaxRotate=MaxRotate;
	LogFileDefaults->FlushInterval=FlushInterval;
	LogFileDefaults->LogFacility=LOG_USER;
	if (ParentPID==0) ParentPID=getpid();
}

TLogFile *LogFileGetEntry(const char *FileName)
{
	ListNode *Node;
	TLogFile *LogFile=NULL;
	STREAM *S=NULL;

	if (! StrLen(FileName)) return(NULL);
	if (! LogFiles) LogFiles=ListCreate();
	if (! LogFileDefaults) LogFileSetDefaults(LOGFILE_TIMESTAMP | LOGFILE_FLUSH | LOGFILE_LOGPID | LOGFILE_LOGUSER, 100000000, 0, 1);

	Node=ListFindNamedItem(LogFiles,FileName);
	if (Node) LogFile=(TLogFile *) Node->Item;
	else
	{
		if (strcmp(FileName,"STDOUT")==0) S=STREAMFromFD(1);
		else if (strcmp(FileName,"STDERR")==0) S=STREAMFromFD(2);
		else if (strcmp(FileName,"SYSLOG")==0) S=STREAMCreate();
		else
		{
			S=STREAMOpenFile(FileName,SF_CREAT | SF_APPEND | SF_WRONLY | SF_NOCACHE);
		}

		if (S)
		{
			LogFile=(TLogFile *) calloc(1,sizeof(TLogFile));
			LogFile->Path=CopyStr(LogFile->Path,FileName);
			LogFile->LogFacility=LogFileDefaults->LogFacility;
			LogFile->Flags=LogFileDefaults->Flags;
			LogFile->MaxSize=LogFileDefaults->MaxSize;
			LogFile->MaxRotate=LogFileDefaults->MaxRotate;
			LogFile->FlushInterval=LogFileDefaults->FlushInterval;
			LogFile->S=S;

			if (strcmp(FileName,"SYSLOG")==0) LogFile->Flags |= LOGFILE_SYSLOG;
			ListAddNamedItem(LogFiles,FileName,LogFile);
			STREAMSetItem(S,"TLogFile",LogFile);
			STREAMSetFlushType(S, FLUSH_FULL, 0, 0);

			//it might already be too big!
			LogFile->S=LogFileInternalDoRotate(LogFile);
		}
	}

	return(LogFile);
}



void LogFileClose(const char *Path)
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


char *LogFileInternalGetRotateDestinationPath(char *RetStr, TLogFile *LogFile)
{
char *Tempstr=NULL;

	Tempstr=CopyStr(Tempstr, STREAMGetValue(LogFile->S,"RotatePath"));
	if (StrLen(Tempstr) && strchr(Tempstr,'$'))
	{
		STREAMSetValue(LogFile->S, "Date",GetDateStr("%Y_%m_%d",NULL));
		STREAMSetValue(LogFile->S, "Time",GetDateStr("%H:%M:%S",NULL));
		RetStr=SubstituteVarsInString(RetStr,Tempstr,LogFile->S->Values,0);
	}
	else RetStr=CopyStr(RetStr,LogFile->Path);

	DestroyString(Tempstr);

return(RetStr);
}


STREAM *LogFileInternalDoRotate(TLogFile *LogFile)
{
  struct stat FStat;
  char *Tempstr=NULL, *Path=NULL, *PrevPath=NULL;
	int i;

	if (! LogFile) return(NULL);
	if (! LogFile->S) return(NULL);
	if (strcmp(LogFile->Path,"SYSLOG")==0) return(LogFile->S);
	if (strcmp(LogFile->Path,"STDOUT")==0) return(LogFile->S);
	if (strcmp(LogFile->Path,"STDERR")==0) return(LogFile->S);
	if (getpid() != ParentPID) return(LogFile->S);

  if (LogFile->MaxSize > 0)
  {
  if (LogFile->S) fstat(LogFile->S->out_fd,&FStat);
  else stat(LogFile->Path,&FStat);
  if (FStat.st_size > LogFile->MaxSize)
  {
		Tempstr=LogFileInternalGetRotateDestinationPath(Tempstr, LogFile);
		for (i=LogFile->MaxRotate; i > 0; i--)
		{
			Path=FormatStr(Path,"%s.%d",Tempstr,i);
			if (i==LogFile->MaxRotate) unlink(Path);
			else rename(Path,PrevPath);
			PrevPath=CopyStr(PrevPath,Path);
		}
			
    if (LogFile->S) STREAMClose(LogFile->S);
		if (PrevPath) rename(LogFile->Path,PrevPath);
    LogFile->S=STREAMOpenFile(LogFile->Path,SF_CREAT | SF_APPEND | SF_WRONLY | SF_NOCACHE);
		if (LogFile->S) STREAMSetFlushType(LogFile->S, FLUSH_FULL, 0, 0);
  }
  }

  DestroyString(PrevPath);
  DestroyString(Tempstr);
  DestroyString(Path);
	return(LogFile->S);
}



void LogFileSetValues(TLogFile *LogFile, int Flags, int MaxSize, int MaxRotate, int FlushInterval)
{
	if (! LogFileDefaults) LogFileSetDefaults(LOGFILE_TIMESTAMP | LOGFILE_FLUSH | LOGFILE_LOGPID | LOGFILE_LOGUSER, 100000000, 0, 1);
	if (ParentPID==0) ParentPID=getpid();

	if (LogFile)
	{
		LogFile->MaxSize=MaxSize;
		LogFile->MaxRotate=MaxRotate;
		LogFile->FlushInterval=FlushInterval;
		LogFile->Flags=Flags;
	}
}


int LogFileFindSetValues(const char *FileName, int Flags, int MaxSize, int MaxRotate, int FlushInterval)
{
	TLogFile *LogFile;

	if (StrLen(FileName)==0) LogFile=LogFileDefaults;
	else LogFile=LogFileGetEntry(FileName);

	if (LogFile) 
	{
		LogFileSetValues(LogFile, Flags, MaxSize, MaxRotate, FlushInterval);
		if (strcmp(FileName,"SYSLOG")==0) LogFile->Flags |= LOGFILE_SYSLOG;
	}
	else return(FALSE);

	return(TRUE);
}


int LogFileInternalWrite(TLogFile *LF, STREAM *S, int Flags, const char *Str)
{
	char *Tempstr=NULL, *LogStr=NULL;
	struct timeval Now;
	struct tm *TimeStruct;
	int result=FALSE;


		if (ParentPID==0) ParentPID=getpid();
		if (LF) S=LogFileInternalDoRotate(LF);
		if (! S) return(FALSE);

		gettimeofday(&Now,NULL);
		if (Flags & LOGFILE_TIMESTAMP)
		{
			TimeStruct=localtime(&Now.tv_sec);
			LogStr=SetStrLen(LogStr,40);
			strftime(LogStr,20,"%Y/%m/%d %H:%M:%S",TimeStruct);

			if (Flags & LOGFILE_MILLISECS) 
			{
		    Tempstr=FormatStr(Tempstr,".%03d ",Now.tv_usec / 1000);
				LogStr=CatStr(LogStr,Tempstr);
			}
			else LogStr=CatStr(LogStr," ");
		}

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
			if (Flags & LOGFILE_LOCK) S->Flags |= SF_WRLOCK;
			else S->Flags &= ~SF_WRLOCK;

			if (LF && ((Now.tv_sec-LF->LastFlushTime) > LF->FlushInterval)) Flags |= LOGFILE_FLUSH;
			STREAMWriteLine(LogStr,S);

			if (Flags & LOGFILE_FLUSH) 
			{
				STREAMFlush(S);
			}
		
			result=TRUE;
		}

	if (Flags & LOGFILE_SYSLOG)
	{
		syslog(LOG_INFO,"%s",LogStr);
		result=TRUE;
	}
	if (LF) LF->LastFlushTime=Now.tv_sec;

	DestroyString(Tempstr);
	DestroyString(LogStr);

	return(result);
}






int LogToSTREAM(STREAM *S, int Flags, const char *Str)
{
if (! S) return(FALSE);

return(LogFileInternalWrite(NULL, S, LOGFILE_FLUSH, Str));
}


int LogWrite(TLogFile *Log, const char *Str)
{
return(LogFileInternalWrite(Log, Log->S, Log->Flags, Str));
}


void LogFileFlushAll(int Force)
{
	time_t Now;
	ListNode *Curr;
	TLogFile *Log;

	Now=GetTime(FALSE);
	Curr=ListGetNext(LogFiles);
	while (Curr)
	{
	        Log=(TLogFile *) Curr->Item;

	        if (Force && Log->S)
	        {
                STREAMFlush(Log->S);
                Log->LastFlushTime=Now;
	        }
	        else if (Log->S && ((Now - Log->LastFlushTime) > Log->FlushInterval) )
	        {
                STREAMFlush(Log->S);
                Log->LastFlushTime=Now;
	        }
		Curr=ListGetNext(Curr);
	}
}



int LogToFile(const char *FileName, const char *fmt, ...)
{
	char *Tempstr=NULL;
	va_list args;
	int result=FALSE, val;
	TLogFile *LogFile;

	LogFile=LogFileGetEntry(FileName);
	if (LogFile)
	{
	va_start(args,fmt);
	Tempstr=VFormatStr(Tempstr,fmt,args);
	va_end(args);
	StripTrailingWhitespace(Tempstr);
	result=LogFileInternalWrite(LogFile,LogFile->S,LogFile->Flags, Tempstr);
	}

DestroyString(Tempstr);
return(result);
}



int LogFileAppendTempLog(const char *LogPath, const char *TmpLogPath)
{
TLogFile *LogFile;
char *Tempstr=NULL;
STREAM *S;
int result=FALSE;

    LogFile=LogFileGetEntry(LogPath);
    LogFileClose(TmpLogPath);
    S=STREAMOpenFile(TmpLogPath,SF_RDONLY);
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
						result=TRUE;
    }
		if (S) STREAMClose(S);

DestroyString(Tempstr);

return(result);
}


void LogFileCheckRotate(const char *FileName)
{
TLogFile *LogFile;

  LogFile=LogFileGetEntry(FileName);
  if (LogFile)
  {
  LogFileInternalDoRotate(LogFile);
	}
}
