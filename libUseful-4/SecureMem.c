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

void SecureClearMem(char *Mem, int Size)
{
    char *ptr;

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


void SecureDestroy(char *Mem, int Size)
{
    if (! Mem) return;
    SecureClearMem(Mem, Size);
    munlock(Mem, Size);
    free(Mem);
}


int SecureLockMem(char *Mem, int Size, int Flags)
{
#ifdef HAVE_MADVISE
//if we have memadvise use it to prevent our memory getting copied in various
//situations

//WARNING: MADV_ arguments are not flags, you cannot or them together! They must
//be individually set, and settings can be unset with MADV_REMOVE

    //MADV_DONTFORK prevents this memory from getting copied to a child process
#ifdef MADV_DONTFORK
    if (Flags & SMEM_NOFORK) madvise(Mem,Size,MADV_DONTFORK);
#endif

    //MADV_DONTDUMP prevents this memory from getting copied to a coredump
#ifdef MADV_DONTDUMP
    if (Flags & SMEM_NOFORK) madvise(Mem,Size,MADV_DONTDUMP);
#endif
#endif

#ifdef HAVE_MLOCK
    if (Flags & SMEM_LOCK) mlock(Mem, Size);
#endif

    if (Flags & SMEM_NOACCESS) mprotect(Mem, Size, PROT_NONE);
    else if (Flags & SMEM_WRONLY) mprotect(Mem, Size, PROT_WRITE);
    else if (Flags & SMEM_RDONLY) mprotect(Mem, Size, PROT_READ);

    return(TRUE);
}



int SecureRealloc(char **OldMem, int OldSize, int NewSize, int Flags)
{
    char *NewMem=NULL;
    int val=0, PageSize;
    int MemSize;

    PageSize=getpagesize();
    MemSize=(NewSize / PageSize + 1) * PageSize;
    if (OldMem && (NewSize < OldSize)) return(OldSize);

    if (posix_memalign((void **) &NewMem, PageSize, MemSize)==0)
    {
        if (OldMem)
        {
            //both needed
            mprotect(*OldMem, OldSize, PROT_READ|PROT_WRITE);
            memcpy(NewMem,*OldMem, OldSize);
            SecureDestroy(*OldMem, OldSize);
        }

        SecureLockMem(NewMem, NewSize, SMEM_PARANOID);
    }
    else
    {
        RaiseError(0, "SecureRealloc", "Failed to allocate secure memory!");
        exit(3);
    }

    *OldMem=NewMem;
    return(NewSize);
}


char *SecureMMap(const char *Path)
{
    struct stat FStat;
    char *addr=NULL;
    int fd;

    fd=open(Path, O_RDONLY);
    if (fd)
    {
        stat(Path, &FStat);
        addr=mmap(NULL, FStat.st_size, PROT_NONE, MAP_PRIVATE , fd, 0);
        close(fd);
    }

    return(addr);
}



SECURESTORE *SecureStoreCreate(int Size)
{
    SECURESTORE *Store;

    Store=(SECURESTORE *) calloc(1,sizeof(SECURESTORE));
    Store->Size=SecureRealloc(&(Store->Data), 0, Size, SMEM_NOFORK | SMEM_NODUMP | SMEM_LOCK);

    return(Store);
}


void SecureStoreDestroy(SECURESTORE *Store)
{
//SecureDestroy(Store->Data, Store->Size);
    free(Store);
}


SECURESTORE *SecureStoreLoad(const char *Path)
{
    SECURESTORE *Store=NULL;
    struct stat FStat;
    char *addr;
    int fd, Flags=0;

    fd=open(Path, O_RDONLY);
    if (fd > -1)
    {
        stat(Path, &FStat);
        Flags=MAP_PRIVATE;

        //MAP_LOCKED works like mlock and prevents memory being swapped out
#ifdef MAP_LOCKED
        Flags |= MAP_LOCKED;
#endif

        addr=(char *) mmap(NULL, FStat.st_size, PROT_READ, Flags, fd, 0);
        if (addr)
        {
            Store=(SECURESTORE *) calloc(1,sizeof(SECURESTORE));
            Store->Size=FStat.st_size;
            Store->Data=addr;
            Store->Divisor=':';
            SecureLockMem(Store->Data, Store->Size, SMEM_PARANOID);
        }
        close(fd);
    }


    return(Store);
}


int SecureStoreGetField(const char *linestart, const char *lineend, int FieldNo, const char Divisor, const char **start)
{
    const char *next, *sptr=NULL, *eptr=NULL;
    int i;

    next=linestart;
    for (i=0; (next < lineend) && (i <= FieldNo); i++)
    {
        sptr=next;
        eptr=memchr(sptr, Divisor, lineend-sptr);
        if (eptr) next=eptr+1;
        else eptr=lineend;
    }

    if (i==(FieldNo+1))
    {
        *start=sptr;
        return(eptr-sptr);
    }

    return(-1);
}

int SecureStoreFieldMatch(SECURESTORE *SS, const char *line_start, const char *line_end, int Field, const char *Match)
{
    const char *sptr, *eptr;
    int mlen, slen;

    mlen=StrLen(Match);
    if (mlen == 0) return(FALSE);
    slen=SecureStoreGetField(line_start, line_end, Field, SS->Divisor, &sptr);
    if (slen != mlen) return(FALSE);
    if (strncmp(sptr,Match,slen)==0) return(TRUE);

    return(FALSE);
}


int SecureStoreNextLine(SECURESTORE *SS, char **Line)
{
    char *line_start, *line_end, *block_end;

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


int SecureStoreGetLine(SECURESTORE *SS, int LineNo, char **Line)
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

int CredsStoreAdd(const char *Realm, const char *User, const char *Cred)
{
    int len;
    char *ptr;

    if (! CredsStore) CredsStore=(SECURESTORE *) calloc(1,sizeof(SECURESTORE));

// ten extra bytes is overkill, as there should be max 3 divisors and an
// end of line character, but what they hey
    len=CredsStore->Size + StrLen(Realm) + StrLen(User) + StrLen(Cred) + 4;
    SecureRealloc(&(CredsStore->Data), CredsStore->Size, len, SMEM_NOFORK | SMEM_NODUMP | SMEM_LOCK);
    SecureLockMem(CredsStore->Data, len, SMEM_WRONLY);
    ptr=SecureStoreWriteField(CredsStore->Data+CredsStore->Size, Realm, CredsStore->Divisor);
    ptr=SecureStoreWriteField(ptr, User, CredsStore->Divisor);
    ptr=SecureStoreWriteField(ptr, Cred, CredsStore->Divisor);
    *ptr='\n';
    SecureLockMem(CredsStore->Data, len, SMEM_NOACCESS);
    CredsStore->Size=len;

    return(TRUE);
}


int CredsStoreLookup(const char *Realm, const char *User, const char **Pass)
{
    char *p_Line, *p_LineEnd;
    int len, result=0;

    *Pass=NULL;
    if (! CredsStore) return(0);
    CredsStore->CurrLine=NULL;
    SecureLockMem(CredsStore->Data, CredsStore->Size, SMEM_RDONLY);
    len=SecureStoreNextLine(CredsStore, &p_Line);
    while (len != EOF)
    {
        p_LineEnd=p_Line+len;
        if (
            SecureStoreFieldMatch(CredsStore, p_Line, p_LineEnd, 0, Realm) &&
            SecureStoreFieldMatch(CredsStore, p_Line, p_LineEnd, 1, User)
        )
        {
            result=SecureStoreGetField(p_Line, p_LineEnd, 2, CredsStore->Divisor, Pass);
        }
        len=SecureStoreNextLine(CredsStore, &p_Line);
    }

    return(result);
}



