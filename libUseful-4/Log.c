#include "Log.h"
#include "Time.h"
#include "FileSystem.h"
#include <syslog.h>


//When logs are used by 'child processes', it's only the 'parent' process that should be
//able to delete/reopen the log. Otherwise the child can delete a log that the parent
//keeps writing to, and keeps handing to child processes. Thus we record the parent pid
static pid_t ParentPID=0;



static ListNode *LogFiles=NULL;
static TLogFile *LogFileDefaults=NULL;


STREAM *LogFileInternalDoRotate(TLogFile *LogFile);

ListNode *LogFilesGetList()
{
    return(LogFiles);
}

void LogFileSetDefaults(int Flags, int MaxSize, int MaxRotate, int FlushInterval)
{
    LogFileDefaults=(TLogFile *) calloc(1,sizeof(TLogFile));
    LogFileDefaults->Flags=Flags;
    LogFileDefaults->MaxSize=MaxSize;
    LogFileDefaults->MaxRotate=MaxRotate;
    LogFileDefaults->FlushInterval=FlushInterval;
    LogFileDefaults->LogFacility=LOG_USER;
    if (ParentPID==0) ParentPID=getpid();
}


STREAM *LogFileOpen(const char *Path, int Flags)
{
    STREAM *S;
    int StreamFlags=SF_CREAT | STREAM_APPEND | SF_WRONLY | SF_NOCACHE;

    if (Flags & LOGFILE_HARDEN) StreamFlags |= STREAM_APPENDONLY;
    MakeDirPath(Path, 0770);
    S=STREAMFileOpen(Path, StreamFlags);
    if (S)
    {
        STREAMSetFlushType(S, FLUSH_FULL, 0, 0);
        STREAMSeek(S, 0, SEEK_END); //just for paranoia's sake
    }
    return(S);
}


TLogFile *LogFileCreate(const char *FileName, int Flags)
{
    TLogFile *LogFile=NULL;
    STREAM *S=NULL;

    if (! StrLen(FileName)) return(NULL);
    if (! LogFiles) LogFiles=ListCreate();
    if (! LogFileDefaults) LogFileSetDefaults(LOGFILE_TIMESTAMP | LOGFILE_FLUSH | LOGFILE_LOGPID | LOGFILE_LOGUSER, 100000000, 0, 1);

    if (strcmp(FileName,"STDOUT")==0) S=STREAMFromFD(1);
    else if (strcmp(FileName,"STDERR")==0) S=STREAMFromFD(2);
    else if (strcmp(FileName,"SYSLOG")==0) S=STREAMCreate();
    else S=LogFileOpen(FileName, Flags);

    if (S)
    {
        LogFile=(TLogFile *) calloc(1,sizeof(TLogFile));
        LogFile->Path=CopyStr(LogFile->Path,FileName);
        if (Flags & LOGFILE_PLAIN) LogFile->Flags=Flags;
        else LogFile->Flags=LogFileDefaults->Flags | Flags;
        LogFile->LogFacility=LogFileDefaults->LogFacility;
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
    return(LogFile);
}


TLogFile *LogFileGetEntry(const char *FileName)
{
    TLogFile *LogFile=NULL;
    ListNode *Node;
    STREAM *S=NULL;

    if (! StrLen(FileName)) return(NULL);
    if (! LogFiles) LogFiles=ListCreate();
    if (! LogFileDefaults) LogFileSetDefaults(LOGFILE_TIMESTAMP | LOGFILE_FLUSH | LOGFILE_LOGPID | LOGFILE_LOGUSER, 100000000, 0, 1);

    Node=ListFindNamedItem(LogFiles,FileName);
    if (Node) LogFile=(TLogFile *) Node->Item;

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


void LogFileCloseAll()
{
    ListNode *Curr, *Next;
    TLogFile *LogFile;

    Curr=ListGetNext(LogFiles);
    while (Curr)
    {
        Next=ListGetNext(Curr);
        LogFile=(TLogFile *) Curr->Item;
        ListDeleteNode(Curr);
        DestroyString(LogFile->Path);
        STREAMClose(LogFile->S);
        free(LogFile);
        Curr=Next;
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

            if (LogFile->S)
            {
                //turn off append only sw we can do reanmae
                if (LogFile->Flags & LOGFILE_HARDEN) STREAMSetFlags(LogFile->S, 0, STREAM_APPENDONLY);
            }
            if (PrevPath) rename(LogFile->Path,PrevPath);
            if (LogFile->S)
            {
                //turn off append only sw we can do reanmae
                if (LogFile->Flags & LOGFILE_HARDEN) STREAMSetFlags(LogFile->S, STREAM_IMMUTABLE, 0);
            }

            if (LogFile->S) STREAMClose(LogFile->S);
            LogFile->S=LogFileOpen(LogFile->Path, LogFile->Flags);
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
    else
    {
        LogFile=LogFileGetEntry(FileName);
        if (! LogFile) LogFile=LogFileCreate(FileName, 0);
    }

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
    struct tm *TimeStruct;
    int result=FALSE;
    time_t Now;

    if (ParentPID==0) ParentPID=getpid();
    if (LF) S=LogFileInternalDoRotate(LF);
    if (! S) return(FALSE);

    if (Flags & LOGFILE_CACHETIME) Now=GetTime(TIME_CACHED);
    else Now=GetTime(0);

    if (Flags & LOGFILE_TIMESTAMP)
    {
        LogStr=CopyStr(LogStr, GetDateStr("%Y/%m/%d %H:%M:%S",NULL));

        if (Flags & LOGFILE_MILLISECS)
        {
            Tempstr=FormatStr(Tempstr,".%03d ", GetTime(TIME_CACHED | TIME_MILLISECS) % 1000);
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

        if (LF)
        {
            if ((Now - LF->LastFlushTime) > LF->FlushInterval) Flags |= LOGFILE_FLUSH;
            LF->LastMessageTime=Now;
            LF->LastMessage=CopyStr(LF->LastMessage, Str);
            if (Flags & LOGFILE_FLUSH) LF->LastFlushTime=Now;
        }

        STREAMWriteLine(LogStr,S);

        if (Flags & LOGFILE_FLUSH) STREAMFlush(S);

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




int LogFileInternalPush(TLogFile *LF, STREAM *S, int Flags, const char *Str)
{
    char *Tempstr=NULL;
    int result=TRUE;

    if (LF->Flags & LOGFILE_REPEATS)
    {
        if (strcmp(LF->LastMessage, Str)==0)
        {
            //LF->LastMessageTime=Now;
            LF->RepeatCount++;
        }
        else if (LF->RepeatCount > 0)
        {
            if (LF->RepeatCount==1) LogFileInternalWrite(LF, S, Flags, LF->LastMessage);
            else
            {
                Tempstr=FormatStr(Tempstr, "Last message repeated %d times", LF->RepeatCount);
                LogFileInternalWrite(LF, S, Flags, Tempstr);
            }
            LF->RepeatCount=0;
            LF->LastMessage=CopyStr(LF->LastMessage, Str);
            result=LogFileInternalWrite(LF, S, Flags, Str);
        }
    }
    else result=LogFileInternalWrite(LF, S, Flags, Str);

    DestroyString(Tempstr);

    return(result);
}




int LogToSTREAM(STREAM *S, int Flags, const char *Str)
{
    if (! S) return(FALSE);

    return(LogFileInternalPush(NULL, S, LOGFILE_FLUSH, Str));
}


int LogWrite(TLogFile *Log, const char *Str)
{
    return(LogFileInternalPush(Log, Log->S, Log->Flags, Str));
}


void LogFileFlushAll(int Flags)
{
    time_t Now;
    ListNode *Curr;
    TLogFile *LogFile;

    Now=GetTime(FALSE);
    Curr=ListGetNext(LogFiles);
    while (Curr)
    {
        LogFile=(TLogFile *) Curr->Item;

        if (Flags && LogFile->S)
        {
            STREAMFlush(LogFile->S);
            LogFile->LastFlushTime=Now;
            if (Flags & LOGFLUSH_REOPEN)
            {
                STREAMClose(LogFile->S);
                LogFile->S=LogFileOpen(LogFile->Path, LogFile->Flags);
            }
        }
        else if (LogFile->S && ((Now - LogFile->LastFlushTime) > LogFile->FlushInterval) )
        {
            STREAMFlush(LogFile->S);
            LogFile->LastFlushTime=Now;
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
    if (! LogFile) LogFile=LogFileCreate(FileName, 0);
    if (LogFile)
    {
        va_start(args, fmt);
        Tempstr=VFormatStr(Tempstr, fmt, args);
        va_end(args);
        StripTrailingWhitespace(Tempstr);
        result=LogFileInternalPush(LogFile,LogFile->S,LogFile->Flags, Tempstr);
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
    if (! LogFile) LogFile=LogFileCreate(LogPath, 0);
    LogFileClose(TmpLogPath);
    MakeDirPath(TmpLogPath, 0770);
    S=STREAMFileOpen(TmpLogPath,SF_RDONLY);
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

void LogFilesHousekeep()
{
    TLogFile *LogFile;
    ListNode *Curr;
    time_t Now;

    Now=GetTime(0);
    Curr=ListGetNext(LogFiles);
    while (Curr)
    {
        LogFile=(TLogFile *) Curr->Item;
        if ((LogFile->MarkInterval > 0) && (LogFile->NextMarkTime < Now))
        {
            if (LogFile->NextMarkTime > 0)
            {
                LogFileInternalWrite(LogFile, LogFile->S, LogFile->Flags | LOGFILE_CACHETIME, "----- MARK ----");
            }
            LogFile->NextMarkTime=Now+LogFile->MarkInterval;
        }

        LogFileInternalDoRotate(LogFile);
        Curr=ListGetNext(Curr);
    }
}
