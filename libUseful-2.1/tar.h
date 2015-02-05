#ifndef LIBUSEFUL_TAR_H
#define LIBUSEFUL_TAR_H

#include "includes.h"

#ifdef __cplusplus
extern "C" {
#endif

int TarReadHeader(STREAM *S, ListNode *Vars);
void TarUnpack(STREAM *Tar);
void TarWriteHeader(STREAM *S, char *FileName, struct stat *FStat);
void TarWriteFooter(STREAM *Tar);
void TarAddFile(STREAM *Tar, STREAM *File);
void TarFiles(STREAM *Tar, char *FilePattern);


#ifdef __cplusplus
}
#endif


#endif
