#include "tar.h"
#include <glob.h>
#include <pwd.h>
#include <grp.h>


#define TAR_RECORDSIZE 512

#define FILE_MODE_OFFSET 100
#define USER_OFFSET  108
#define GROUP_OFFSET 116
#define SIZE_OFFSET  124
#define MTIME_OFFSET 136
#define CHECKSUM_OFFSET 148
#define FTYPE_OFFSET 157

typedef struct 
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
	char pad[12];
} TTarHeader;


int TarReadHeader(STREAM *S, ListNode *Vars)
{
char *Tempstr=NULL, *ptr;
int len, result, RetVal=FALSE;
TTarHeader *Head;

len=sizeof(TTarHeader);
Head=(TTarHeader *) calloc(1,len);
result=STREAMReadBytes(S,Head,len);
printf("HEAD: %d %s\n",result,(char *) Head);
if (result == len)
{
Tempstr=CopyStr(Tempstr,Head->prefix);
Tempstr=CatStr(Tempstr,Head->name);
SetVar(Vars,"Path",Tempstr);

//Convert 'Size' from octal. Yes, octal.
Tempstr=FormatStr(Tempstr,"%d",strtol(Head->size,NULL,8));
SetVar(Vars,"Size",Tempstr);

//mode is in octal too
Tempstr=FormatStr(Tempstr,"%d",strtol(Head->mode,NULL,8));
SetVar(Vars,"Mode",Tempstr);

//mtime in, yes, you guessed it, octal 
Tempstr=FormatStr(Tempstr,"%d",strtol(Head->mtime,NULL,8));
SetVar(Vars,"Mtime",Tempstr);


SetVar(Vars,"Type","file"); 
StripTrailingWhitespace(Head->magic);
if (strcmp(Head->magic,"ustar")==0) 
{
switch (Head->typeflag)
{
	case '1': SetVar(Vars,"Type","hardlink"); break;
	case '2': SetVar(Vars,"Type","symlink"); break;
	case '3': SetVar(Vars,"Type","chardev"); break;
	case '4': SetVar(Vars,"Type","blkdev"); break;
	case '5': SetVar(Vars,"Type","directory"); break;
}

}
RetVal=TRUE;
}

DestroyString(Tempstr);
free(Head);

return(RetVal);
}


void TarUnpack(STREAM *Tar)
{
ListNode *Vars;
char *Path=NULL, *Tempstr=NULL, *ptr;
int bytes_read, bytes_total, val, result;
STREAM *S;

Vars=ListCreate();
while (TarReadHeader(Tar, Vars))
{
	Path=CopyStr(Path,GetVar(Vars,"Path"));
	if (StrLen(Path))
	{
		ptr=GetVar(Vars,"Type");
		if (ptr)
		{
		if (strcmp(ptr,"directory")==0)
		{
			mkdir(Path,atoi(GetVar(Vars,"Mode")));
		}
		else if (strcmp(ptr,"file")==0)
		{
			MakeDirPath(Path,0700);
			S=STREAMOpenFile(Path,O_WRONLY|O_CREAT|O_TRUNC);
			if (S) fchmod(S->out_fd,atoi(GetVar(Vars,"Mode")));
			bytes_read=0;
			bytes_total=atoi(GetVar(Vars,"Size"));
			Tempstr=SetStrLen(Tempstr,BUFSIZ);
			while (bytes_read < bytes_total)
			{
        val=bytes_total - bytes_read;
        if (val > BUFSIZ) val=BUFSIZ;
        if ((val % 512)==0) result=val;
        else result=((val / 512) + 1) * 512;
        result=STREAMReadBytes(Tar,Tempstr,result);
        if (result > val) result=val;
        if (S) STREAMWriteBytes(S,Tempstr,result);
        bytes_read+=result;
			}
			STREAMClose(S);	
		}
		}
	}
	ListClear(Vars,DestroyString);
}

ListDestroy(Vars,DestroyString);
DestroyString(Tempstr);
DestroyString(Path);
}



void TarWriteHeader(STREAM *S, char *Path, struct stat *FStat)
{
char *Tempstr=NULL, *ptr;
int i, chksum=0;
TTarHeader *Head;
struct passwd *pwd;
struct group *grp;

Head=(TTarHeader *) calloc(1,sizeof(TTarHeader));

	ptr=Path;
	if (*ptr=='/') ptr++;
	memcpy(Head->name,ptr,StrLen(ptr));

	sprintf(Head->mode,"%07o",FStat->st_mode);
	sprintf(Head->uid,"%07o",FStat->st_uid);
	sprintf(Head->gid,"%07o",FStat->st_gid);
	sprintf(Head->size,"%011o",FStat->st_size);
	sprintf(Head->mtime,"%011o",FStat->st_mtime);

	if (S_ISDIR(FStat->st_mode)) Head->typeflag='5';
	else if (S_ISLNK(FStat->st_mode)) Head->typeflag='2';
	else if (S_ISCHR(FStat->st_mode)) Head->typeflag='3';
	else if (S_ISBLK(FStat->st_mode)) Head->typeflag='4';
	else if (S_ISFIFO(FStat->st_mode)) Head->typeflag='6';
	else Head->typeflag='0';

	memset(Head->chksum,' ',8);

	strcpy(Head->magic,"ustar  ");
	//memcpy(Head->version,"00",2);

	pwd=getpwuid(FStat->st_uid);
	if (pwd) strcpy(Head->uname,pwd->pw_name);

	grp=getgrgid(FStat->st_gid);
	if (grp) strcpy(Head->gname,grp->gr_name);

	if ( (Head->typeflag == '3') || (Head->typeflag == '4') ) 
	{
		sprintf(Head->devmajor,"%07o",major(FStat->st_rdev));
		sprintf(Head->devminor,"%07o",minor(FStat->st_rdev));
	}

	ptr=(char *) Head;
	for (i=0; i < 512; i++) chksum+=*(ptr+i);
	sprintf(Head->chksum,"%06o\0",chksum);


STREAMWriteBytes(S,(char *) Head,512);

DestroyString(Tempstr);
free(Head);
}



void TarWriteFooter(STREAM *Tar)
{
char *Tempstr=NULL;

Tempstr=SetStrLen(Tempstr,TAR_RECORDSIZE);
memset(Tempstr,0,TAR_RECORDSIZE);
STREAMWriteBytes(Tar,Tempstr,TAR_RECORDSIZE);
STREAMWriteBytes(Tar,Tempstr,TAR_RECORDSIZE);

DestroyString(Tempstr);
}




void TarAddFile(STREAM *Tar, STREAM *File)
{
char *Buffer=NULL;
int result;

Buffer=SetStrLen(Buffer,TAR_RECORDSIZE);

memset(Buffer,0,TAR_RECORDSIZE);
result=STREAMReadBytes(File,Buffer,TAR_RECORDSIZE);
while (result > 0)
{
	STREAMWriteBytes(Tar,Buffer,TAR_RECORDSIZE);
	memset(Buffer,0,TAR_RECORDSIZE);
	result=STREAMReadBytes(File,Buffer,TAR_RECORDSIZE);
}

DestroyString(Buffer);
}



void TarInternalProcessFiles(STREAM *Tar, char *FilePattern)
{
glob_t Glob;
char *Tempstr=NULL, *ptr;
struct stat FStat;
int i;
STREAM *S;

ptr=GetToken(FilePattern,"\\S",&Tempstr,GETTOKEN_QUOTES);
if (ptr) glob(Tempstr,0,NULL,&Glob);
while (ptr)
{
	ptr=GetToken(ptr,"\\S",&Tempstr,GETTOKEN_QUOTES);
	if (ptr) glob(Tempstr,GLOB_APPEND,NULL,&Glob);
}

for (i=0; i < Glob.gl_pathc; i++)
{
	stat(Glob.gl_pathv[i],&FStat);
	if (S_ISDIR(FStat.st_mode))
	{
		Tempstr=MCopyStr(Tempstr,Glob.gl_pathv[i],"/*",NULL);
		TarInternalProcessFiles(Tar, Tempstr);
	}
	else 
	{
		S=STREAMOpenFile(Glob.gl_pathv[i],O_RDONLY);
		if (S)
		{
			TarWriteHeader(Tar, Glob.gl_pathv[i],&FStat);
			TarAddFile(Tar, S);
			STREAMClose(S);
		}
	}
}


globfree(&Glob);
DestroyString(Tempstr);
}


void TarFiles(STREAM *Tar, char *FilePattern)
{
	TarInternalProcessFiles(Tar, FilePattern);
	TarWriteFooter(Tar);
}
