#include "URL.h"


static const char *ParseHostDetailsExtractAuth(const char *Data, char **User, char **Password)
{
    char *Token=NULL;
    const char *ptr, *tptr;

    ptr=strrchr(Data,'@');
    if (ptr)
    {
        //in some cases there will be an '@' in the username, so GETTOKEN_QUOTES
        //should handle any @ which is prefixed with a \ to quote it out
        ptr=GetToken(Data,"@",&Token, GETTOKEN_QUOTES);
        if (User)
        {
            tptr=GetToken(Token,":",User,0);
            if (StrValid(tptr) && Password) *Password=CopyStr(*Password,tptr);
        }
    }
    else ptr=Data;

    Destroy(Token);

    return(ptr);
}


const char *ParseHostDetails(const char *Data, char **Host,char **Port,char **User, char **Password)
{
    char *Token=NULL;
    const char *ptr, *tptr;

    if (Port) *Port=CopyStr(*Port, "");
    if (Host) *Host=CopyStr(*Host, "");
    if (User) *User=CopyStr(*User, "");
    if (Password) *Password=CopyStr(*Password, "");

    //if a user:password@host part exists in the URL, then this will extract it
    ptr=ParseHostDetailsExtractAuth(Data, User, Password);

//IP6 addresses, because it's a badly designed protocol, use the same separator as used to identify the port
//hence we have to enclose them in square braces, which we must handle here
    if (*ptr == '[')
    {
        tptr=ptr+1;
        while ((*ptr !=']') && (*ptr !='\0')) ptr++;
        if (Host) *Host=CopyStrLen(*Host, tptr, ptr-tptr);
    }
    else
    {
        ptr=GetToken(ptr, ":", &Token, 0);
        if (Host) *Host=CopyStr(*Host, Token);
    }

    if (Port) *Port=CopyStr(*Port, ptr);

    DestroyString(Token);

    return(ptr);
}


void ParseURL(const char *URL, char **Proto, char **Host, char **Port, char **User, char **Password, char **Path, char **Args)
{
    const char *ptr;
    char *Token=NULL, *tProto=NULL, *aptr;

//we might not return a protocol!
    if (Proto) *Proto=CopyStr(*Proto,"");

//Even if they pass NULL for protocol, we need to take a copy for use in
//the 'guess the port' section below
    ptr=strchr(URL,':');
    if (ptr)
    {
        tProto=CopyStrLen(tProto,URL,ptr-URL);
        strlwr(tProto);
        aptr=strchr(tProto, '.');
        if (aptr)
        {
            //protocol name is not allowed to contain '.', so this must be a hostname or
            //ip address.
            ptr=URL;
        }
        else
        {
            if (Proto) *Proto=CopyStr(*Proto,tProto);
            ptr++;
            //some number of '//' follow protocol
            while (*ptr=='/') ptr++;
        }
    }
    else ptr=URL;

    // either we've cut out a protocol, or we haven't. If not the next thing is going to be the hostname
    // maybe there we be a path coming after '/', even if '/' is absent this GetToken will return the Host part
    ptr=GetToken(ptr,"/|?",&Token,GETTOKEN_MULTI_SEP);
    ParseHostDetails(Token, Host, Port, User, Password);

    if (StrValid(ptr))
    {
        if (Path)
        {
            *Path=MCopyStr(*Path,"/",ptr,NULL);

            //Only split the HTTP CGI arguments from the document path if we were
            //asked to return the args seperately
            if (Args)
            {
                aptr=strchr(*Path,'?');
                if (! aptr) aptr=strchr(*Path,'#');
                if (aptr)
                {
                    StrTrunc(*Path, aptr-*Path);
                    aptr++;
                    *Args=CopyStr(*Args,aptr);
                }
            }
        }
    }

//the 'GetToken' call will have thrown away the '/' at the start of the path
//add it back in

    if (Port && (! StrValid(*Port)) && StrValid(tProto))
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



void ParseConnectDetails(const char *Str, char **Type, char **Host, char **Port, char **User, char **Pass, char **Path)
{
    char *Token=NULL, *Args=NULL;
    const char *ptr;

    ptr=GetToken(Str," ",&Token,0);
    ParseURL(Token, Type, Host, Port, User, Pass, Path, &Args);

    if (Path && StrValid(Args)) *Path=MCatStr(*Path,"?",Args,NULL);

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



char *ResolveURL(char *RetStr, const char *Parent, const char *SubItem)
{
    char *Proto=NULL, *Host=NULL, *Port=NULL, *Path=NULL;
    char *BasePath=NULL;

    //if SubItem contains a '://' then it's a full url
    if (strstr(SubItem, "://")) return(CopyStr(RetStr, SubItem));

    ParseURL(Parent,&Proto,&Host,&Port,NULL,NULL,&Path,NULL);
    if (StrValid(Port)) BasePath=FormatStr(BasePath, "%s://%s:%s/", Proto,Host,Port);
    else BasePath=FormatStr(BasePath, "%s://%s/", Proto,Host);

//if it starts with '/' then we don't append to existing path
    if (*SubItem=='/') RetStr=MCopyStr(RetStr, BasePath, SubItem, NULL);
    else RetStr=MCopyStr(RetStr, BasePath, Path, "/", SubItem, NULL);

    DestroyString(Proto);
    DestroyString(Host);
    DestroyString(Port);
    DestroyString(Path);
    DestroyString(BasePath);

    return(RetStr);
}

