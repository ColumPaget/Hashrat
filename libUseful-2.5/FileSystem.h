#ifndef LIBUSEFUL_FILEPATH_H
#define LIBUSEFUL_FILEPATH_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

char *GetBasename(char *Path);
char *SlashTerminateDirectoryPath(char *DirPath);
char *StripDirectorySlash(char *DirPath);
int FileExists(const char *);
int MakeDirPath(const char *Path, int DirMask);
int FindFilesInPath(const char *File, const char *Path, ListNode *Files);
char *FindFileInPath(char *InBuff, const char *File, const char *Path);
int ChangeFileExtension(const char *FilePath, const char *NewExt);
int FindFilesInPath(const char *File, const char *Path, ListNode *Files);


int FileNotifyInit(const char *Path, int Flags);
int FileNotifyGetNext(int fd, char **Path);


#ifdef __cplusplus
}
#endif



#endif
