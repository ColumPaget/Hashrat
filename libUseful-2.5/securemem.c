#include "securemem.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "string.h"

char *CredsStore=NULL;
int CredsStoreSize=0;
int CredsStoreUsed=0;

void SecureClearMem(char *Mem, int Size)
{
char *ptr;

	if (! Mem) return;
	xmemset((volatile char *) Mem,0,Size);
	for (ptr=Mem; ptr < (Mem + Size); ptr++)
	{
		if (*ptr != 0) 
		{
			fprintf(stderr,"LIBUSEFUL ERROR: Failed to clear secure memory");
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


int SecureRealloc(char **OldMem, int OldSize, int NewSize, int Flags)
{
int MemSize;
char *NewMem=NULL;
int val=0, PageSize;

PageSize=getpagesize();
MemSize=(NewSize / PageSize + 1) * PageSize;
if (OldMem && (NewSize < OldSize)) return(OldSize);

if (posix_memalign((void **) &NewMem, PageSize, MemSize)==0)
{
	if (OldMem)
	{
		//both needed 
		mprotect(*OldMem, OldSize, PROT_READ|PROT_WRITE);
		memcpy(NewMem,*OldMem,OldSize);
		SecureDestroy(*OldMem, OldSize);
	}


	#ifdef HAVE_MADVISE

		#ifdef MADV_NOFORK
			if (Flags & SMEM_NOFORK) madvise(NewMem,NewSize,MADV_DONTFORK);
		#endif
		
		#ifdef MADV_DONTDUMP
			if (Flags & SMEM_NODUMP) madvise(NewMem,NewSize,MADV_DONTDUMP);
		#endif

	#endif

	#ifdef HAVE_MLOCK
		 if (Flags & SMEM_LOCK) mlock(NewMem, NewSize);
	#endif

	if (Flags & SMEM_NOACCESS) mprotect(NewMem, NewSize, PROT_NONE);
	else if (Flags & SMEM_WRITEONLY) mprotect(NewMem, NewSize, PROT_WRITE);
	else if (Flags & SMEM_READONLY) mprotect(NewMem, NewSize, PROT_READ);
}
else
{
		fprintf(stderr,"LIBUSEFUL ERROR: Failed to allocate secure memory");
		exit(3);				
}

*OldMem=NewMem;
return(NewSize);
}


char *CredsStoreWrite(char *Dest, const char *String, int len)
{
uint16_t *plen;
char *ptr;

ptr=Dest;
plen=(uint16_t *) ptr;
*plen=len;
ptr+=sizeof(uint16_t);
memcpy(ptr, String, len);
ptr+=len;
*ptr='\0';
ptr++;
return(ptr);
}


char *CredsStoreGetSecret(const char *Realm, const char *User)
{
uint16_t plen, rlen, ulen;
char *ptr, *end, *frealm, *fuser;

rlen=StrLen(Realm);
ulen=StrLen(User);
ptr=CredsStore;
end=CredsStore + CredsStoreUsed;

if (rlen && ulen)
{
mprotect(CredsStore, CredsStoreSize, PROT_READ);
while (ptr < end)
{
	frealm=NULL;
	fuser=NULL;

	plen=*(uint16_t *) ptr;
	ptr+=sizeof(uint16_t);
	if (plen == rlen) frealm=ptr;
	ptr+=plen+1;

	plen=*(uint16_t *) ptr;
	ptr+=sizeof(uint16_t);
	if (plen == ulen) fuser=ptr;
	ptr+=plen+1;

	plen=*(uint16_t *) ptr;
	ptr+=sizeof(uint16_t);
	if (
			(frealm && fuser) &&
			(strcmp(frealm,Realm)==0) &&
			(strcmp(fuser,User)==0)
		) 
		{
			printf("FOUND %s\n",ptr);
			return(ptr);
		}
		ptr+=plen+1;
}
}
mprotect(CredsStore, CredsStoreSize, PROT_NONE);

return(NULL);
}


void CredsStoreAdd(const char *Realm, const char *User, const char *Secret)
{
int len;
char *ptr;

//Don't add if already exists
ptr=CredsStoreGetSecret(Realm, User);
if (ptr && (strcmp(Secret, ptr)==0))
{
mprotect(CredsStore, CredsStoreSize, PROT_NONE);
return;
}

len=CredsStoreUsed+StrLen(Realm) + StrLen(User) + StrLen(Secret) + 100;
CredsStoreSize=SecureRealloc(&CredsStore, CredsStoreSize, len, SMEM_NOFORK | SMEM_NODUMP | SMEM_LOCK);

mprotect(CredsStore, CredsStoreSize, PROT_WRITE);
ptr=CredsStoreWrite(CredsStore+CredsStoreUsed, Realm, StrLen(Realm));
ptr=CredsStoreWrite(ptr, User, StrLen(User));
ptr=CredsStoreWrite(ptr, Secret, StrLen(Secret));
mprotect(CredsStore, CredsStoreSize, PROT_NONE);

CredsStoreUsed=ptr-CredsStore;
}



