#include "FileSystem.h"
#include "Errors.h"
#include "Users.h"
#include <glob.h>
//#include <sys/ioctl.h>
//#include <sys/resource.h>
#include <sys/mount.h>
#include <utime.h>
#include <time.h>

#ifdef HAVE_XATTR
#include <sys/xattr.h>
#endif

#ifdef USE_FANOTIFY
#include <sys/fanotify.h>
#endif


#ifndef HAVE_GET_CURR_DIR
char *get_current_dir_name()
{
    char *path;

    path=(char *) calloc(1, PATH_MAX+1);
    getcwd(path, PATH_MAX+1);
    return(path);
}
#endif


const char *GetBasename(const char *Path)
{
    const char *ptr;
    int len;

    len=StrLen(Path);
    if (len==0) return("");
    if (len==1) return(Path);

    ptr=Path+len-1;
    while (ptr > Path)
    {
        if ((*ptr=='/') && (*(ptr+1) != '\0')) break;
        ptr--;
    }

    if ((*ptr=='/') && (*(ptr+1) != '\0')) ptr++;

    return(ptr);
}


char *SlashTerminateDirectoryPath(char *DirPath)
{
    char *ptr, *RetStr=NULL;

    if (! StrValid(DirPath)) return(CopyStr(DirPath,"/"));
    RetStr=DirPath;
    ptr=RetStr+StrLen(RetStr)-1;
    if (*ptr != '/') RetStr=AddCharToStr(RetStr,'/');

    return(RetStr);
}


char *StripDirectorySlash(char *DirPath)
{
    char *ptr;

//don't strip '/' (root dir)
    if (! StrValid(DirPath)) return(DirPath);
    if (strcmp(DirPath,"/")==0) return(DirPath);
    ptr=DirPath+StrLen(DirPath)-1;

    if (*ptr == '/') *ptr='\0';

    return(DirPath);
}


int MakeDirPath(const char *Path, int DirMask)
{
    const char *ptr;
    char *Tempstr=NULL;
    int result=-1;

    ptr=Path;
    if (*ptr=='/') ptr++;
    ptr=strchr(ptr, '/');
    while (ptr)
    {
        Tempstr=CopyStrLen(Tempstr,Path,ptr-Path);
        result=mkdir(Tempstr, DirMask);
        if ((result==-1) && (errno != EEXIST))
        {
            RaiseError(ERRFLAG_ERRNO, "MakeDirPath", "cannot mkdir '%s'",Tempstr);
            break;
        }
        ptr=strchr(++ptr, '/');
    }
    DestroyString(Tempstr);
    if (result==0) return(TRUE);
    return(FALSE);
}



int FileChangeExtension(const char *FilePath, const char *NewExt)
{
    char *ptr;
    char *Tempstr=NULL;
    int result;

    Tempstr=CopyStr(Tempstr,FilePath);
    ptr=strrchr(Tempstr,'/');
    if (!ptr) ptr=Tempstr;
    ptr=strrchr(ptr,'.');
    if (! ptr) ptr=Tempstr+StrLen(Tempstr);
    StrTrunc(Tempstr, ptr-Tempstr);

    if (*NewExt=='.') Tempstr=CatStr(Tempstr,NewExt);
    else Tempstr=MCatStr(Tempstr,".",NewExt,NULL);
    result=rename(FilePath,Tempstr);
    if (result !=0) RaiseError(ERRFLAG_ERRNO, "FileChangeExtension", "cannot rename '%s' to '%s'",FilePath, Tempstr);

    DestroyString(Tempstr);
    if (result==0) return(TRUE);
    else return(FALSE);
}


int FileMoveToDir(const char *FilePath, const char *Dir)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=MCopyStr(Tempstr, Dir, "/", GetBasename(FilePath), NULL);
    MakeDirPath(Tempstr, 0700);
    result=rename(FilePath,Tempstr);
    if (result !=0) RaiseError(ERRFLAG_ERRNO, "FileMoveToDir", "cannot rename '%s' to '%s'",FilePath, Tempstr);

    DestroyString(Tempstr);

    if (result==0) return(TRUE);
    else return(FALSE);
}


int FindFilesInPath(const char *File, const char *Path, ListNode *Files)
{
    char *Tempstr=NULL, *CurrPath=NULL;
    const char *ptr;
    int i;
    glob_t Glob;

    if (*File=='/')
    {
        CurrPath=CopyStr(CurrPath,"");
        ptr=""; //so we execute once below
    }
    else ptr=GetToken(Path,":",&CurrPath,0);

    while (ptr)
    {
        CurrPath=SlashTerminateDirectoryPath(CurrPath);
        Tempstr=MCopyStr(Tempstr,CurrPath,File,NULL);

        glob(Tempstr,0,0,&Glob);
        for (i=0; i < Glob.gl_pathc; i++) ListAddItem(Files,CopyStr(NULL,Glob.gl_pathv[i]));
        globfree(&Glob);

        ptr=GetToken(ptr,":",&CurrPath,0);
    }

    DestroyString(Tempstr);
    DestroyString(CurrPath);

    return(ListSize(Files));
}



char *FindFileInPath(char *InBuff, const char *File, const char *Path)
{
    char *Tempstr=NULL, *CurrPath=NULL, *RetStr=NULL;
    const char *ptr;

    RetStr=CopyStr(InBuff,"");

    if (*File=='/')
    {
        CurrPath=CopyStr(CurrPath,"");
        ptr=""; //so we execute once below
    }
    else ptr=GetToken(Path,":",&CurrPath,0);

    while (ptr)
    {
        CurrPath=SlashTerminateDirectoryPath(CurrPath);
        Tempstr=MCopyStr(Tempstr,CurrPath,File,NULL);
        if (access(Tempstr,F_OK)==0)
        {
            RetStr=CopyStr(RetStr,Tempstr);
            break;
        }

        ptr=GetToken(ptr,":",&CurrPath,0);
    }

    DestroyString(Tempstr);
    DestroyString(CurrPath);

    return(RetStr);
}


/* This checks if a certain file exists (not if we can open it etc, just if */
/* we can stat it, this is useful for checking pid files etc).              */
int FileExists(const char *FileName)
{
    struct stat StatData;

    if (stat(FileName,&StatData) == 0) return(1);
    else return(0);
}


int FileChOwner(const char *FileName, const char *Owner)
{
    int uid, result;

    uid=LookupUID(Owner);
    if (uid > -1)
    {
        result=chown(FileName, uid, -1);
        if (result==0) return(TRUE);
    }
    RaiseError(ERRFLAG_ERRNO, "FileChOwner", "failed to change owner to user=%s uid=%d",Owner,uid);
    return(FALSE);
}



int FileChGroup(const char *FileName, const char *Group)
{
    int gid, result;

    gid=LookupGID(Group);
    if (gid > -1)
    {
        result=chown(FileName, -1, gid);
        if (result==0) return(TRUE);
    }
    RaiseError(ERRFLAG_ERRNO, "FileChGroup", "failed to change group to group=%s gid=%d",Group,gid);
    return(FALSE);
}


int FileChMod(const char *Path, const char *Mode)
{
    int perms;

    perms=FileSystemParsePermissions(Mode);
    if (chmod(Path, perms) ==0) return(TRUE);
    return(FALSE);
}



int FileTouch(const char *Path)
{
    struct utimbuf times;

    times.actime=time(NULL);
    times.modtime=time(NULL);

    if (utime(Path, &times)==0) return(TRUE);

    RaiseError(ERRFLAG_ERRNO, "FileTouch", "failed to update file mtime");
    return(FALSE);
}

unsigned long FileCopyWithProgress(const char *SrcPath, const char *DestPath, DATA_PROGRESS_CALLBACK Callback)
{
    STREAM *Src;
    unsigned long result;
    struct stat FStat;

    Src=STREAMOpen(SrcPath,"r");
    if (! Src) return(0);

    if (Callback) STREAMAddProgressCallback(Src,Callback);
    result=STREAMCopy(Src, DestPath);
    STREAMClose(Src);
    if (stat(SrcPath, &FStat)==0) chmod(DestPath, FStat.st_mode);

    return(result);
}


int FileGetBinaryXAttr(char **RetStr, const char *Path, const char *Name)
{
    int len=0;

    *RetStr=CopyStr(*RetStr, "");
#ifdef HAVE_XATTR
#ifdef __APPLE__ //'cos some idiot's always got to 'think different'
    len=getxattr(Path, Name, NULL, 0, 0, 0);
#else
    len=getxattr(Path, Name, NULL, 0);
#endif

    if (len > 0)
    {
        *RetStr=SetStrLen(*RetStr,len);
#ifdef __APPLE__
        getxattr(Path, Name, *RetStr, len, 0, 0);
#else
        getxattr(Path, Name, *RetStr, len);
#endif
    }
#else
    RaiseError(0, "FileGetXAttr", "xattr support not compiled in");
#endif

    return(len);
}

char *FileGetXAttr(char *RetStr, const char *Path, const char *Name)
{
    int len;

    len=FileGetBinaryXAttr(&RetStr, Path, Name);
    if (len > 0) RetStr[len]=0;

    return(RetStr);
}


int FileSetBinaryXAttr(const char *Path, const char *Name, const char *Value, int Len)
{
#ifdef HAVE_XATTR
#ifdef __APPLE__
    return(setxattr(Path, Name, Value, Len, 0, 0));
#else
    return(setxattr(Path, Name, Value, Len, 0));
#endif
#else
    RaiseError(0, "FileSetXAttr", "xattr support not compiled in");
#endif

    return(-1);
}


int FileSetXAttr(const char *Path, const char *Name, const char *Value)
{
    return(FileSetBinaryXAttr(Path, Name, Value, StrLen(Value)));
}


int FileSystemMount(const char *Dev, const char *MountPoint, const char *Type, const char *Args)
{
    const char *ptr, *p_Type, *p_MountPoint;
    char *Token=NULL;
    int Flags=0, result, Perms=700;

    p_Type=Type;
    if (! StrValid(MountPoint))
    {
        p_MountPoint=Dev;
        if (*p_MountPoint) p_MountPoint++;
    }
    else p_MountPoint=MountPoint;

    if (! StrValid(p_MountPoint)) return(FALSE);

    if (strcmp(Type,"bind")==0)
    {
#ifdef MS_BIND
        p_Type="";
        Flags=MS_BIND;
#else
        RaiseError(0, "FileSystemMount", "Bind mounts not supported on this system.");
        return(FALSE);
#endif
    }


    ptr=GetToken(Args, " |,", &Token, GETTOKEN_MULTI_SEP);
    while (ptr)
    {
#ifdef MS_RDONLY
        if (strcmp(Token,"ro")==0) Flags |= MS_RDONLY;
#endif

#ifdef MS_NOATIME
        if (strcmp(Token,"noatime")==0) Flags |= MS_NOATIME;
#endif

#ifdef MS_NOATIME
        if (strcmp(Token,"nodiratime")==0) Flags |= MS_NODIRATIME;
#endif

#ifdef MS_NOEXEC
        if (strcmp(Token,"noexec")==0) Flags |= MS_NOEXEC;
#endif

#ifdef MS_NOSUID
        if (strcmp(Token,"nosuid")==0) Flags |= MS_NOSUID;
#endif

#ifdef MS_NODEV
        if (strcmp(Token,"nodev")==0) Flags |= MS_NODEV;
#endif

#ifdef MS_REMOUNT
        if (strcmp(Token,"remount")==0) Flags |= MS_REMOUNT;
#endif


        if (strncmp(Token,"perms=",6)==0) Perms=strtol(Token+6,NULL,8);
        ptr=GetToken(ptr, " |,", &Token, GETTOKEN_MULTI_SEP);
    }

    Token=MCopyStr(Token,p_MountPoint,"/",NULL);
    MakeDirPath(Token,Perms);

//must do a little dance for readonly bind mounts. We must first mount, then remount readonly
#ifdef MS_BIND
    if ((Flags & MS_BIND) && (Flags & MS_RDONLY))
    {
        mount(Dev,p_MountPoint,"",MS_BIND,NULL);
        Flags |= MS_REMOUNT;
    }
#endif

#ifdef linux
    result=mount(Dev,p_MountPoint,p_Type,Flags,NULL);
#else
//assume BSD if not linux
    result=mount(p_Type,p_MountPoint,Flags,(void *) Dev);
#endif


    if (result !=0) RaiseError(ERRFLAG_ERRNO, "FileSystemMount", "failed to mount %s:%s on %s", p_Type,Dev,p_MountPoint);

    DestroyString(Token);

    if (result==0) return(TRUE);
    return(FALSE);
}



//if the system doesn't have these flags then define empty values for them
#ifndef UMOUNT_NOFOLLOW
#define UMOUNT_NOFOLLOW 0
#endif

#ifndef MNT_DETACH
#define MNT_DETACH 0
#endif


#define UMOUNT_RECURSE  1
#define UMOUNT_SUBDIRS  2
#define UMOUNT_RMDIR    4
#define UMOUNT_RMFILE   8


int FileSystemUnMountFlagsDepth(const char *MountPoint, int UnmountFlags, int ExtraFlags, int Depth, int MaxDepth)
{
    int result, i;
    char *Path=NULL;
    struct stat FStat;
    glob_t Glob;

    if (strcmp(MountPoint,"/proc")==0) MaxDepth=10;
    if (strcmp(MountPoint,"/sys")==0) MaxDepth=10;
    if (strcmp(MountPoint,"/dev")==0) MaxDepth=10;



    if (ExtraFlags & UMOUNT_RECURSE)
    {
        if ((MaxDepth ==0) || (Depth <= MaxDepth))
        {
            Path=MCopyStr(Path,MountPoint,"/*",NULL);
            glob(Path, 0, 0, &Glob);
            for (i=0; i < Glob.gl_pathc; i++)
            {
                if (stat(Glob.gl_pathv[i],&FStat)==0)
                {
                    if (S_ISDIR(FStat.st_mode))
                    {
                        FileSystemUnMountFlagsDepth(Glob.gl_pathv[i], UnmountFlags, ExtraFlags & ~UMOUNT_SUBDIRS, Depth+1, MaxDepth);
                    }
                    else if (ExtraFlags & UMOUNT_RMFILE) unlink(Glob.gl_pathv[i]);
                }
            }
            globfree(&Glob);
        }
    }

    if (ExtraFlags & UMOUNT_SUBDIRS) return(0);

    result=0;
    while (result > -1)
    {
#ifdef HAVE_UMOUNT2
        result=umount2(MountPoint, UnmountFlags);
#elif HAVE_UMOUNT
        result=umount(MountPoint);
#elif HAVE_UNMOUNT
        result=unmount(MountPoint,0);
#else
        result=-1;
#endif
    }

    if (ExtraFlags & UMOUNT_RMDIR) rmdir(MountPoint);

    Destroy(Path);
    return(result);
}


int FileSystemUnMountFlags(const char *MountPoint, int UnmountFlags, int ExtraFlags)
{
    return(FileSystemUnMountFlagsDepth(MountPoint, UnmountFlags, ExtraFlags, 0, 0));
}

int FileSystemUnMount(const char *MountPoint, const char *Args)
{
    int Flags=UMOUNT_NOFOLLOW;
    int ExtraFlags=0;
    char *Token=NULL;
    const char *ptr;
    int result;

    ptr=GetToken(Args, " |,", &Token, GETTOKEN_MULTI_SEP);
    while (ptr)
    {
        if (strcmp(Token,"follow")==0) Flags &= ~UMOUNT_NOFOLLOW;
        if (strcmp(Token,"lazy")==0) Flags |= MNT_DETACH;
        if (strcmp(Token,"detach")==0) Flags |= MNT_DETACH;
        if (strcmp(Token,"recurse")==0) ExtraFlags |= UMOUNT_RECURSE;
        if (strcmp(Token,"subdirs")==0) ExtraFlags |= UMOUNT_SUBDIRS | UMOUNT_RECURSE;
        if (strcmp(Token,"rmdir")==0) ExtraFlags |= UMOUNT_RMDIR;

        ptr=GetToken(ptr, " |,", &Token, GETTOKEN_MULTI_SEP);
    }

    result=FileSystemUnMountFlags(MountPoint, Flags, ExtraFlags);
    DestroyString(Token);

    return(result);
}


int FileSystemRmDir(const char *Dir)
{
    return(FileSystemUnMountFlagsDepth(Dir, 0, UMOUNT_RECURSE | UMOUNT_RMDIR | UMOUNT_RMFILE, 0, 0));
}


int FileSystemCopyDir(const char *Src, const char *Dest)
{
    glob_t Glob;
    const char *ptr;
    struct stat Stat;
    char *Tempstr=NULL, *Path=NULL;
    int i, result, RetVal=FALSE;

    Tempstr=MCopyStr(Tempstr, Dest, "/", NULL);
    MakeDirPath(Tempstr, 0777);
    Tempstr=MCopyStr(Tempstr, Src, "/*", NULL);
    glob(Tempstr, 0, 0, &Glob);
    Tempstr=MCopyStr(Tempstr, Src, "/.*", NULL);
    glob(Tempstr, GLOB_APPEND, NULL, &Glob);
    for (i=0; i < Glob.gl_pathc; i++)
    {
        ptr=GetBasename(Glob.gl_pathv[i]);

        if ((strcmp(ptr,".") !=0) && (strcmp(ptr, "..") !=0) )
        {
            ptr=Glob.gl_pathv[i];
            if (lstat(ptr, &Stat) == 0)
            {
                RetVal=TRUE;
                Path=MCopyStr(Path, Dest, "/", GetBasename(ptr), NULL);
                if (S_ISLNK(Stat.st_mode))
                {
                    Tempstr=SetStrLen(Tempstr, PATH_MAX);
                    result=readlink(ptr, Tempstr, PATH_MAX);
                    StrTrunc(Tempstr, result);
                    result=symlink(Path, Tempstr);
                }
                else if (S_ISDIR(Stat.st_mode))
                {
                    FileSystemCopyDir(ptr, Path);
                }
                else if (S_ISREG(Stat.st_mode))
                {
                    FileCopy(ptr, Path);
                }
                else
                {
                    RaiseError(0, "FileSystemCopyDir", "WARNING: not copying %s. Files of this type aren't yet supported for copy", Src);
                }
            }
        }
    }

    globfree(&Glob);

    Destroy(Tempstr);
    Destroy(Path);

    return(RetVal);
}


static int FileSystemParsePermissionsTri(const char **ptr, int ReadPerm, int WritePerm, int ExecPerm)
{
    int Perms=0;

    if (**ptr=='+') ptr++;
    if (**ptr=='=') ptr++;
    if (**ptr=='r') Perms |= ReadPerm;
    if (ptr_incr(ptr, 1) ==0) return(Perms);
    if (**ptr=='w') Perms |= WritePerm;
    if (ptr_incr(ptr, 1) ==0) return(Perms);
    if (**ptr=='x') Perms |= ExecPerm;
    if (ptr_incr(ptr, 1) ==0) return(Perms);

    return(Perms);
}


int FileSystemParsePermissions(const char *PermsStr)
{
    int Perms=0;
    const char *ptr;

    if (! StrValid(PermsStr)) return(0);
    ptr=PermsStr;

    switch(*ptr)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        return(strtol(PermsStr,NULL,8));
        break;

    case 'a':
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IRUSR|S_IRGRP|S_IROTH, S_IWUSR|S_IWGRP|S_IWOTH, S_IXUSR|S_IXGRP|S_IXOTH);
        break;

    case 'u':
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IRUSR, S_IWUSR, S_IXUSR);
        break;

    case 'g':
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IRGRP, S_IWGRP, S_IXGRP);
        break;

    case 'o':
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IROTH, S_IWOTH, S_IXOTH);
        break;

    default:
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IRUSR, S_IWUSR, S_IXUSR);
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IRGRP, S_IWGRP, S_IXGRP);
        Perms |= FileSystemParsePermissionsTri(&ptr, S_IROTH, S_IWOTH, S_IXOTH);
        break;
    }

    return(Perms);
}

