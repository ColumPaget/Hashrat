#include "FileSystem.h"
#include <glob.h>

char *GetBasename(char *Path)
{
char *ptr;
int len;

len=StrLen(Path);
if (len==0) return("");
if (len==1) return(Path);

ptr=Path+len-1;
while (ptr > Path)
{
  if ((*ptr=='/') && (*(ptr+1) != '\0')) break;
  ptr--;
}

if ((*ptr=='/') && (*(ptr+1) != '\0')) ptr++;

return(ptr);
}


char *SlashTerminateDirectoryPath(char *DirPath)
{
char *ptr, *RetStr=NULL;

if (! DirPath) return(CopyStr(DirPath,"/"));
RetStr=DirPath;
ptr=RetStr+StrLen(RetStr)-1;
if (*ptr != '/') RetStr=AddCharToStr(RetStr,'/');

return(RetStr);
}


char *StripDirectorySlash(char *DirPath)
{
char *ptr;

//don't strip '/' (root dir)
if (StrLen(DirPath)==1) return(DirPath);
ptr=DirPath+StrLen(DirPath)-1;

if (*ptr == '/') *ptr='\0';

return(DirPath);
}


int MakeDirPath(char *Path, int DirMask)
{
 char *ptr;
 char *Tempstr=NULL;
 int result;

 ptr=Path;
 if (*ptr=='/') ptr++;
 ptr=strchr(ptr, '/');
 while (ptr)
 {
   Tempstr=CopyStrLen(Tempstr,Path,ptr-Path);
   result=mkdir(Tempstr, DirMask);
   if ((result==-1) && (errno != EEXIST)) break;
   ptr=strchr(++ptr, '/');
 }
 DestroyString(Tempstr);
 if (result==0) return(TRUE);
 return(FALSE);
}



int ChangeFileExtension(char *FilePath, char *NewExt)
{
char *ptr;
char *Tempstr=NULL;
int result;

Tempstr=CopyStr(Tempstr,FilePath);
ptr=strrchr(Tempstr,'/');
if (!ptr) ptr=Tempstr;
ptr=strrchr(ptr,'.');
if (! ptr) ptr=Tempstr+StrLen(Tempstr);
*ptr='\0';

if (*NewExt=='.') Tempstr=CatStr(Tempstr,NewExt);
else Tempstr=MCatStr(Tempstr,".",NewExt,NULL);
result=rename(FilePath,Tempstr);

DestroyString(Tempstr);
if (result==0) return(TRUE);
else return(FALSE);
}


int FindFilesInPath(char *File, char *Path, ListNode *Files)
{
char *Tempstr=NULL, *CurrPath=NULL, *RetStr=NULL, *ptr;
int i;
glob_t Glob;

if (*File=='/')
{
	CurrPath=CopyStr(CurrPath,"");
	ptr=""; //so we execute once below
}
else ptr=GetToken(Path,":",&CurrPath,0);
while (ptr)
{
CurrPath=SlashTerminateDirectoryPath(CurrPath);
Tempstr=MCopyStr(Tempstr,CurrPath,File,NULL);

glob(Tempstr,0,0,&Glob);
for (i=0; i < Glob.gl_pathc; i++) ListAddItem(Files,CopyStr(NULL,Glob.gl_pathv[i]));
globfree(&Glob);

ptr=GetToken(ptr,":",&CurrPath,0);
}

DestroyString(Tempstr);
DestroyString(CurrPath);

return(ListSize(Files));
}



char *FindFileInPath(char *InBuff, char *File, char *Path)
{
char *Tempstr=NULL, *CurrPath=NULL, *RetStr=NULL, *ptr;

RetStr=CopyStr(InBuff,"");

if (*File=='/')
{
	CurrPath=CopyStr(CurrPath,"");
	ptr=""; //so we execute once below
}
else ptr=GetToken(Path,":",&CurrPath,0);

while (ptr)
{
CurrPath=SlashTerminateDirectoryPath(CurrPath);
Tempstr=MCopyStr(Tempstr,CurrPath,File,NULL);
if (access(Tempstr,F_OK)==0) 
{
RetStr=CopyStr(RetStr,Tempstr);
break;
}

ptr=GetToken(ptr,":",&CurrPath,0);
}

DestroyString(Tempstr);
DestroyString(CurrPath);

return(RetStr);
}


/* This checks if a certain file exists (not if we can open it etc, just if */
/* we can stat it, this is useful for checking pid files etc).              */
int FileExists(char *FileName)
{
struct stat StatData;

if (stat(FileName,&StatData) == 0) return(1);
else return(0);
}

