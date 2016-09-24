#include "includes.h"
#include "base64.h"
#include "Hash.h"
#include "Time.h"
#include <sys/utsname.h>
#include <sys/file.h>
#include "base64.h"

//xmemset uses a 'volatile' pointer so that it won't be optimized out
void xmemset(char *Str, char fill, off_t size)
{
volatile char *p;

for (p=Str; p < (Str+size); p++) *p=fill;
}


int WritePidFile(char *ProgName)
{
char *Tempstr=NULL;
int fd;


if (*ProgName=='/') Tempstr=CopyStr(Tempstr,ProgName);
else Tempstr=FormatStr(Tempstr,"/var/run/%s.pid",ProgName);

fd=open(Tempstr,O_CREAT | O_TRUNC | O_WRONLY,0600);
if (fd > -1)
{
  fchmod(fd,0644);
  if (flock(fd,LOCK_EX|LOCK_NB) !=0)
  {
    close(fd);
    exit(1);
  }
  Tempstr=FormatStr(Tempstr,"%d\n",getpid());
  write(fd,Tempstr,StrLen(Tempstr));
}

//Don't close 'fd'!

DestroyString(Tempstr);

return(fd);
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


char *Ascii85(char *RetStr, const char *Bytes, int ilen, const char *CharMap)
{
const char *ptr, *block, *end;
uint32_t val, mod;
int olen=0, i;
char Buff[6];

end=Bytes+ilen;
for (ptr=Bytes; ptr < end; )
{
	block=ptr;
	val = ((*ptr & 0xFF) << 24); ptr++;
	if (ptr < end)
	{
	val |= ((*ptr & 0xFF) << 16); ptr++;
	}

	if (ptr < end)
	{
	val |= ((*ptr & 0xFF) << 8); ptr++;
	}

	if (ptr < end)
	{
	val |= (*ptr & 0xFF); ptr++;
	}

	if (val==0) strcpy(Buff,"z");
	else for (i=4; i >-1; i--)
	{
		mod=val % 85;
		val /= 85;
		Buff[i]=CharMap[mod & 0xFF];
	}

	//we only add as many characters as we encoded
	//so for the last chracter
	RetStr=CatStrLen(RetStr,Buff,ptr-block);
} 

return(RetStr);
}


char *EncodeBytes(char *Buffer, const char *Bytes, int len, int Encoding)
{
char *Tempstr=NULL, *RetStr=NULL;
int i;

RetStr=CopyStr(Buffer,"");
switch (Encoding)
{
	case ENCODE_BASE64: 
	RetStr=SetStrLen(RetStr,len * 4);
	to64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len); break;
	break;

	case ENCODE_IBASE64: 
	RetStr=SetStrLen(RetStr,len * 4);
	Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,IBASE64_CHARS,'\0'); break;
	break;

	case ENCODE_PBASE64: 
	RetStr=SetStrLen(RetStr,len * 4);
	Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,SBASE64_CHARS,'\0'); break;
	break;


	case ENCODE_CRYPT: 
	RetStr=SetStrLen(RetStr,len * 4);
	Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,CRYPT_CHARS,'\0'); break;
	break;

	case ENCODE_XXENC: 
	RetStr=SetStrLen(RetStr,len * 4);
	Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,XXENC_CHARS,'+'); break;
	break;

	case ENCODE_UUENC: 
	RetStr=SetStrLen(RetStr,len * 4);
	Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,UUENC_CHARS,'\''); break;
	break;

	case ENCODE_ASCII85: 
	RetStr=Ascii85(RetStr,Bytes,len,ASCII85_CHARS); break;
	break;

	case ENCODE_Z85: 
	RetStr=Ascii85(RetStr,Bytes,len,Z85_CHARS); break;
	break;

	case ENCODE_OCTAL:
  for (i=0; i < len; i++)
  {
  Tempstr=FormatStr(Tempstr,"%03o",Bytes[i] & 255);
  RetStr=CatStr(RetStr,Tempstr);
  }
	break;

	case ENCODE_DECIMAL:
  for (i=0; i < len; i++)
  {
  Tempstr=FormatStr(Tempstr,"%03d",Bytes[i] & 255);
  RetStr=CatStr(RetStr,Tempstr);
  }
	break;

	case ENCODE_HEX:
  for (i=0; i < len; i++)
  {
  Tempstr=FormatStr(Tempstr,"%02x",Bytes[i] & 255);
  RetStr=CatStr(RetStr,Tempstr);
  }
	break;

	case ENCODE_HEXUPPER:
  for (i=0; i < len; i++)
  {
  Tempstr=FormatStr(Tempstr,"%02X",Bytes[i] & 255);
  RetStr=CatStr(RetStr,Tempstr);
  }
	break;
	

	default:
	RetStr=SetStrLen(RetStr,len );
	memcpy(RetStr,Bytes,len);
	RetStr[len]='\0';
	break;
}

DestroyString(Tempstr);
return(RetStr);
}



#include <pwd.h>
#include <grp.h>

int SwitchUser(const char *NewUser)
{
struct passwd *pwent;
char *ptr;

    pwent=getpwnam(NewUser);
    if (! pwent)
		{
			syslog(LOG_ERR,"ERROR: Cannot switch to user '%s'. No such user",NewUser);
			ptr=LibUsefulGetValue("SwitchUserAllowFail");
			if (ptr && (strcasecmp(ptr,"yes")==0)) return(FALSE);
			exit(1);
		}


    if (setreuid(pwent->pw_uid,pwent->pw_uid) !=0) 
		{
			syslog(LOG_ERR,"ERROR: Switch to user '%s' failed. Error was: %s",NewUser,strerror(errno));
			ptr=LibUsefulGetValue("SwitchUserAllowFail");
			if (ptr && (strcasecmp(ptr,"yes")==0)) return(FALSE);
			exit(1);
		}

	return(TRUE);
}


int SwitchGroup(const char *NewGroup)
{
struct group *grent;
char *ptr;
 
		grent=getgrnam(NewGroup);
		if (! grent) 
		{
			syslog(LOG_ERR,"ERROR: Cannot switch to group '%s'. No such group",NewGroup);
			ptr=LibUsefulGetValue("SwitchGroupAllowFail");
			if (ptr && (strcasecmp(ptr,"yes")==0)) return(FALSE);
			exit(1);
		}


		if (setgid(grent->gr_gid) !=0) 
		{
			syslog(LOG_ERR,"ERROR: Switch to group '%s' failed. Error was: %s",NewGroup,strerror(errno));
			ptr=LibUsefulGetValue("SwitchGroupAllowFail");
			if (ptr && (strcasecmp(ptr,"yes")==0)) return(FALSE);
			exit(1);
		}

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



int GenerateRandomBytes(char **RetBuff, int ReqLen, int Encoding)
{
struct utsname uts;
int i, len;
clock_t ClocksStart, ClocksEnd;
char *Tempstr=NULL, *RandomBytes=NULL;
int fd;


fd=open("/dev/urandom",O_RDONLY);
if (fd > -1)
{
	RandomBytes=SetStrLen(RandomBytes,ReqLen);
  len=read(fd,RandomBytes,ReqLen);
  close(fd);
}
else
{
	ClocksStart=clock();
	//how many clock cycles used here will depend on overall
	//machine activity/performance/number of running processes
	for (i=0; i < 100; i++) sleep(0);
	uname(&uts);
	ClocksEnd=clock();


	Tempstr=FormatStr(Tempstr,"%lu:%lu:%lu:%lu:%llu\n",getpid(),getuid(),ClocksStart,ClocksEnd,GetTime(TIME_MILLISECS));
	//This stuff should be unique to a machine
	Tempstr=CatStr(Tempstr, uts.sysname);
	Tempstr=CatStr(Tempstr, uts.nodename);
	Tempstr=CatStr(Tempstr, uts.machine);
	Tempstr=CatStr(Tempstr, uts.release);
	Tempstr=CatStr(Tempstr, uts.version);


	len=HashBytes(&RandomBytes, "sha256", Tempstr, StrLen(Tempstr), 0);
	if (len > ReqLen) len=ReqLen;
}


*RetBuff=EncodeBytes(*RetBuff, RandomBytes, len, Encoding);

DestroyString(Tempstr);
DestroyString(RandomBytes);

return(len);
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
TERA=TERABYTE;
}
else
{
KAY=KIBIBYTE;
MEG=MEGIBYTE;
GIG=GIGIBYTE;
TERA=TERIBYTE;
}

	val=strtod(Data,&ptr);
	while (isspace(*ptr)) ptr++;
	if (*ptr=='k') val=val * KAY;
	if (*ptr=='M') val=val * MEG;
	if (*ptr=='G') val=val * GIG;
	if (*ptr=='T') val=val * TERA;


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
	 if (val >= (GIG))
    {
      val=val / GIG;
      kMGT='G';
    }
    else if (val >= (MEG))
    {
      val=val / MEG;
      kMGT='M';

    }
    else if (val >= (KAY))
    {
      val=val /  KAY;
      kMGT='k';
    }

Str=FormatStr(Str,"%0.1f%c",(float) val,kMGT);
return(Str);
}



char *DecodeBase64(char *Return, int *len, char *Text)
{
char *RetStr;

RetStr=SetStrLen(Return,StrLen(Text) *2);
*len=from64tobits(RetStr,Text);

return(RetStr);
}

