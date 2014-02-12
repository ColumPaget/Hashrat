#ifndef LIBUSEFUL_FILEPATH_H
#define LIBUSEFUL_FILEPATH_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

char *GetBasename(char *Path);
char *SlashTerminateDirectoryPath(char *DirPath);
char *StripDirectorySlash(char *DirPath);
int FileExists(char *);
int MakeDirPath(char *Path, int DirMask);
int FindFilesInPath(char *File, char *Path, ListNode *Files);
char *FindFileInPath(char *InBuff, char *File, char *Path);
int ChangeFileExtension(char *FilePath, char *NewExt);
int FindFilesInPath(char *File, char *Path, ListNode *Files);

#ifdef __cplusplus
}
#endif



#endif
