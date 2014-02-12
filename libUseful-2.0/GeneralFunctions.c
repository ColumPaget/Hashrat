#include "includes.h"
#include "base64.h"

int WritePidFile(char *ProgName) 
{ 
char *Tempstr=NULL; 
STREAM *S;
int result=FALSE;
 

if (*ProgName=='/') Tempstr=CopyStr(Tempstr,ProgName);
else Tempstr=FormatStr(Tempstr,"/var/run/%s.pid",ProgName);

S=STREAMOpenFile(Tempstr,O_CREAT | O_WRONLY); 
if (S)
{
	fchmod(S->in_fd,0644); 
	if (flock(S->in_fd,LOCK_EX|LOCK_NB) !=0) 
	{ 
		STREAMClose(S); 
		exit(1); 
	}
	result=TRUE; 
	Tempstr=FormatStr(Tempstr,"%d\n",getpid()); 
	STREAMWriteLine(Tempstr,S); 
	STREAMFlush(S); 
} 

//Don't close 'S'!

DestroyString(Tempstr);

return(result);
} 


void CloseOpenFiles()
{
      int i;

      for (i=3; i < 1024; i++) close(i);
}


char *BytesToHexStr(char *Buffer, char *Bytes, int len)
{
int i;
char *Str=NULL, *ptr;


Str=SetStrLen(Buffer,(len *2) +1);
ptr=Str;
for (i=0; i < len; i++)
{
	snprintf(ptr,2,"%02x",Bytes[i]);
	ptr+=2;
}
*ptr='\0';

return(Str);
}


int HexStrToBytes(char **Buffer, char *HexStr)
{
int i, len;
char *Str=NULL, *ptr;

len=StrLen(HexStr);
*Buffer=SetStrLen(*Buffer,len / 2);
ptr=*Buffer;
for (i=0; i < len; i+=2)
{
   Str=CopyStrLen(Str,HexStr+i,2);
   *ptr=strtol(Str,NULL,16);
   ptr++;
}

DestroyString(Str);
return(len / 2);
}


void SwitchProgram(char *CommandLine)
{
char **argv, *ptr;
char *Token=NULL;
int i;

argv=(char **) calloc(101,sizeof(char *));
ptr=CommandLine;
for (i=0; i < 100; i++)
{
ptr=GetToken(ptr,"\\S",&Token,GETTOKEN_QUOTES);
if (! ptr) break;
argv[i]=CopyStr(argv[i],Token);
}

/* we are the child so we continue */
execv(argv[0],argv);
//no point trying to free stuff here, we will no longer
//be the main program
}


#include <pwd.h>
#include <grp.h>

int SwitchUser(char *NewUser)
{
struct passwd *pwent;

    pwent=getpwnam(NewUser);
    if (! pwent) return(FALSE);
    if (setreuid(pwent->pw_uid,pwent->pw_uid) !=0) return(FALSE);
    return(TRUE);
}


int SwitchGroup(char *NewGroup)
{
struct group *grent;
 
     grent=getgrnam(NewGroup);
     if (! grent) return(FALSE);
     if (setgid(grent->gr_gid) !=0) return(FALSE);
     return(TRUE);
}

char *GetCurrUserHomeDir()
{
struct passwd *pwent;

    pwent=getpwuid(getuid());
    if (! pwent) return(NULL);
    return(pwent->pw_dir);
}



void ColLibDefaultSignalHandler(int sig)
{

}


int CreateLockFile(char *FilePath, int Timeout)
{
int fd, result;

SetTimeout(Timeout);
fd=open(FilePath, O_CREAT | O_RDWR, 0600);
if (fd <0) return(-1);
result=flock(fd,LOCK_EX);
alarm(0);

if (result==-1)
{
  close(fd);
  return(-1);
}
return(fd);
}




char *GetNameValuePair(const char *Input, const char *PairDelim, const char *NameValueDelim, char **Name, char **Value)
{
char *ptr, *ptr2;
char *Token=NULL;

ptr=GetToken(Input,PairDelim,&Token,GETTOKEN_QUOTES);
if (StrLen(Token) && strstr(Token,NameValueDelim))
{
ptr2=GetToken(Token,NameValueDelim,Name,GETTOKEN_QUOTES);
ptr2=GetToken(ptr2,PairDelim,Value,GETTOKEN_QUOTES);
}

DestroyString(Token);
return(ptr);
}





char *GetRandomData(char *RetBuff, int len, char *AllowedChars)
{
int fd;
char *Tempstr=NULL, *RetStr=NULL;
int i;
uint8_t val, max_val;

srand(time(NULL));
max_val=StrLen(AllowedChars);

RetStr=CopyStr(RetBuff,"");
fd=open("/dev/urandom",O_RDONLY);
for (i=0; i < len ; i++)
{
	if (fd > -1) read(fd,&val,1);
	else val=rand();

	RetStr=AddCharToStr(RetStr,AllowedChars[val % max_val]);
}

if (fd) close(fd);

DestroyString(Tempstr);
return(RetStr);
}


char *GetRandomHexStr(char *RetBuff, int len)
{
return(GetRandomData(RetBuff,len,HEX_CHARS));
}


char *GetRandomAlphabetStr(char *RetBuff, int len)
{
return(GetRandomData(RetBuff,len,ALPHA_CHARS));
}





#define KILOBYTE 1000
#define MEGABYTE 1000000
#define GIGABYTE 1000000000
#define TERABYTE 1000000000000

#define KIBIBYTE 1024
#define MEGIBYTE 1024 * 1024
#define GIGIBYTE 1024 * 1024 * 1024
#define TERIBYTE 1024 * 1024 * 1024 *1024

double ParseHumanReadableDataQty(char *Data, int Type)
{
double val;
char *ptr=NULL;
double KAY,MEG,GIG,TERA;

if (Type)
{
KAY=KILOBYTE;
MEG=MEGABYTE;
GIG=GIGABYTE;
//TERA=TERABYTE;
}
else
{
KAY=KIBIBYTE;
MEG=MEGIBYTE;
GIG=GIGIBYTE;
//TERA=TERIBYTE;
}

	val=strtod(Data,&ptr);
	while (isspace(*ptr)) ptr++;
	if (*ptr=='k') val=val * KAY;
	if (*ptr=='M') val=val * MEG;
	if (*ptr=='G') val=val * GIG;
//	if (*ptr=='T') val=val * TERA;


return(val);
}



char *GetHumanReadableDataQty(double Size, int Type)
{
static char *Str=NULL;
double val=0;
char kMGT=' ';
//Set to 0 to keep valgrind happy
double KAY=0,MEG=0,GIG=0,TERA=0;

if (Type)
{
KAY=KILOBYTE;
MEG=MEGABYTE;
GIG=GIGABYTE;
//TERA=TERABYTE;
}
else
{
KAY=KIBIBYTE;
MEG=MEGIBYTE;
GIG=GIGIBYTE;
//TERA=TERIBYTE;
}
    val=Size;
    kMGT=' ';
/*    if (val > (TERA))
    {
      val=val / TERA;
      kMGT='T';
    }
    else*/
	 if (val > (GIG))
    {
      val=val / GIG;
      kMGT='G';
    }
    else if (val > (MEG))
    {
      val=val / MEG;
      kMGT='M';

    }
    else if (val > (KAY))
    {
      val=val /  KAY;
      kMGT='k';
    }

Str=FormatStr(Str,"%0.1f%c",(float) val,kMGT);
return(Str);
}


void EraseString(char *Buff, char *Target)
{
char *ptr;
int len;

len=StrLen(Target);
ptr=strstr(Buff,Target);
while (ptr)
{
memset(ptr,' ',len);
ptr=strstr(ptr,Target);
}

}
