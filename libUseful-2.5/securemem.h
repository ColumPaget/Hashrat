#ifndef LIBUSEFUL_SECUREMEM_H
#define LIBUSEFUL_SECUREMEM_H

#define SMEM_LOCK 1
#define SMEM_NODUMP 2
#define SMEM_NOFORK 4
#define SMEM_READONLY 8
#define SMEM_WRITEONLY 16
#define SMEM_NOACCESS  32
#define SMEM_SECURE (SMEM_LOCK | SMEM_NOFORK | SMEM_NODUMP)
#define SMEM_PARANOID (SMEM_SECURE | SMEM_NOACCESS)

void SecureClearMem(char *Mem, int Size);
void SecureDestroy(char *Mem, int Size);
int SecureRealloc(char **OldMem, int OldSize, int NewSize, int Flags);

void CredsStoreAdd(const char *Realm, const char *User, const char *Secret);
char *CredsStoreGetSecret(const char *Realm, const char *User);

#endif
