/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_FILEPATH_H
#define LIBUSEFUL_FILEPATH_H

#include "includes.h"
#include "DataProcessing.h"

#ifdef __cplusplus
extern "C" {
#endif

//These functions relate to the filesystem


//copy the file at SrcPath to DestPath. If DestPath exists it will be overwritten, if it doesn't
//exist it will be created.
#define FileCopy(SrcPath, DestPath) (FileCopyWithProgress(SrcPath, DestPath, NULL))

//move fire to a directory, creating the diretory path if needed.
int FileMoveToDir(const char *FilePath, const char *Dir);

//copy SrcPath to DestPath, periodically calling the function 'Callback' to ouput information about
//copy progress. Look in 'DataProcessing.h' for more information about the Callback function.
unsigned long FileCopyWithProgress(const char *SrcPath, const char *DestPath, DATA_PROGRESS_CALLBACK Callback);

//replacement for the basename function. There are two different versions of basename that give
//different results, and basename reserves the right to modify the string that is passed in.
//GetBasename doesn't modify the string, and passes a pointer to the start of the filename
//with the Path string.
const char *GetBasename(const char *Path);


//if get_current_dir_name doesn't exist, then add it
#ifndef HAVE_GET_CURR_DIR
char *get_current_dir_name();
#endif

//Append a '/' to a libUseful style string if it doesn't already have one. As this function adds
//to the length of the string it should only be used with libUseful style strings
char *SlashTerminateDirectoryPath(char *DirPath);

//Remove a '/' from a libUSeful style string. returns the changed string
char *StripDirectorySlash(char *DirPath);

//clearer than '(access(File, F_OK)==0)'. returns TRUE if file exists, FALSE otherwise
int FileExists(const char *);

//Make an entire directory path. This is similar to the command-line 'mkdir -p'.
//Note, that this function will only make directories up to the last '/'. Thus you can pass it a
//full path to a file and it will make all the directories but not make one with the file name.
//If you want it to make a directory with the final name in the path then append a '/' to the end
//of the path.
int MakeDirPath(const char *Path, int DirMask);

//searches a ':' separated path (as in the PATH environment variable) for a file. If found, the full
//file path is copied into InBuff and returned. InBuff might be resized, so it should be a libUseful
//style string (see String.h). If the file cannot be found an empty string will be returned.
//The function returns the first matching file it finds.
char *FindFileInPath(char *InBuff, const char *File, const char *Path);

//Like 'FindFileInPath' except that the string in 'File' can be a glob/fnmatch/shell style pattern, so
//that more than one file may be matched. These file paths are returned as items in a libUseful style
//list (See List.h). The list should be destroyed with ListDestroy(Files, DestroyString);
int FindFilesInPath(const char *File, const char *Path, ListNode *Files);

//If a file ends with an extension like .tmp or .exe, then change it to NewExt. If it doesn't have
//an extension then add NewExt
int FileChangeExtension(const char *FilePath, const char *NewExt);


//Change owner and group of a file (handles looking up the uid and gid internally).
//return 'TRUE' on success, 'FALSE' otherwise
int FileChOwner(const char *Path, const char *Owner);
int FileChGroup(const char *Path, const char *Group);

//modify time access and modification times to imply it's been modified just now
int FileTouch(const char *Path);

//mount a file system. 'Type' is the filesystem type (ext2, xfs, etc). If 'Type' is set to 'bind' then
//on linux a 'bind mount' is performed. This mounts a directory onto another directory. In this case
//'Dev' is the source directory rather than the more usual device file. If the 'ro' option is passed in
//'Args' then a read-only bind mount is created.
//'Args' contains a space-seperated list of options. Available options are:
// ro, noatime, notdiratime, noexec, nosuid, nodev, and remount. These set the approrpriate flags in
//the mount commnd (see 'man 2 mount').

//If the directory specified by 'MountPoint' does not exist, FileSystemMount will create it, and any
//needed parent directories. The argument "perms=xxx" be passed with other options in 'Args' where 'xxx'
//is an octal expression of filesystem permissions for any created directories. The default is 0700
int FileSystemMount(const char *Dev, const char *MountPoint, const char *Type, const char *Args);


//unmount the file system mounted at 'MountPoint'. 'Args' can contain a space-seperated list of the
//following options:  'follow', 'lazy', 'detach', 'recurse', 'rmdir'
//'follow' will follow symbolic links and unmount directories they point to. 'lazy' and 'detach' will
//perform 'lazy unmounts', which mark the mount point for unmount if it's busy and carry out the
//unmount when it ceases to be busy. 'recurse' will umount any filesystems mounted within the mount
//point (rather than just failing with 'file system busy' and 'rmdir' will remove the mount-point
//directory after unmounting
int FileSystemUnMount(const char *MountPoint, const char *Args);




//File system extended attributes support. This allows storing values in the file system against a filename.
//This is currently a linux-only feature, though other OS attributes may be added in future.
//N.B. the ext2, ext3 and ext4 filesystems limit the size of a value to the blocksize. xfs has no such limit.
//The names used to store such attributes always have a prefix, usually 'user.' or 'trusted.'.
//'trusted' attributes are only available to programs with the CAP_SYS_ADMIN capability (this usually means
//only programs running as root) 'user' attributes are accessible by all users.

//e.g.  FileSetAttr("/etc/data.crypt","trusted.password","mysecret");


//Get 'binary' attribute. This can be binary data rather than text. 'RetStr' must be a pointer to a libUseful
//dynamically allocated string (see Strings.h or Overview.md for details of libUseful strings). The function
//will resize the string as needed to fit the data. returns number of bytes placed in RetStr
int FileGetBinaryXAttr(char **RetStr, const char *Path, const char *Name);

//Get Text attribute. 'RetStr' must be a libUseful dymamically allocated string. String is resized as needed
//data copied into it, then returned.
char *FileGetXAttr(char *RetStr, const char *Path, const char *Name);

//Set 'binary' attribute. 'Value' can hold binary data like, for instance, a .bmp image.
int FileSetBinaryXAttr(const char *Path, const char *Name, const char *Value, int Len);

//Set text attribute
int FileSetXAttr(const char *Path, const char *Name, const char *Value);

//recursive copy of a directory
int FileSystemCopyDir(const char *Src, const char *Dest);

int FileSystemRmDir(const char *Dir);

#ifdef __cplusplus
}
#endif



#endif
