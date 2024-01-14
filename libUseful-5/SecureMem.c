#include "SecureMem.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "String.h"
#include "Errors.h"
#include "GeneralFunctions.h"

//needed for low-level file access
#include <sys/file.h>

static SECURESTORE *CredsStore=NULL;


int SecureLockMem(unsigned char *Mem, int Size, int Flags)
{
    int SetFlags=0;

//even if we don't have madvise, or we have it but it doesn't suport MADV_DONTFORK we stil set
//this flag so we can expunge memory 'by hand' if a fork is called within libUseful
    if (Flags & SMEM_NOFORK) SetFlags |= SMEM_NOFORK;

#ifdef HAVE_MADVISE
//if we have memadvise use it to prevent our memory getting copied in various
//situations

//WARNING: MADV_ arguments are not flags, you cannot or them together! They must
//be individually set, and settings can be unset with MADV_REMOVE


//MADV_DONTFORK IS DISABLED. It causes crashes on fork, no matter what one tries todo. For now some similar
//functionality is acheieved by calling 'CredsStoreOnFork' when a fork occurs in libUseful
    /*
    #ifdef MADV_DONTFORK
        //MADV_DONTFORK prevents this memory from getting copied to a child process
        if (Flags & SMEM_NOFORK)
    		{
    				madvise(Mem, Size, MADV_DONTFORK);
    				SetFlags |= SMEM_MADV_DONTFORK;
    		}
    #endif
    */

    //MADV_DONTDUMP prevents this memory from getting copied to a coredump
#ifdef MADV_DONTDUMP
    if (Flags & SMEM_NODUMP)
    {
        madvise(Mem, Size, MADV_DONTDUMP);
        SetFlags |= SMEM_NODUMP;
    }
#endif
#endif

#ifdef HAVE_MLOCK
    if (Flags & SMEM_LOCK)
    {
        mlock(Mem, Size);
        SetFlags |= SMEM_LOCK;
    }
#endif

    if (Flags & SMEM_NOACCESS)
    {
        mprotect(Mem, Size, PROT_NONE);
        SetFlags |= SMEM_NOACCESS;
    }
    else if (Flags & SMEM_WRONLY)
    {
        mprotect(Mem, Size, PROT_WRITE);
        SetFlags |= SMEM_WRONLY;
    }
    else if (Flags & SMEM_RDONLY)
    {
        mprotect(Mem, Size, PROT_READ);
        SetFlags |= SMEM_RDONLY;
    }

    return(SetFlags);
}


void SecureClearMem(unsigned char *Mem, int Size)
{
    unsigned char *ptr;

    if (! Mem) return;
    xmemset((volatile char *) Mem,0,Size);
    for (ptr=Mem; ptr < (Mem + Size); ptr++)
    {
        if (*ptr != 0)
        {
            RaiseError(0, "SecureClearMem", "Failed to clear secure memory!");
            exit(3);
        }
    }
}


void SecureDestroy(unsigned char *Mem, int Size)
{
    if (! Mem) return;
    SecureLockMem(Mem, Size, SMEM_WRONLY);
    SecureClearMem(Mem, Size);
    munlock(Mem, Size);
    free(Mem);
}




int SecureRealloc(unsigned char **OldMem, int OldSize, int NewSize, int Flags)
{
    unsigned char *NewMem=NULL;
    int val=0, PageSize;
    int MemSize;

    PageSize=getpagesize();
    MemSize=((NewSize / PageSize) + 1) * PageSize;
    if (OldMem && (MemSize < OldSize)) return(OldSize);

    if (posix_memalign((void **) &NewMem, PageSize, MemSize)==0)
    {
        if (OldMem && *OldMem)
        {
            //both needed
            mprotect(*OldMem, OldSize, PROT_READ|PROT_WRITE);
            memcpy(NewMem, *OldMem, OldSize);
            SecureDestroy(*OldMem, OldSize);
        }

        SecureLockMem(NewMem, MemSize, Flags);
    }
    else
    {
        RaiseError(0, "SecureRealloc", "Failed to allocate secure memory!");
        exit(3);
    }

    *OldMem=NewMem;
    return(MemSize);
}


char *SecureMMap(const char *Path)
{
    struct stat FStat;
    unsigned char *addr=NULL;
    int fd;

    fd=open(Path, O_RDONLY);
    if (fd)
    {
        stat(Path, &FStat);
        addr=mmap(NULL, FStat.st_size, PROT_NONE, MAP_PRIVATE, fd, 0);
        close(fd);
    }

    return(addr);
}



SECURESTORE *SecureStoreCreate(int Size)
{
    SECURESTORE *Store;

    Store=(SECURESTORE *) calloc(1,sizeof(SECURESTORE));
    if (Size > 0)
    {
        Store->Size=SecureRealloc(&(Store->Data), 0, Size, 0);
        Store->Flags=SecureLockMem(Store->Data, Store->Size, SMEM_SECURE);
    }

    return(Store);
}


void SecureStoreDestroy(SECURESTORE *Store)
{
    if (Store)
    {
        //if SMEM_MADV_DONTFORK is set, then do not touch Store->Data as it will not be copied to the
        //forked processes memory space
        if (Store->Data && (! (Store->Flags & SMEM_MADV_DONTFORK)) ) SecureDestroy(Store->Data, Store->Size);
        else if (Store->Data) free(Store->Data);
        free(Store);
    }
}


void *SecureStoreAdd(SECURESTORE *Store, void *Data, uint32_t Size)
{
    int NewSize;
    void *ptr;

    NewSize=Store->Used + Size + sizeof(uint32_t);
    Store->Size=SecureRealloc(&(Store->Data), Store->Size, NewSize, Store->Flags);
    ptr=Store->Data + Store->Used;
    SecureLockMem(CredsStore->Data, CredsStore->Size, SMEM_WRONLY);
    memcpy(ptr, &Size, sizeof(uint32_t));
    ptr+=sizeof(uint32_t);
    if (Data) memcpy(ptr, Data, Size);
    SecureLockMem(CredsStore->Data, CredsStore->Size, SMEM_NOACCESS);
    Store->Used=NewSize;

    return(ptr);
}


int SecureStoreGetNext(SECURESTORE *Store, const unsigned char **ptr)
{
    uint32_t len=0;

    if (*ptr==NULL) *ptr=Store->Data;

    if ( (*ptr >= Store->Data) && (*ptr < (Store->Data + Store->Used)) )
    {
        len=*(uint32_t *) *ptr;

        if ((Store->Data + Store->Used) > (*ptr + len))
        {
            *ptr += sizeof(uint32_t);
            return(len);
        }
    }

    *ptr=NULL;
    return(0);
}




SECURESTORE *SecureStoreLoad(const char *Path)
{
    SECURESTORE *Store=NULL;
    struct stat FStat;
    char *addr;
    int fd, result, Flags=0;

    fd=open(Path, O_RDONLY);
    if (fd > -1)
    {
        stat(Path, &FStat);
        Store=SecureStoreCreate(FStat.st_size);
        Store->Divisor=':';
        result=read(fd, &Store->Data, FStat.st_size);
        SecureLockMem(Store->Data, Store->Size, SMEM_PARANOID);
        close(fd);
    }


    return(Store);
}



int SecureStoreNextLine(SECURESTORE *SS, unsigned char **Line)
{
    unsigned char *line_start, *line_end, *block_end;

    block_end=SS->Data + SS->Size;
    if (SS->CurrLine >= block_end) return(EOF);
    if (! SS->CurrLine) SS->CurrLine=SS->Data;

    *Line=SS->CurrLine;
//search for the '\n' that ends lines
    line_end=memchr(SS->CurrLine, '\n', block_end - SS->CurrLine);

//maybe data doesn't end with a final '\n'
    if (! line_end) line_end=block_end;

    SS->CurrLine=line_end+1;

//return length of line, which is end-start.
    return(line_end - (*Line));
}


int SecureStoreGetLine(SECURESTORE *SS, int LineNo, unsigned char **Line)
{
    int len, i;

    SS->CurrLine=SS->Data;
    for (i=0; i <=LineNo; i++)
    {
        len=SecureStoreNextLine(SS, Line);
        //not that many lines in data!!
        if (len==EOF) return(EOF);
    }

    return(len);
}


char *SecureStoreWriteField(char *Dest, const char *Field, char Divisor)
{
    int len;
    char *ptr;

    ptr=Dest;
    len=StrLen(Field);
    memcpy(ptr,Field,len);
    ptr+=len;
    *ptr=Divisor;
    ptr++;

    return(ptr);
}



int CredsStoreLoad(const char *Path)
{
    if (CredsStore) SecureStoreDestroy(CredsStore);

    CredsStore=SecureStoreLoad(Path);
    if (CredsStore) return(TRUE);
    return(FALSE);
}


char *CredsStoreWriteField(char *Dest, const char *Field)
{
    char *ptr;
    int len;

    len=StrLen(Field);
    ptr=Dest;
    memcpy(Dest, Field, len);
    ptr+=len;
    *ptr=':';
    ptr++;

    return (ptr);
}


SECURESTORE *CredsStoreCreate()
{
    CredsStore=SecureStoreCreate(0);
    LibUsefulSetupAtExit();

    return(CredsStore);
}


int CredsStoreAdd(const char *Realm, const char *User, const char *Cred)
{
    int len;
    char *ptr;
    char *QRealm=NULL, *QUser=NULL, *QCred=NULL;

    if (! CredsStore) CredsStoreCreate();

    QRealm=QuoteCharsInStr(QRealm, Realm, ":");
    QUser=QuoteCharsInStr(QUser, User, ":");
    QCred=QuoteCharsInStr(QCred, Cred, ":");

    len=StrLen(QRealm) + StrLen(QUser) + StrLen(QCred) +3;
    ptr=SecureStoreAdd(CredsStore, NULL, len);

    CredsStore->Flags=SecureLockMem(CredsStore->Data, CredsStore->Size, SMEM_WRONLY|SMEM_SECURE);
    if (CredsStore->Flags & SMEM_NOFORK) CredsStore->OwnerPid=getpid();
    ptr=CredsStoreWriteField(ptr, QRealm);
    ptr=CredsStoreWriteField(ptr, QUser);
    ptr=CredsStoreWriteField(ptr, QCred);
    SecureLockMem(CredsStore->Data, CredsStore->Size, SMEM_NOACCESS);

    Destroy(QRealm);
    Destroy(QUser);
    Destroy(QCred);
    return(TRUE);
}


int SecureStoreFieldMatch(SECURESTORE *SS, const char **Input, const char *Match)
{
    const char *sptr, *eptr;
    int mlen, slen, result;
    char *QMatch=NULL;

    QMatch=QuoteCharsInStr(QMatch, Match, ":");
    mlen=StrLen(QMatch);
    if (mlen == 0) return(FALSE);

    sptr=*Input;
    eptr=traverse_until(sptr, ':');
    slen=eptr - sptr;
    //if (slen != mlen) return(FALSE);

    result=strncmp(sptr, QMatch, mlen);
    Destroy(QMatch);

    if (*eptr==':') eptr++;
    *Input=eptr;

    if (result==0)	return(TRUE);
    return(FALSE);
}

int CredsStoreLookup(const char *Realm, const char *User, const char **Pass)
{
    const char *p_Line, *p_Data, *end;
    int len, result=0;

    *Pass=NULL;
    if (! CredsStore) return(0);
    SecureLockMem(CredsStore->Data, CredsStore->Size, SMEM_RDONLY);
    p_Line=NULL;
    len=SecureStoreGetNext(CredsStore, (const unsigned char **) &p_Line);
    while (len > 0)
    {
        p_Data=p_Line;
        if (
            SecureStoreFieldMatch(CredsStore, &p_Data, Realm) &&
            SecureStoreFieldMatch(CredsStore, &p_Data, User)
        )
        {
            *Pass=p_Data;
            end=traverse_until(p_Data, ':');
            result=end - p_Data;
            break;
        }
        p_Line+=len;
        len=SecureStoreGetNext(CredsStore,  (const unsigned char **) &p_Line);
    }

    return(result);
}


void CredsStoreDestroy()
{
    if (CredsStore) SecureStoreDestroy(CredsStore);
    CredsStore=NULL;
}

void CredsStoreOnFork()
{
    if (CredsStore && (CredsStore->Flags & SMEM_NOFORK))
    {
        SecureStoreDestroy(CredsStore);
    }
}
