#include "ssh.h"
#include "files.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define SSH_LOGON_DONE 1
#define SSH_ASK_PASSWD 2
#define SSH_REMOTE_HAS_HASHRAT 4
#define SSH_REMOTE_HAS_SHA1SUM 8
#define SSH_REMOTE_HAS_MD5SUM  16

static char *SSHGenerateReplayTerminator(char *RetStr, const char *Command)
{
	return(FormatStr(RetStr,"%s-%lu-%lu-%lu",Command,getpid(),time(NULL),rand()));
}

static void SSHRequestPasswd(char **Passwd)
{
STREAM *S;
char inchar;

S=STREAMFromFD(0);

//Turn off echo (and other things)
TTYConfig(0,0,0);
fprintf(stderr,"Password: "); fflush(NULL);
inchar=STREAMReadChar(S);
while ((inchar != EOF) && (inchar != '\n') && (inchar != '\r'))
{
	*Passwd=AddCharToStr(*Passwd,inchar);
	write(2,"*",1);
	inchar=STREAMReadChar(S);
}
StripCRLF(*Passwd);
//turn echo back on
TTYReset(0);

printf("\n");
STREAMDestroy(S);
}


static STREAM *SSHOpenHashrat(HashratCtx *Ctx, const char *URL, char **Path)
{
char *Tempstr=NULL;
const char *ptr;

ptr=URL+4; //past ssh:
while (*ptr=='/') ptr++;
ptr=strchr(ptr,'/');
if (! ptr) return(NULL);

Tempstr=CopyStrLen(Tempstr,URL,ptr-URL);
if (strncmp(ptr,"/./",3)==0) ptr+=3;
*Path=QuoteCharsInStr(*Path,ptr," 	;&'`\"");

if (! Ctx->NetCon) Ctx->NetCon=STREAMOpen(Tempstr, "r");

DestroyString(Tempstr);
return(Ctx->NetCon);
}




STREAM *SSHGet(HashratCtx *Ctx, const char *URL)
{
char *Tempstr=NULL, *Path=NULL;
const char *ptr;
STREAM *S;
struct timeval tv;

//collect any previous ssh's
while (waitpid(-1,NULL,WNOHANG) > 0);

//clean up bytes in the stream

S=SSHOpenHashrat(Ctx, URL, &Path);
if (S)
{
	Tempstr=MCopyStr(Tempstr,"cat '",Path,"' 2>/dev/null \n",NULL);
	STREAMWriteLine(Tempstr,S); 
	STREAMFlush(S);
	tv.tv_usec=0;
	tv.tv_sec=2;
	FDSelect(S->in_fd, SELECT_READ, &tv);
}

Destroy(Path);
Destroy(Tempstr);

return(S);
}


static void Decode_LS_Output(const char *Line, char **Path, struct stat *Stat)
{
char *Token=NULL;
const char *ptr, *tptr;

//219884 -rw-r--r-- 1 root        root     56980 Feb 13 18:06 filestore.o
		memset(Stat,0,sizeof(struct stat));

		ptr=Line;
		while (isspace(*ptr)) ptr++;
    ptr=GetToken(ptr,"\\S",&Token,0);
		Stat->st_ino=atol(Token);

		ptr=GetToken(ptr,"\\S",&Token,0);

		if (StrValid(Token) > 9)
		{
		tptr=Token;
    switch (*tptr)
    {
      case 'd': Stat->st_mode |=S_IFDIR; break;
      case 'l': Stat->st_mode |=S_IFLNK; break;
      case 'c': Stat->st_mode |=S_IFCHR; break;
      case 'b': Stat->st_mode |=S_IFBLK; break;
      case 's': Stat->st_mode |=S_IFSOCK; break;
      default: Stat->st_mode |=S_IFREG; break;
    }

		tptr++;
		if (*tptr=='r') Stat->st_mode |= S_IRUSR;
		tptr++;
		if (*tptr=='w') Stat->st_mode |= S_IWUSR;
		tptr++;
		if (*tptr=='x') Stat->st_mode |= S_IXUSR;

		tptr++;
		if (*tptr=='r') Stat->st_mode |= S_IRGRP;
		tptr++;
		if (*tptr=='w') Stat->st_mode |= S_IWGRP;
		tptr++;
		if (*tptr=='x') Stat->st_mode |= S_IXGRP;

		tptr++;
		if (*tptr=='r') Stat->st_mode |= S_IROTH;
		tptr++;
		if (*tptr=='w') Stat->st_mode |= S_IWOTH;
		tptr++;
		if (*tptr=='x') Stat->st_mode |= S_IXOTH;
		}

    ptr=GetToken(ptr,"\\S",&Token,0);

		//user id
    ptr=GetToken(ptr,"\\S",&Token,0);
		Stat->st_uid=atol(Token);

		//group id
    ptr=GetToken(ptr,"\\S",&Token,0);
		Stat->st_gid=atol(Token);

		//Size
    ptr=GetToken(ptr,"\\S",&Token,0);
		Stat->st_size=atol(Token);

		/*
    StripTrailingWhitespace(Token);
    FI->Size=atoi(Token);
		*/

		//Date Str
    ptr=GetToken(ptr,"\\S",&Token,0);
    ptr=GetToken(ptr,"\\S",&Token,0);
//    DateStr=MCatStr(DateStr," ",Token,NULL);
    ptr=GetToken(ptr,"\\S",&Token,0);
 //   DateStr=MCatStr(DateStr," ",Token,NULL);

		*Path=CopyStr(*Path,ptr);

	Destroy(Token);
}



int SSHGlob(HashratCtx *Ctx, const char *URL, ListNode *Files)
{
char *Tempstr=NULL, *Path=NULL, *TermLine=NULL, *wptr;
//don't make this const, we change a char with it
const char *ptr;
STREAM *S;
int RetVal=FT_FILE;
struct stat *Stat;

S=SSHOpenHashrat(Ctx, URL, &Path);
if (S)
{
	TermLine=SSHGenerateReplayTerminator(TermLine, "LIST");
	Tempstr=MCopyStr(Tempstr,"ls -Llidn ",Path," 2> /dev/null\necho ",TermLine,"\n",NULL);
	STREAMWriteLine(Tempstr,S); STREAMFlush(S);

	Tempstr=STREAMReadLine(Tempstr,S);
	while (Tempstr)
	{
		StripTrailingWhitespace(Tempstr);
		if (strcmp(Tempstr,TermLine)==0) break;
		Stat=(struct stat *) calloc(1,sizeof(struct stat));
		Decode_LS_Output(Tempstr, &Path, Stat);
		if (S_ISDIR(Stat->st_mode)) RetVal=FT_DIR;
		if (StrValid(Path))
		{
			Tempstr=MCatStr(Tempstr,"/",Path,NULL);
			if (Files) ListAddNamedItem(Files,Tempstr,Stat);
			else free(Stat);
		}
		else free(Stat);
		Tempstr=STREAMReadLine(Tempstr,S);
	}

}

//Don't close 'S', it is reused as Ctx->NetCon
//STREAMClose(S);
Destroy(Path);
Destroy(Tempstr);
Destroy(TermLine);

return(RetVal);
}
