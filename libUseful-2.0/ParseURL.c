#include "ParseURL.h"


void ParseHostDetails(char *Data,char **Host,char **Port,char **User, char **Password)
{
char *Token=NULL, *ptr, *tptr;

if (Port) *Port=CopyStr(*Port,"");
if (Host) *Host=CopyStr(*Host,"");
if (User) *User=CopyStr(*User,"");
if (Password) *Password=CopyStr(*Password,"");

ptr=strrchr(Data,'@');
if (ptr)
{
	ptr=GetToken(Data,"@",&Token,0);
	if (User)
	{
	tptr=GetToken(Token,":",User,0);
	if (StrLen(tptr)) *Password=CopyStr(*Password,tptr);
	}
}
else ptr=Data;

if (Host) ptr=GetToken(ptr,":",Host,0);
if (Port && StrLen(ptr)) *Port=CopyStr(*Port,ptr);

DestroyString(Token);
}





void ParseURL(char *URL, char **Proto, char **Host, char **Port, char **User, char **Password, char **Path, char **Args)
{
char *ptr, *aptr;
char *Token=NULL, *tProto=NULL;


//Even if they pass NULL for protocol, we need to take a copy for use in
//the 'guess the port' section below
ptr=strchr(URL,':');
if (ptr)
{
	tProto=CopyStrLen(tProto,URL,ptr-URL);
	strlwr(tProto);
	if (Proto) *Proto=CopyStr(*Proto,tProto);
	
	ptr++;
	//some number of '//' follow protocol
	while (*ptr=='/') ptr++;

	ptr=GetToken(ptr,"/",&Token,0);
	ParseHostDetails(Token,Host,Port,User,Password);
}
else ptr=URL;

while (*ptr=='/') ptr++;

if (ptr)
{
if (Path) 
{
	*Path=MCopyStr(*Path,"/",ptr,NULL);

	//Only split the HTTP CGI arguments from the document path if we were 
	//asked to return the args seperately
	if (Args)
	{
		aptr=strrchr(*Path,'?');
		if (aptr) 
		{
		*aptr='\0';
		aptr++;
		*Args=CopyStr(*Args,aptr);
		}
	}
}
}

//the 'GetToken' call will have thrown away the '/' at the start of the path
//add it back in

if (Port && (! StrLen(*Port)) && StrLen(tProto))
{
	if (strcmp(tProto,"http")==0) *Port=CopyStr(*Port,"80");
	else if (strcmp(tProto,"https")==0) *Port=CopyStr(*Port,"443");
	else if (strcmp(tProto,"ssh")==0) *Port=CopyStr(*Port,"22");
	else if (strcmp(tProto,"ftp")==0) *Port=CopyStr(*Port,"21");
	else if (strcmp(tProto,"telnet")==0) *Port=CopyStr(*Port,"23");
	else if (strcmp(tProto,"smtp")==0) *Port=CopyStr(*Port,"25");
	else if (strcmp(tProto,"mailto")==0) *Port=CopyStr(*Port,"25");

}


DestroyString(Token);
DestroyString(tProto);
}






void ParseConnectDetails(char *Str, char **Type, char **Host, char **Port, char **User, char **Pass, char **Path)
{
char *ptr, *Token=NULL, *Args=NULL;

ptr=GetToken(Str," ",&Token,0);
ParseURL(Token, Type, Host, Port, User, Pass, Path, &Args);

if (Path && StrLen(Args)) *Path=MCatStr(*Path,"?",Args,NULL);

while (ptr)
{
	if (strcmp(Token,"-password")==0) ptr=GetToken(ptr," ",Pass,0);
	else if (strcmp(Token,"-keyfile")==0)
	{
		ptr=GetToken(ptr," ",&Token,0);
		*Pass=MCopyStr(*Pass,"keyfile:",Token,NULL);
	}
ptr=GetToken(ptr," ",&Token,0);
}

DestroyString(Token);
DestroyString(Args);
}



