#include "ssh.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

#define SSH_LOGON_DONE 1
#define SSH_ASK_PASSWD 2

char *SSHGenerateReplayTerminator(char *RetStr, const char *Command)
{
	return(FormatStr(RetStr,"%s-%lu-%lu-%lu",Command,getpid(),time(NULL),rand()));
}

void SSHRequestPasswd(char **Passwd)
{
STREAM *S;
char inchar;

S=STREAMFromFD(0);

//Turn off echo (and other things)
InitTTY(0,0,0);
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
ResetTTY(0);

printf("\n");
STREAMDisassociateFromFD(S);
}



int SSHFinalizeConnection(HashratCtx *Ctx, int Flags, char **Passwd)
{
char *Tempstr=NULL, *Line=NULL;
ListNode *Dialog=NULL;
int result=TRUE;


	STREAMSetFlushType(Ctx->NetCon,FLUSH_ALWAYS,0,0);
	if (Flags & SSH_ASK_PASSWD) 
	{
		*Passwd=CopyStr(*Passwd, GetVar(Ctx->Vars,"SshPasswd"));
		if (! StrLen(*Passwd))
		{
		SSHRequestPasswd(Passwd);
		SetVar(Ctx->Vars,"SshPasswd", *Passwd);
		}
	}

	Tempstr=SSHGenerateReplayTerminator(Tempstr, "CONNECTED1");
	STREAMWriteLine("echo ",Ctx->NetCon);
	STREAMWriteLine(Tempstr,Ctx->NetCon);
	STREAMWriteLine("\n",Ctx->NetCon);

  Dialog=ListCreate();
  ExpectDialogAdd(Dialog, "Are you sure you want to continue connecting (yes/no)?", "yes\n", DIALOG_OPTIONAL);
  ExpectDialogAdd(Dialog, "Permission denied", "", DIALOG_OPTIONAL | DIALOG_FAIL);
  ExpectDialogAdd(Dialog, Tempstr, "", DIALOG_END);
	if (StrLen(*Passwd))
	{
  Tempstr=MCopyStr(Tempstr,*Passwd,"\n",NULL);
  ExpectDialogAdd(Dialog, "assword:", Tempstr, DIALOG_END);
	}
  else ExpectDialogAdd(Dialog, "assword:", "", DIALOG_OPTIONAL | DIALOG_FAIL);

  STREAMExpectDialog(Ctx->NetCon, Dialog);
  ListDestroy(Dialog,ExpectDialogDestroy);


	STREAMSetFlushType(Ctx->NetCon,FLUSH_LINE,0,0);

	//allow time for stty to take effect!
	STREAMWriteLine("PS1=\n",Ctx->NetCon); 
	STREAMWriteLine("PS2=\n",Ctx->NetCon);
	STREAMWriteLine("stty -echo\necho\n",Ctx->NetCon);
	STREAMFlush(Ctx->NetCon);


	//Do this again so we know we're beyond any crap produced by our setup of the terminal	
	Tempstr=SSHGenerateReplayTerminator(Tempstr, "CONNECTED2");
	STREAMWriteLine("echo ",Ctx->NetCon);
	STREAMWriteLine(Tempstr,Ctx->NetCon);
	STREAMWriteLine("\n",Ctx->NetCon);


	Line=STREAMReadLine(Line,Ctx->NetCon);
	while (Line)
	{
	StripTrailingWhitespace(Line);
	if (strcmp(Line,Tempstr)==0) break;
	Line=STREAMReadLine(Line,Ctx->NetCon);
	}


DestroyString(Tempstr);
DestroyString(Line);
return(result);
}



STREAM *SSHConnect(char *URL, char **Path, HashratCtx *Ctx)
{
char *Tempstr=NULL, *Host=NULL, *PortStr=NULL, *User=NULL, *Passwd=NULL, *ptr;
int Port=22, Flags=0;

ParseURL(URL, NULL, &Host, &PortStr, &User, &Passwd, &Tempstr, NULL);
*Path=MCopyStr(*Path,"/",Tempstr,NULL);
if (! Ctx->NetCon) 
{
Ctx->NetCon=STREAMCreate();

if (StrLen(PortStr)) Port=atoi(PortStr);

Tempstr=FormatStr(Tempstr,"/usr/bin/ssh -2 -T %s@%s -p %d",User,Host,Port);

ptr=GetVar(Ctx->Vars,"SshIdFile");
if (StrLen(ptr)) Tempstr=MCatStr(Tempstr," -i ",ptr, NULL);

//Never use TTYFLAG_CANON here
Ctx->NetCon=STREAMSpawnCommand(Tempstr,COMMS_BY_PTY|TTYFLAG_CRLF|TTYFLAG_IGNSIG);

if ((! StrLen(ptr)) && (! StrLen(Passwd)))  Flags |= SSH_ASK_PASSWD;
SSHFinalizeConnection(Ctx, Flags, &Passwd);

STREAMSetTimeout(Ctx->NetCon,10000);

/*
StripTrailingWhitespace(Tempstr);
StripTrailingWhitespace(User);

if ( (StrLen(User)==0) || (strcmp(Tempstr,User) !=0) )
{
	STREAMClose(S);
	S=NULL;
}
*/

}

DestroyString(User);
DestroyString(Passwd);
DestroyString(Host);
DestroyString(Tempstr);
DestroyString(PortStr);

return(Ctx->NetCon);
}



STREAM *SSHGet(HashratCtx *Ctx, char *URL)
{
char *Tempstr=NULL, *Path=NULL, *ptr;
STREAM *S;
struct timeval tv;

//collect any previous ssh's
while (waitpid(-1,NULL,WNOHANG) > 0);

//clean up bytes in the stream

S=SSHConnect(URL, &Path, Ctx);
if (S)
{
	ptr=Path;
	if (*ptr=='/') ptr++;
	Tempstr=MCopyStr(Tempstr,"cat '",ptr,"' 2>/dev/null \n",NULL);
	STREAMWriteLine(Tempstr,S); 
	STREAMFlush(S);
	tv.tv_usec=0;
	tv.tv_sec=2;
	FDSelect(S->in_fd, SELECT_READ, &tv);
}

DestroyString(Path);
DestroyString(Tempstr);

return(S);
}


void Decode_LS_Output(char *Line, char **Path, struct stat *Stat)
{
char *Token=NULL, *ptr, *tptr;

//219884 -rw-r--r-- 1 root        root     56980 Feb 13 18:06 filestore.o
		memset(Stat,0,sizeof(struct stat));

		ptr=Line;
		while (isspace(*ptr)) ptr++;
    ptr=GetToken(ptr,"\\S",&Token,0);
		Stat->st_ino=atol(Token);

		ptr=GetToken(ptr,"\\S",&Token,0);

		if (StrLen(Token) > 9)
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

	DestroyString(Token);
}



int SSHGlob(HashratCtx *Ctx, char *URL, ListNode *Files)
{
char *Tempstr=NULL, *Path=NULL, *TermLine=NULL, *ptr;
STREAM *S;
int count=0;
struct stat *Stat;


S=SSHConnect(URL, &Tempstr, Ctx);
if (S)
{
	ptr=Tempstr;
	if (*ptr == '/') ptr++;
	
	Path=QuoteCharsInStr(Path,ptr," 	;&'`\"");
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
		Tempstr=CopyStr(Tempstr,URL);
		ptr=Tempstr+4; //go past 'ssh:'
		while (*ptr=='/') ptr++;
		ptr=strchr(ptr,'/');
		if (ptr) *ptr='\0';

		if (StrLen(Path))
		{
			Tempstr=MCatStr(Tempstr,"/",Path,NULL);
			if (Files) ListAddNamedItem(Files,Tempstr, Stat);
			else free(Stat);
			count++;
		}
		else free(Stat);
		Tempstr=STREAMReadLine(Tempstr,S);
	}

}

//Don't close 'S', it is reused as Ctx->NetCon
//STREAMClose(S);
DestroyString(Path);
DestroyString(Tempstr);
DestroyString(TermLine);

return(count);
}
