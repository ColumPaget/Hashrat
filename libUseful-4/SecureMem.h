/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/

#ifndef LIBUSEFUL_SECUREMEM_H
#define LIBUSEFUL_SECUREMEM_H

#define SMEM_LOCK 1
#define SMEM_NODUMP 2
#define SMEM_NOFORK 4
#define SMEM_RDONLY 8
#define SMEM_WRONLY 16
#define SMEM_NOACCESS  32
#define SMEM_MADV_DONTFORK 4096
#define SMEM_SECURE (SMEM_LOCK | SMEM_NOFORK | SMEM_NODUMP)
#define SMEM_PARANOID (SMEM_SECURE | SMEM_NOACCESS)

#define SecureStoreUnlock(SS) SecureLockMem(SS->Data, SS->Size, SMEM_RDONLY)
#define SecureStoreLock(SS) SecureLockMem(SS->Data, SS->Size, SMEM_NOACCESS)

#include <sys/types.h>

typedef struct
{
    int Flags;
    int Used;
    int Size;
		pid_t OwnerPid;
    unsigned char *Data;
    unsigned char *CurrLine;

//Divisor is a single character that divides up fields in the data
    char Divisor;
} SECURESTORE;


#ifdef __cplusplus
extern "C" {
#endif

void SecureClearMem(unsigned char *Mem, int Size);
void SecureDestroy(unsigned char *Mem, int Size);
int SecureRealloc(unsigned char **OldMem, int OldSize, int NewSize, int Flags);

SECURESTORE *SecureStoreCreate(int Size);
void SecureStoreDestroy(SECURESTORE *SS);
SECURESTORE *SecureStoreLoad(const char *Path);

int CredsStoreLoad(const char *Path);
int CredsStoreAdd(const char *Realm, const char *User, const char *Cred);
int CredsStoreLookup(const char *Realm, const char *User, const char **Pass);
void CredsStoreDestroy();
void CredsStoreOnFork();


#ifdef __cplusplus
}
#endif


#endif
