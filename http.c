#include "files.h"
#include "http.h"


int HTTPHost(const char *URL, char **Host)
{
const char *ptr=NULL, *eptr;
int result=FALSE;

//return TRUE but don't clip protocol off URL. This will cause it to fail the 'same origin check' in HTTPBuildURL
if (strncmp(URL, "mailto:", 7) ==0) 
{
ptr=URL;
result=TRUE;
}
else if (strncmp(URL, "http:", 5) ==0) ptr=URL+5;
else if (strncmp(URL, "https:", 6) ==0) ptr=URL+6;
else if (strncmp(URL, "//",2)==0) ptr=URL;


while (ptr && (*ptr=='/')) ptr++;

if (ptr) result=TRUE;
if (Host)
{
	if (! ptr) *Host=CopyStr(*Host, "");
	else
	{
	eptr=strchr(ptr, '/');
	if (! eptr) eptr=strchr(ptr, '?');

	if (eptr) *Host=CopyStrLen(*Host, URL, eptr-URL);
	else *Host=CopyStr(*Host, URL);
	}
}

return(result);
}


static char *HTTPBuildURL(char *RetStr, const char *Parent, const char *Value)
{
char *ptr, *Host=NULL;

RetStr=CopyStr(RetStr,"");

//if ptr then we got an absolute URL starting with https: or http:
if (HTTPHost(Value, &Host))
{
	//same origin check. If host doesn't match our parent host, don't touch it
	if (strcasecmp(Parent, Host) !=0)
	{
		Destroy(Host);
		return(RetStr);
	}
} 
else if (StrValid(Parent)) 
{
	// you can have a URL of the form '?n=10' which is just recalling the current URL with new arguments. We ignore this
	if (Value[0]=='?') return(RetStr);

	//if the URL starts with '/' then it needs the host prepended to it
	if (Value[0] =='/') 
	{
		//Copy Host into RetStr. No '/' needed after as Value already has that
		HTTPHost(Parent, &RetStr);
	}
	//if it doesn't start with '/' then it needs the whole parent path appended
	else RetStr=MCopyStr(RetStr, Parent, "/", NULL);
}


ptr=strchr(Value, '?');
if (ptr) RetStr=CatStrLen(RetStr, Value, ptr-Value);
else RetStr=CatStr(RetStr, Value);

Destroy(Host);
return(RetStr);
}


static void HTTPGlobAdd(ListNode *Items, const char *Parent, const char *Data)
{
struct stat *Stat;
char *Name=NULL, *Value=NULL, *Host=NULL, *Tempstr=NULL;
const char *ptr;

if (! HTTPHost(Parent, &Host)) Host=CopyStr(Host, Parent);

ptr=GetNameValuePair(Data, "\\S","=",&Name, &Value);
while (ptr)
{
	//just add a blank stat
	if ( (strcmp(Name, "src")==0) || (strcmp(Name, "href")==0))
	{
//printf("ADD: [%s]  [%s]\n", Value, Parent);
		//don't add 'self' links that point back to this document
		if (strcmp(Value, Parent) !=0)
		{
			Stat=(struct stat *) calloc(1, sizeof(struct stat));
			Tempstr=HTTPBuildURL(Tempstr, Parent, Value);
			if (StrValid(Tempstr)) ListAddNamedItem(Items, Tempstr, Stat);
		}
	}

	ptr=GetNameValuePair(ptr, "\\S","=",&Name, &Value);
}

Destroy(Tempstr);
Destroy(Value);
Destroy(Name);
Destroy(Host);
}



int HTTPGlob(HashratCtx *Ctx, const char *Path, ListNode *Dirs)
{
STREAM *S;
char *Tempstr=NULL, *Name=NULL, *Data=NULL, *Parent=NULL;
char *wptr, *start;
const char *ptr;


Parent=CopyStr(Parent, Path);
start=Parent;
if (strncmp(start, "https:",6)==0) start+=6;
if (strncmp(start, "http:",5)==0) start+=5;
while (*start=='/') start++;
wptr=strrchr(start, '/');
if ((wptr) && (wptr > start)) *wptr='\0';


S=STREAMOpen(Path, "r");
if (S)
{
Tempstr=STREAMReadDocument(Tempstr, S);
STREAMClose(S);
}

if (Tempstr)
{
	ptr=XMLGetTag(Tempstr, NULL, &Name, &Data);
	while (ptr)
	{
		switch (*Name)
		{
		case 'a':
		case 'A':
		if (strcasecmp(Name, "a")==0) HTTPGlobAdd(Dirs, Parent, Data);
		break;
		
		case 'i':
		case 'I':
		if (strcmp(Name, "img")==0) HTTPGlobAdd(Dirs, Parent, Data);
		break;
		
		case 's':
		case 'S':
		if (strcmp(Name, "script")==0) HTTPGlobAdd(Dirs, Parent, Data);
		break;
		
		case '!':
		/*
		// html comment
		while (ptr && (strcmp(Name, "--") !=0) )
		{
			//terminator actually tends to show up as data rather than name
		printf("Comment: %s\n",Data);
			if (strcmp(Data, "--") ==0) break;
			ptr=XMLGetTag(ptr, NULL, &Name, &Data);
		}
		*/
		break;
		}
		
		ptr=XMLGetTag(ptr, NULL, &Name, &Data);
	}
}

Destroy(Tempstr);
Destroy(Parent);
Destroy(Name);
Destroy(Data);

return(FT_DIR);
}


int HTTPStat(HashratCtx *Ctx, const char *Path, struct stat *FStat)
{
STREAM *S;
const char *ptr;
int result=FT_FILE;

memset(FStat, 0, sizeof(struct stat));
S=STREAMOpen(Path, "H");
if (S)
{
ptr=STREAMGetValue(S, "HTTP:Content-Length");
if (StrValid(ptr)) FStat->st_size=atoi(ptr);
ptr=STREAMGetValue(S, "HTTP:Content-Type");
if (StrValid(ptr) && (strncmp(ptr,"text/html",9)==0)) 
{
	result=FT_DIR;
}
FStat->st_mode |= 0444;
STREAMClose(S);
}

return(result);
}

