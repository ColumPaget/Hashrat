#include "Http.h"
#include "DataProcessing.h"
#include "ConnectionChain.h"
#include "Hash.h"
#include "URL.h"
#include "OAuth.h"
#include "Time.h"
#include "base64.h"
#include "SecureMem.h"
#include "Errors.h"


#define HTTP_OKAY 0
#define HTTP_NOCONNECT 1
#define HTTP_NOTFOUND 2
#define HTTP_REDIRECT 3
#define HTTP_ERROR 4
#define HTTP_CIRCULAR_REDIRECTS 5
#define HTTP_NOTMODIFIED 6


#define HTTP_HEADERS_SENT 1
#define HTTP_CLIENTDATA_SENT 2
#define HTTP_HEADERS_READ 4



const char *HTTP_AUTH_BY_TOKEN="AuthTokenType";
static ListNode *Cookies=NULL;
static int g_HTTPFlags=0;



void HTTPInfoDestroy(void *p_Info)
{
    HTTPInfoStruct *Info;

    if (! p_Info) return;
    Info=(HTTPInfoStruct *) p_Info;
    DestroyString(Info->Protocol);
    DestroyString(Info->Method);
    DestroyString(Info->Host);
    DestroyString(Info->Doc);
    DestroyString(Info->UserName);
    DestroyString(Info->Destination);
    DestroyString(Info->ResponseCode);
    DestroyString(Info->PreviousRedirect);
    DestroyString(Info->RedirectPath);
    DestroyString(Info->ContentType);
    DestroyString(Info->Timestamp);
    DestroyString(Info->PostData);
    DestroyString(Info->PostContentType);
    DestroyString(Info->Proxy);
    DestroyString(Info->Credentials);
    DestroyString(Info->Authorization);
    DestroyString(Info->ProxyAuthorization);
    DestroyString(Info->ConnectionChain);

    ListDestroy(Info->ServerHeaders,Destroy);
    ListDestroy(Info->CustomSendHeaders,Destroy);
    free(Info);
}




//These functions relate to adding a 'Data processor' to the stream that
//will decode chunked HTTP transfers

typedef struct
{
    char *Buffer;
    size_t ChunkSize;
    size_t BuffLen;
} THTTPChunk;



int HTTPChunkedInit(TProcessingModule *Mod, const char *Args)
{
    Mod->Data=(THTTPChunk *) calloc(1, sizeof(THTTPChunk));

    return(TRUE);
}



int HTTPChunkedRead(TProcessingModule *Mod, const char *InBuff, unsigned long InLen, char **OutBuff, unsigned long *OutLen, int Flush)
{
    size_t len=0, bytes_out=0;
    THTTPChunk *Chunk;
    char *ptr, *vptr, *end;

    Chunk=(THTTPChunk *) Mod->Data;
    if (InLen > 0)
    {
        len=Chunk->BuffLen+InLen;
        Chunk->Buffer=SetStrLen(Chunk->Buffer,len);
        ptr=Chunk->Buffer+Chunk->BuffLen;
        memcpy(ptr,InBuff,InLen);
        Chunk->Buffer[len]='\0';
        Chunk->BuffLen=len;
    }
    else len=Chunk->BuffLen;

    end=Chunk->Buffer+Chunk->BuffLen;

    if (Chunk->ChunkSize==0)
    {
        //if chunksize == 0 then read the size of the next chunk

        //if there's nothing in our buffer, and nothing being added, then
        //we've already finished!
        if ((Chunk->BuffLen==0) && (InLen==0)) return(STREAM_CLOSED);

        ptr=Chunk->Buffer;
        vptr=ptr;
        //skip past any leading '\r' or '\n'
        if (*vptr=='\r') vptr++;
        if (*vptr=='\n') vptr++;

        ptr=memchr(vptr,'\n', end-vptr);

        //sometimes people seem to miss off the final '\n', so if we get told there's no more data
        //we should use a '\r' if we've got one
        if ((! ptr) && (InLen < 1))
        {
            ptr=memchr(vptr,'\r',end-vptr);
            if (! ptr) ptr=end;
        }


        if (ptr)
        {
            *ptr='\0';
            ptr++;
        }
        else return(0);

        Chunk->ChunkSize=strtol(vptr,NULL,16);
        //if we got chunksize of 0 then we're done, return STREAM_CLOSED
        if (Chunk->ChunkSize==0) return(STREAM_CLOSED);

        Chunk->BuffLen=end-ptr;

        if (Chunk->BuffLen > 0)	memmove(Chunk->Buffer,ptr, Chunk->BuffLen);
        //in case it went negative in the above calcuation
        else Chunk->BuffLen=0;

        //maybe we have a full chunk already? Set len to allow us to use it
        len=Chunk->BuffLen;
    }


//either path we've been through above can result in a full chunk in the buffer
    if (len >= Chunk->ChunkSize)
    {
        bytes_out=Chunk->ChunkSize;

        //We should really grow OutBuff to take all the data
        //but for the sake of simplicity we'll just use the space
        //supplied
        if (bytes_out > *OutLen) bytes_out=*OutLen;
        memcpy(*OutBuff,Chunk->Buffer,bytes_out);

        ptr=Chunk->Buffer + bytes_out;
        Chunk->BuffLen   -= bytes_out;
        Chunk->ChunkSize -= bytes_out;
        memmove(Chunk->Buffer, ptr, end-ptr);

    }

    if (Chunk->ChunkSize < 0) Chunk->ChunkSize=0;

    return(bytes_out);
}



int HTTPChunkedClose(TProcessingModule *Mod)
{
    THTTPChunk *Chunk;

    Chunk=(THTTPChunk *) Mod->Data;
    DestroyString(Chunk->Buffer);
    free(Chunk);

    return(TRUE);
}


void HTTPAddChunkedProcessor(STREAM *S)
{
    TProcessingModule *Mod=NULL;

    Mod=(TProcessingModule *) calloc(1,sizeof(TProcessingModule));
    Mod->Name=CopyStr(Mod->Name,"HTTP:Chunked");
    Mod->Init=HTTPChunkedInit;
    Mod->Read=HTTPChunkedRead;
    Mod->Close=HTTPChunkedClose;

    Mod->Init(Mod, "");
    STREAMAddDataProcessor(S,Mod,"");
}



char *HTTPUnQuote(char *RetBuff, const char *Str)
{
    char *RetStr=NULL, *Token=NULL;
    const char *ptr;
    int olen=0, ilen;

    RetStr=CopyStr(RetStr,"");
    ilen=StrLen(Str);

    for (ptr=Str; ptr < (Str+ilen); ptr++)
    {
        switch (*ptr)
        {
        case '+':
            RetStr=AddCharToBuffer(RetStr,olen,' ');
            olen++;
            break;

        case '%':
            ptr++;
            Token=CopyStrLen(Token,ptr,2);
            ptr++; //not +=2, as we will increment again
            RetStr=AddCharToBuffer(RetStr,olen,strtol(Token,NULL,16) & 0xFF);
            olen++;
            break;

        default:
            RetStr=AddCharToBuffer(RetStr,olen,*ptr);
            olen++;
            break;
        }

    }

		StrLenCacheAdd(RetStr, olen);

    DestroyString(Token);
    return(RetStr);
}


char *HTTPQuoteChars(char *RetBuff, const char *Str, const char *CharList)
{
    char *RetStr=NULL, *Token=NULL;
    const char *ptr;
    int olen=0, ilen;

    RetStr=CopyStr(RetStr,"");
    ilen=StrLen(Str);

    for (ptr=Str; ptr < (Str+ilen); ptr++)
    {
        if (strchr(CharList,*ptr))
        {
					// replacing ' ' with '+' should work, but some servers seem to no longer support it
					/*
						if (*ptr==' ')
						{
            RetStr=AddCharToBuffer(RetStr,olen,'+');
						olen++;
						}
						else
					*/
						{
            Token=FormatStr(Token,"%%%02X",*ptr);
						StrLenCacheAdd(RetStr, olen);
            RetStr=CatStr(RetStr,Token);
            olen+=StrLen(Token);
						}
        }
        else
        {
            RetStr=AddCharToBuffer(RetStr,olen,*ptr);
            olen++;
        }
    }


    RetStr[olen]='\0';
		StrLenCacheAdd(RetStr, olen);

    DestroyString(Token);
    return(RetStr);
}



char *HTTPQuote(char *RetBuff, const char *Str)
{
return(HTTPQuoteChars(RetBuff, Str, " \t\r\n\"#%()[]{}?&!,+':;/"));
}


void HTTPInfoSetValues(HTTPInfoStruct *Info, const char *Host, int Port, const char *Logon, const char *Password, const char *Method, const char *Doc, const char *ContentType, int ContentLength)
{
    Info->State=0;
    Info->PostData=CopyStr(Info->PostData,"");
    Info->Host=CopyStr(Info->Host,Host);
    if (Port > 0) Info->Port=Port;
    else Info->Port=0;
    Info->Method=CopyStr(Info->Method,Method);
    Info->Doc=CopyStr(Info->Doc,Doc);
    Info->PostContentType=CopyStr(Info->PostContentType,ContentType);
    Info->PostContentLength=ContentLength;
    Info->UserName=CopyStr(Info->UserName, Logon);

    if (StrValid(Password)) CredsStoreAdd(Host, Logon, Password);
}




HTTPInfoStruct *HTTPInfoCreate(const char *Protocol, const char *Host, int Port, const char *Logon, const char *Password, const char *Method, const char *Doc, const char *ContentType, int ContentLength)
{
    HTTPInfoStruct *Info;
    const char *ptr;

    Info=(HTTPInfoStruct *) calloc(1,sizeof(HTTPInfoStruct));
    Info->Protocol=CopyStr(Info->Protocol,Protocol);
    HTTPInfoSetValues(Info, Host, Port, Logon, Password, Method, Doc, ContentType, ContentLength);

    Info->ServerHeaders=ListCreate();
    Info->CustomSendHeaders=ListCreate();
    SetVar(Info->CustomSendHeaders,"Accept","*/*");

    if (g_HTTPFlags) Info->Flags=g_HTTPFlags;

    ptr=LibUsefulGetValue("HTTP:Proxy");
    if (StrValid(ptr))
    {
        Info->Proxy=CopyStr(Info->Proxy,ptr);
        strlwr(Info->Proxy);
        if (strncmp(Info->Proxy,"http:",5)==0) Info->Flags |= HTTP_PROXY;
        else if (strncmp(Info->Proxy,"https:",6)==0) Info->Flags |= HTTP_PROXY;
        else Info->Flags=HTTP_TUNNEL;
    }

    Info->UserAgent=CopyStr(Info->UserAgent, LibUsefulGetValue("HTTP:UserAgent"));

    return(Info);
}

char *HTTPInfoToURL(char *RetBuff, HTTPInfoStruct *Info)
{
    char *p_proto;
    char *Doc=NULL, *RetStr=NULL;

    if (Info->Flags & HTTP_SSL) p_proto="https";
    else p_proto="http";

    Doc=HTTPQuoteChars(Doc,Info->Doc," ");
    RetStr=FormatStr(RetBuff,"%s://%s:%d%s",p_proto,Info->Host,Info->Port,Info->Doc);

    DestroyString(Doc);
    return(RetStr);
}


void HTTPInfoPOSTSetContent(HTTPInfoStruct *Info, const char *ContentType, const char *ContentData, int ContentLength, int Flags)
{
//if there were arguments in URL, and HTTP_POSTARGS is set, then post these
//otherwise include them in the URL again
    if (Flags & HTTP_POSTARGS)
    {
        if (StrValid(ContentData))
        {
            if (ContentLength == 0) ContentLength=StrLen(ContentData);

            Info->PostData=SetStrLen(Info->PostData,ContentLength);
            memcpy(Info->PostData, ContentData, ContentLength);
            Info->PostContentLength=StrLen(Info->PostData);
        }

        if (StrValid(ContentType)) Info->PostContentType=CopyStr(Info->PostContentType,ContentType);
        else if (ContentLength > 0) Info->PostContentType=CopyStr(Info->PostContentType,"application/x-www-form-urlencoded; charset=UTF-8");

        if (ContentLength) Info->PostContentLength=ContentLength;
    }
    else if (StrValid(ContentData))
    {
        Info->Doc=MCatStr(Info->Doc,"?",ContentData, NULL);
        Info->PostData=CopyStr(Info->PostData, "");
    }
}

void HTTPInfoSetURL(HTTPInfoStruct *Info, const char *Method, const char *iURL)
{
    char *URL=NULL, *Proto=NULL, *User=NULL, *Pass=NULL, *Token=NULL, *Args=NULL, *Value=NULL;
    const char *p_URL, *ptr;

    ptr=GetToken(iURL, "\\S", &URL, 0);
    p_URL=strrchr(URL,'|');
    if (p_URL)
    {
        Info->ConnectionChain=CopyStrLen(Info->ConnectionChain, URL, p_URL - URL);
        p_URL++;
    }
    else p_URL=URL;

    if (strcasecmp(Method,"POST")==0) ParseURL(p_URL, &Proto, &Info->Host, &Token, &User, &Pass, &Info->Doc, &Args);
    else ParseURL(p_URL, &Proto, &Info->Host, &Token, &User, &Pass, &Info->Doc, NULL);

    if (StrValid(Token)) Info->Port=atoi(Token);

    if (StrValid(User) || StrValid(Pass)) Info->UserName=CopyStr(Info->UserName, User);

    if (StrValid(Proto) && (strcmp(Proto,"https")==0)) Info->Flags |= HTTP_SSL;


    ptr=GetNameValuePair(ptr,"\\S","=",&Token, &Value);
    while (ptr)
    {
        if (strcasecmp(Token, "oauth")==0)
        {
            Info->AuthFlags |= HTTP_AUTH_OAUTH;
            Info->Credentials=CopyStr(Info->Credentials, Value);
        }
        else if (strcasecmp(Token, "hostauth")==0) Info->AuthFlags |= HTTP_AUTH_HOST;
        else if (strcasecmp(Token, "http1.0")==0) Info->Flags |= HTTP_VER1_0;
        else if (strcasecmp(Token, "content-type")==0)   Info->PostContentType=CopyStr(Info->PostContentType, Value);
        else if (strcasecmp(Token, "content-length")==0) Info->PostContentLength=atoi(Value);
        else if (strcasecmp(Token, "user")==0) Info->UserName=CopyStr(Info->UserName, Value);
        else if (strcasecmp(Token, "useragent")==0) Info->UserAgent=CopyStr(Info->UserAgent, Value);
        else if (strcasecmp(Token, "user-agent")==0) Info->UserAgent=CopyStr(Info->UserAgent, Value);
        else SetVar(Info->CustomSendHeaders, Token, Value);
        ptr=GetNameValuePair(ptr,"\\S","=",&Token, &Value);
    }

		if (Info->PostContentLength > 0) Info->Doc=MCatStr(Info->Doc, "?", Args, NULL);
		else HTTPInfoPOSTSetContent(Info, "", Args, 0, 0);

    if (StrValid(Pass)) CredsStoreAdd(Info->Host, User, Pass);

		if (StrEnd(Info->Doc)) Info->Doc=CopyStr(Info->Doc, "/");

    DestroyString(User);
    DestroyString(Pass);
    DestroyString(Token);
    DestroyString(Value);
    DestroyString(Proto);
    DestroyString(Args);
    DestroyString(URL);
}


HTTPInfoStruct *HTTPInfoFromURL(const char *Method, const char *URL)
{
    HTTPInfoStruct *Info;
    Info=HTTPInfoCreate("HTTP/1.1","", 80, "", "", Method, "", "",0);
    HTTPInfoSetURL(Info, Method, URL);

    return(Info);
}


void HTTPParseCookie(HTTPInfoStruct *Info, const char *Str)
{
    const char *startptr, *endptr;
    char *Tempstr=NULL;
    ListNode *Curr;
    int len;

    startptr=Str;
    while (*startptr==' ') startptr++;

    endptr=strchr(startptr,';');
    if (endptr==NULL) endptr=startptr+strlen(Str);
//	if (( *endptr==';') || (*endptr=='\r') ) endptr--;

    Tempstr=CopyStrLen(Tempstr,startptr,endptr-startptr);

    Curr=ListGetNext(Cookies);
    endptr=strchr(Tempstr,'=');
    len=endptr-Tempstr;
    len--;

    while (Curr !=NULL)
    {
        if (strncmp(Curr->Item,Tempstr,len)==0)
        {
            Curr->Item=CopyStr(Curr->Item,Tempstr);
            DestroyString(Tempstr);
            return;
        }
        Curr=ListGetNext(Curr);
    }

    if (! Cookies) Cookies=ListCreate();
    ListAddItem(Cookies,(void *)CopyStr(NULL,Tempstr));

    DestroyString(Tempstr);
}



char *AppendCookies(char *InStr, ListNode *CookieList)
{
    ListNode *Curr;
    char *Tempstr=NULL;

    Tempstr=InStr;
    Curr=ListGetNext(CookieList);

    if (Curr)
    {
        Tempstr=CatStr(Tempstr,"Cookie: ");
        while ( Curr )
        {
            Tempstr=CatStr(Tempstr,(char *)Curr->Item);
            Curr=ListGetNext(Curr);
            if (Curr) Tempstr=CatStr(Tempstr, "; ");
        }
        Tempstr=CatStr(Tempstr,"\r\n");
    }

    return(Tempstr);
}


void HTTPClearCookies()
{
    ListClear(Cookies, Destroy);
}


static int HTTPHandleWWWAuthenticate(const char *Line, int *Type, char **Config)
{
    const char *ptr, *ptr2;
    char *Token=NULL, *Name=NULL, *Value=NULL;
    char *Realm=NULL, *QOP=NULL, *Nonce=NULL, *Opaque=NULL, *AuthType=NULL;

    ptr=Line;
    while (isspace(*ptr)) ptr++;
    ptr=GetToken(ptr," ",&Token,0);

    *Type &= ~(HTTP_AUTH_BASIC | HTTP_AUTH_DIGEST);
    if (strcasecmp(Token,"basic")==0) *Type |= HTTP_AUTH_BASIC;
    if (strcasecmp(Token,"digest")==0) *Type |= HTTP_AUTH_DIGEST;

    QOP=CopyStr(QOP,"");
    Realm=CopyStr(Realm,"");
    Nonce=CopyStr(Nonce,"");
    Opaque=CopyStr(Opaque,"");

    while (ptr)
    {
        ptr=GetToken(ptr,",",&Token,GETTOKEN_QUOTES);
        StripLeadingWhitespace(Token);
        StripTrailingWhitespace(Token);
        ptr2=GetToken(Token,"=",&Name,GETTOKEN_QUOTES);
        ptr2=GetToken(ptr2,"=",&Value,GETTOKEN_QUOTES);


        if (strcasecmp(Name,"realm")==0) Realm=CopyStr(Realm,Value);
        else if (strcasecmp(Name,"qop")==0)  QOP=CopyStr(QOP,Value);
        else if (strcasecmp(Name,"nonce")==0) Nonce=CopyStr(Nonce,Value);
        else if (strcasecmp(Name,"opaque")==0) Opaque=CopyStr(Opaque,Value);
    }

    if (*Type & HTTP_AUTH_DIGEST) *Config=MCopyStr(*Config, Realm,":", Nonce, ":", QOP, ":", Opaque, ":", NULL);
    else *Config=MCopyStr(*Config,Realm,":",NULL);

    DestroyString(Token);
    DestroyString(Name);
    DestroyString(Value);
    DestroyString(Realm);
    DestroyString(QOP);
    DestroyString(Nonce);
    DestroyString(Opaque);
    DestroyString(AuthType);

    return(*Type);
}

/* redirect headers can get complex. WE can have a 'full' header:
 *    http://myhost/whatever
 * or just the 'path' part
 *    /whatever
 * or even this
 *    //myhost/whatever
 */

static void HTTPParseLocationHeader(HTTPInfoStruct *Info, const char *Header)
{
   if (
         (strncasecmp(Header,"http:",5)==0) ||
         (strncasecmp(Header,"https:",6)==0)
      )
      {
         Info->RedirectPath=HTTPQuoteChars(Info->RedirectPath, Header, " ");
      }
	 		else if (strncmp(Header, "//",2)==0)
			{
        if (Info->Flags & HTTP_SSL) Info->RedirectPath=MCopyStr(Info->RedirectPath,"https:",Header,NULL);
         else Info->RedirectPath=MCopyStr(Info->RedirectPath,"http:",Header,NULL);
			}
      else
      {
         if (Info->Flags & HTTP_SSL) Info->RedirectPath=FormatStr(Info->RedirectPath,"https://%s:%d%s",Info->Host,Info->Port,Header);
         else Info->RedirectPath=FormatStr(Info->RedirectPath,"http://%s:%d%s",Info->Host,Info->Port,Header);
      }

}


static void HTTPParseHeader(STREAM *S, HTTPInfoStruct *Info, char *Header)
{
    char *Token=NULL, *Tempstr=NULL;
    const char *ptr;

    if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"HEADER: %s\n",Header);
    ptr=GetToken(Header,":",&Token,0);
    while (isspace(*ptr)) ptr++;

    Tempstr=MCopyStr(Tempstr,"HTTP:",Token,NULL);
    STREAMSetValue(S,Tempstr,ptr);
    ListAddNamedItem(Info->ServerHeaders,Token,CopyStr(NULL,ptr));

    if (StrValid(Token) && StrValid(ptr))
    {
        switch (*Token)
        {
        case 'C':
        case 'c':
            if (strcasecmp(Token,"Content-length")==0)
            {
                Info->ContentLength=atoi(ptr);
            }
            else if (strcasecmp(Token,"Content-type")==0)
            {
                Info->ContentType=CopyStr(Info->ContentType,ptr);
            }
            else if (strcasecmp(Token,"Connection")==0)
            {
                if (strcasecmp(ptr,"Close")==0) Info->Flags &= ~HTTP_KEEPALIVE;
            }
            else if ((strcasecmp(Token,"Content-Encoding")==0) )
            {
                if (! (Info->Flags & HTTP_NODECODE))
                {
                    if (
                        (strcasecmp(ptr,"gzip")==0) ||
                        (strcasecmp(ptr,"x-gzip")==0)
                    )
                    {
                        Info->Flags |= HTTP_GZIP;
                    }
                    if (
                        (strcasecmp(ptr,"deflate")==0)
                    )
                    {
                        Info->Flags |= HTTP_DEFLATE;
                    }
                }
            }
            break;

        case 'D':
        case 'd':
            if (strcasecmp(Token,"Date")==0) Info->Timestamp=CopyStr(Info->Timestamp,ptr);
            break;

        case 'L':
        case 'l':
            if (strcasecmp(Token,"Location")==0) HTTPParseLocationHeader(Info, ptr);
            break;

        case 'P':
        case 'p':
            if (strcasecmp(Token,"Proxy-Authenticate")==0) HTTPHandleWWWAuthenticate(ptr, &Info->AuthFlags, &Info->ProxyAuthorization);
            break;

        case 'W':
        case 'w':
            if (strcasecmp(Token,"WWW-Authenticate")==0) HTTPHandleWWWAuthenticate(ptr, &Info->AuthFlags, &Info->Authorization);
            break;

        case 'S':
        case 's':
            if (strcasecmp(Token,"Set-Cookie")==0) HTTPParseCookie(Info,ptr);
            else if (strcasecmp(Token,"Status")==0)
            {
                //'Status' overrides the response
                Info->ResponseCode=CopyStrLen(Info->ResponseCode,ptr,3);
                STREAMSetValue(S,"HTTP:ResponseCode",Info->ResponseCode);
                STREAMSetValue(S,"HTTP:ResponseReason",Tempstr);
            }
            break;


        case 'T':
        case 't':
            if (
                (strcasecmp(Token,"Transfer-Encoding")==0)
            )
            {
                if (! (Info->Flags & HTTP_NODECODE))
                {
                    if (strstr(ptr,"chunked"))
                    {
                        Info->Flags |= HTTP_CHUNKED;
                    }
                }
            }
            break;
        }
    }

    DestroyString(Token);
    DestroyString(Tempstr);
}


char *HTTPDigest(char *RetStr, const char *Method, const char *Logon, const char *Password, const char *Realm, const char *Doc, const char *Nonce)
{
    char *Tempstr=NULL, *HA1=NULL, *HA2=NULL, *ClientNonce=NULL, *Digest=NULL;
    int len1, len2;

    Tempstr=FormatStr(Tempstr,"%s:%s:%s",Logon,Realm,Password);
    len1=HashBytes(&HA1,"md5",Tempstr,StrLen(Tempstr),ENCODE_HEX);

    Tempstr=FormatStr(Tempstr,"%s:%s",Method,Doc);
    len2=HashBytes(&HA2,"md5",Tempstr,StrLen(Tempstr),ENCODE_HEX);

    Tempstr=MCopyStr(Tempstr,HA1,":",Nonce,":",HA2,NULL);
    len2=HashBytes(&Digest,"md5",Tempstr,StrLen(Tempstr),ENCODE_HEX);
    RetStr=MCopyStr(RetStr, "username=\"",Logon,"\", realm=\"",Realm,"\", nonce=\"",Nonce,"\", response=\"",Digest,"\", ","uri=\"",Doc,"\", algorithm=\"MD5\"", NULL);

    /* AVANCED DIGEST
    for (i=0; i < 10; i++)
    {
    	Tempstr=FormatStr(Tempstr,"%x",rand() % 255);
    	ClientNonce=CatStr(ClientNonce,Tempstr);
    }

    Tempstr=FormatStr(Tempstr,"%s:%s:%08d:%s:auth:%s",HA1,Nonce,AuthCounter,ClientNonce,HA2);
    HashBytes(&Digest,"md5",Tempstr,StrLen(Tempstr),0);
    Tempstr=FormatStr(Tempstr,"%s: Digest username=\"%s\",realm=\"%s\",nonce=\"%s\",uri=\"%s\",qop=auth,nc=%08d,cnonce=\"%s\",response=\"%s\"\r\n",AuthHeader,Logon,Realm,Nonce,Doc,AuthCounter,ClientNonce,Digest);
    SendStr=CatStr(SendStr,Tempstr);
    */

    DestroyString(Tempstr);
    DestroyString(HA1);
    DestroyString(HA2);
    DestroyString(Digest);
    DestroyString(ClientNonce);

    return(RetStr);
}


static char *HTTPHeadersAppendAuth(char *RetStr, char *AuthHeader, HTTPInfoStruct *Info, char *AuthInfo)
{
    char *SendStr=NULL, *Tempstr=NULL, *Realm=NULL, *Nonce=NULL;
    const char *ptr;
    char *Logon=NULL;
    const char *p_Password=NULL;
    int len, passlen;

    if (StrEnd(AuthInfo)) return(RetStr);
    if (Info->AuthFlags & (HTTP_AUTH_TOKEN | HTTP_AUTH_OAUTH)) return(MCatStr(RetStr, AuthHeader,": ",AuthInfo,"\r\n",NULL));

    SendStr=CatStr(RetStr,"");

    ptr=GetToken(AuthInfo,":",&Realm,0);
    ptr=GetToken(ptr,":",&Nonce,0);

    passlen=CredsStoreLookup(Realm, Info->UserName, &p_Password);

    if (! passlen) passlen=CredsStoreLookup(Info->Host, Info->UserName, &p_Password);

		if (passlen)
		{
    if (Info->AuthFlags & HTTP_AUTH_DIGEST)
    {
        Tempstr=HTTPDigest(Tempstr, Info->Method, Info->UserName, p_Password, Realm, Info->Doc, Nonce);
        SendStr=MCatStr(SendStr,AuthHeader,": Digest ", Tempstr, "\r\n",NULL);
    }
    else
    {
        Tempstr=MCopyStr(Tempstr,Info->UserName,":",NULL);
        //Beware! Password will not be null terminated
        Tempstr=CatStrLen(Tempstr,p_Password,passlen);
        len=strlen(Tempstr);

        //We should now have Logon:Password
        Nonce=SetStrLen(Nonce,len * 2);

        to64frombits((unsigned char *) Nonce, (unsigned char *) Tempstr, len);
        SendStr=MCatStr(SendStr,AuthHeader,": Basic ",Nonce,"\r\n",NULL);

        //wipe Tempstr, because it held password for a while
        xmemset(Tempstr,0,len);
    }

    Info->AuthFlags |= HTTP_AUTH_SENT;
		}

    DestroyString(Tempstr);
    DestroyString(Logon);
    DestroyString(Realm);
    DestroyString(Nonce);

    return(SendStr);
}


void HTTPSendHeaders(STREAM *S, HTTPInfoStruct *Info)
{
    char *SendStr=NULL, *Tempstr=NULL;
    const char *ptr;
    ListNode *Curr;

    STREAMClearDataProcessors(S);
    SendStr=CopyStr(SendStr,Info->Method);
    SendStr=CatStr(SendStr," ");

    if (Info->Flags & HTTP_PROXY) Tempstr=HTTPInfoToURL(Tempstr, Info);
    else Tempstr=HTTPQuoteChars(Tempstr,Info->Doc," ");
    SendStr=CatStr(SendStr,Tempstr);

    if (Info->Flags & HTTP_VER1_0) SendStr=CatStr(SendStr," HTTP/1.0\r\n");
    else SendStr=MCatStr(SendStr," ",Info->Protocol,"\r\n","Host: ",Info->Host,"\r\n",NULL);

    if (StrValid(Info->PostContentType))
    {
        Tempstr=FormatStr(Tempstr,"Content-type: %s\r\n",Info->PostContentType);
        SendStr=CatStr(SendStr,Tempstr);
    }

		//probably need to find some other way of detecting need for sending ContentLength other than whitelisting methods
    if ((strcasecmp(Info->Method,"POST")==0) || (strcasecmp(Info->Method,"PUT")==0) || (strcasecmp(Info->Method,"PATCH")==0))
    {
        Tempstr=FormatStr(Tempstr,"Content-Length: %d\r\n",Info->PostContentLength);
        SendStr=CatStr(SendStr,Tempstr);
    }

    if (StrValid(Info->Destination))
    {
        Tempstr=FormatStr(Tempstr,"Destination: %s\r\n",Info->Destination);
        SendStr=CatStr(SendStr,Tempstr);
    }

    /* If we have authorisation details then send them */
    if (Info->AuthFlags & HTTP_AUTH_OAUTH)
    {
        Info->Authorization=MCopyStr(Info->Authorization, "Bearer ", OAuthLookup(Info->Credentials, FALSE), NULL);
    }
    if (StrValid(Info->Authorization)) SendStr=HTTPHeadersAppendAuth(SendStr, "Authorization", Info, Info->Authorization);
    else if (Info->AuthFlags & HTTP_AUTH_HOST) SendStr=HTTPHeadersAppendAuth(SendStr, "Authorization", Info, Info->Host);
    if (Info->ProxyAuthorization) SendStr=HTTPHeadersAppendAuth(SendStr, "Proxy-Authorization", Info, Info->ProxyAuthorization);

    if (Info->Flags & HTTP_NOCACHE) SendStr=CatStr(SendStr,"Pragma: no-cache\r\nCache-control: no-cache\r\n");


    if (Info->Depth > 0)
    {
        Tempstr=FormatStr(Tempstr,"Depth: %d\r\n",Info->Depth);
        SendStr=CatStr(SendStr,Tempstr);
    }

    /*
    if ((PathData->Options.Restart) && (PathData->offset >0))
    {
    snprintf(Buffer,sizeof(Buffer),"Range: bytes=%d-\r\n",PathData->offset);
    SendStr=CatStr(SendStr,Buffer);

    }
    */

    if (Info->IfModifiedSince > 0)
    {
        Tempstr=CopyStr(Tempstr,GetDateStrFromSecs("%a, %d %b %Y %H:%M:%S GMT",Info->IfModifiedSince,NULL));
        SendStr=MCatStr(SendStr,"If-Modified-Since: ",Tempstr, "\r\n",NULL);
    }

    if (
        (strcasecmp(Info->Method,"DELETE") !=0) &&
        (strcasecmp(Info->Method,"HEAD") !=0) &&
        (strcasecmp(Info->Method,"PUT") !=0)
    )
    {

        Tempstr=CopyStr(Tempstr,"");

        if (! (Info->Flags & HTTP_NOCOMPRESS))
        {
            if (DataProcessorAvailable("compress","gzip")) Tempstr=CatStr(Tempstr,"gzip");
            if (DataProcessorAvailable("compress","zlib"))
            {
                if (StrValid(Tempstr)) Tempstr=CatStr(Tempstr,", deflate");
                else Tempstr=CatStr(Tempstr,"deflate");
            }
        }

        if (StrValid(Tempstr)) SendStr=MCatStr(SendStr,"Accept-Encoding: ",Tempstr,"\r\n",NULL);
    }

    if (Info->Flags & HTTP_KEEPALIVE)
    {
        //if (Info->Flags & HTTP_VER1_0)
        SendStr=CatStr(SendStr,"Connection: Keep-Alive\r\n");
        //SendStr=CatStr(SendStr,"Content-Length: 0\r\n");
    }
    else
    {
        SendStr=CatStr(SendStr,"Connection: close\r\n");
    }

    SendStr=MCatStr(SendStr,"User-Agent: ",Info->UserAgent, "\r\n",NULL);

    Curr=ListGetNext(Info->CustomSendHeaders);
    while (Curr)
    {
        SendStr=MCatStr(SendStr,Curr->Tag, ": ", (char *)  Curr->Item, "\r\n",NULL);
        Curr=ListGetNext(Curr);
    }

    if (! (Info->Flags & HTTP_NOCOOKIES))
    {
        SendStr=AppendCookies(SendStr,Cookies);
    }

    SendStr=CatStr(SendStr,"\r\n");

    Info->State |= HTTP_HEADERS_SENT;
    if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"HTTPSEND: ------\n%s------\n\n",SendStr);
    STREAMWriteLine(SendStr,S);

//this flush pushes the data to the server, which is needed for most HTTP methods
//other than PUT or POST because we'll send no further data. It has a useful
//side-effect that the STREAM buffers will be wiped of any lingering data, which
//is important as we've just sent headers that could include passwords or other
//sensitive data
    STREAMFlush(S);

//wipe send str, because headers can contain sensitive data like passwords
    xmemset(SendStr,0,StrLen(SendStr));

    DestroyString(Tempstr);
    DestroyString(SendStr);
}




void HTTPReadHeaders(STREAM *S, HTTPInfoStruct *Info)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    ListClear(Info->ServerHeaders,Destroy);
    Info->ContentLength=0;

//Not needed
//Info->RedirectPath=CopyStr(Info->RedirectPath,"");
    Info->Flags &= ~(HTTP_CHUNKED | HTTP_GZIP | HTTP_DEFLATE);
    Tempstr=STREAMReadLine(Tempstr,S);
    if (Tempstr)
    {
        if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"RESPONSE: %s\n",Tempstr);
        //Token will be protocol (HTTP/1.0 or ICY or whatever)
        ptr=GetToken(Tempstr,"\\S",&Token,0);
        Info->ResponseCode=CopyStrLen(Info->ResponseCode,ptr,3);
        STREAMSetValue(S,"HTTP:ResponseCode",Info->ResponseCode);
        STREAMSetValue(S,"HTTP:ResponseReason",Tempstr);
        Tempstr=STREAMReadLine(Tempstr,S);
    }

    while (Tempstr)
    {
        StripTrailingWhitespace(Tempstr);
        if (StrEnd(Tempstr)) break;
        HTTPParseHeader(S, Info,Tempstr);
        Tempstr=STREAMReadLine(Tempstr,S);
    }

    S->BytesRead=0;
    ptr=STREAMGetValue(S, "HTTP:Content-Length");
    if (ptr) S->Size=(atoi(ptr));

    DestroyString(Tempstr);
    DestroyString(Token);
}



int HTTPProcessResponse(HTTPInfoStruct *HTTPInfo)
{
    int result=HTTP_ERROR;
    char *Proto=NULL, *Tempstr=NULL;
    int RCode;

    if (HTTPInfo->ResponseCode)
    {
        RCode=atoi(HTTPInfo->ResponseCode);
        switch (RCode)
        {
        case 304:
            result=HTTP_NOTMODIFIED;
            break;

        case 200:
        case 201:
        case 202:
        case 203:
        case 204:
        case 205:
        case 206:
        case 207:
        case 400:
            result=HTTP_OKAY;
            break;

        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            if (HTTPInfo->PreviousRedirect && (strcmp(HTTPInfo->RedirectPath,HTTPInfo->PreviousRedirect)==0)) result=HTTP_CIRCULAR_REDIRECTS;
            else
            {
                if (HTTPInfo->Flags & HTTP_DEBUG) fprintf(stderr,"HTTP: Redirected to %s\n",HTTPInfo->RedirectPath);

                //As redirect check has been done, we can copy redirect path to previous
                HTTPInfo->PreviousRedirect=CopyStr(HTTPInfo->PreviousRedirect,HTTPInfo->RedirectPath);
                ParseURL(HTTPInfo->RedirectPath, &Proto, &HTTPInfo->Host, &Tempstr,NULL, NULL,&HTTPInfo->Doc,NULL);
                HTTPInfo->Port=atoi(Tempstr);

                //if HTTP_SSL_REWRITE is set, then all redirects get forced to https
                if (HTTPInfo->Flags & HTTP_SSL_REWRITE) Proto=CopyStr(Proto,"https");
                if (strcmp(Proto,"https")==0) HTTPInfo->Flags |= HTTP_SSL;
                else HTTPInfo->Flags &= ~HTTP_SSL;

                //303 Redirects must be get!
                if (RCode==303)
                {
                    HTTPInfo->Method=CopyStr(HTTPInfo->Method,"GET");
                    HTTPInfo->PostData=CopyStr(HTTPInfo->PostData,"");
                    HTTPInfo->PostContentType=CopyStr(HTTPInfo->PostContentType,"");
                    HTTPInfo->PostContentLength=0;
                }

                if (! (HTTPInfo->Flags & HTTP_NOREDIRECT)) result=HTTP_REDIRECT;
                else result=HTTP_OKAY;
            }
            break;

        //401 Means authenticate, so it's not a pure error
        case 401:
            //407 Means authenticate with a proxy
            result=HTTP_AUTH_BASIC;
            break;

        case 407:
            result=HTTP_AUTH_PROXY;
            break;

        default:
            result=HTTP_NOTFOUND;
            break;
        }
    }

    if (result == HTTP_NOTFOUND)
    {
        RaiseError(0, "HTTP", STREAMGetValue(HTTPInfo->S, "HTTP:ResponseReason"));
    }

    DestroyString(Proto);
    DestroyString(Tempstr);

    return(result);
}



STREAM *HTTPSetupConnection(HTTPInfoStruct *Info, int ForceHTTPS)
{
    char *Proto=NULL, *Host=NULL, *Tempstr=NULL;
    const char *ptr;
    int Port=0, Flags=0;
    STREAM *S;


		//proto in here will not be http/https but tcp/ssl/tls
    Proto=CopyStr(Proto,"tcp");
    if (Info->Flags & HTTP_PROXY)
    {
        ParseURL(Info->Proxy, &Proto, &Host, &Tempstr, NULL, NULL, NULL,NULL);
        Port=atoi(Tempstr);

        if (ForceHTTPS) Proto=CopyStr(Proto,"ssl");
    }
    else
    {
        Host=CopyStr(Host,Info->Host);
        Port=Info->Port;

        if (ForceHTTPS || (Info->Flags & HTTP_SSL)) Proto=CopyStr(Proto,"ssl");

        if (Port==0)
        {
            if (strcmp(Proto,"ssl")==0) Port=443;
            else Port=80;
        }
    }

    if (StrValid(Info->ConnectionChain)) Tempstr=FormatStr(Tempstr,"%s|%s:%s:%d/",Info->ConnectionChain,Proto,Host,Port);
    else Tempstr=FormatStr(Tempstr,"%s:%s:%d/",Proto,Host,Port);

		S=STREAMOpen(Tempstr, "");
    if (S)
    {
        S->Type=STREAM_TYPE_HTTP;
        HTTPSendHeaders(S,Info);
    }
    else
    {
        RaiseError(ERRFLAG_ERRNO, "http", "failed to connect to %s:%d",Host,Port);
        STREAMClose(S);
        S=NULL;
    }

    Info->S=S;

    DestroyString(Proto);
    DestroyString(Tempstr);
    DestroyString(Host);

    return(S);
}



STREAM *HTTPConnect(HTTPInfoStruct *Info)
{
    STREAM *S=NULL;

    if (
        (g_HTTPFlags & HTTP_REQ_HTTPS) ||
        (g_HTTPFlags & HTTP_TRY_HTTPS)
    )
    {
        S=HTTPSetupConnection(Info, TRUE);
        if (g_HTTPFlags & HTTP_REQ_HTTPS) return(S);
    }

    if (!S) S=HTTPSetupConnection(Info, FALSE);

    if (S)
    {
        S->Path=FormatStr(S->Path,"%s://%s:%d/%s",Info->Protocol,Info->Host,Info->Port,Info->Doc);
        STREAMSetItem(S, "HTTP:InfoStruct", Info);
    }
    return(S);
}


STREAM *HTTPTransact(HTTPInfoStruct *Info)
{
    int result=HTTP_NOCONNECT;

    while (1)
    {
        if (! Info->S) Info->S=HTTPConnect(Info);
        else if (! (Info->State & HTTP_HEADERS_SENT)) HTTPSendHeaders(Info->S,Info);

        if (Info->S && STREAMIsConnected(Info->S))
        {
            Info->ResponseCode=CopyStr(Info->ResponseCode,"");

            if (! (Info->State & HTTP_CLIENTDATA_SENT))
            {
                //Set this even if no client data to send, so we no we've been
                //through here once
                Info->State |= HTTP_CLIENTDATA_SENT;

                if (StrValid(Info->PostData))
                {
                    STREAMWriteLine(Info->PostData,Info->S);
                    if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"\n%s\n",Info->PostData);
                }
                else
                {
                    if (strcasecmp(Info->Method,"POST")==0) break;
                    if (strcasecmp(Info->Method,"PUT")==0) break;
                    if (strcasecmp(Info->Method,"PATCH")==0) break;
                }
            }


            //Must clear this once the headers and clientdata sent
            Info->State=0;

            HTTPReadHeaders(Info->S,Info);
            result=HTTPProcessResponse(Info);
            STREAMSetValue(Info->S,"HTTP:URL",Info->Doc);

            if (Info->Flags & HTTP_CHUNKED) HTTPAddChunkedProcessor(Info->S);

            if (Info->Flags & HTTP_GZIP)
            {
                STREAMAddStandardDataProcessor(Info->S,"uncompress","gzip","");
            }
            else if (Info->Flags & HTTP_DEFLATE)
            {
                STREAMAddStandardDataProcessor(Info->S,"uncompress","zlib","");
            }
            if (Info->Flags & (HTTP_CHUNKED | HTTP_GZIP | HTTP_DEFLATE)) STREAMReBuildDataProcessors(Info->S);

            //tranaction succeeded, stop trying, break out of loop
            if (result == HTTP_OKAY) break;
            //any of these mean the tranaction failed. give up. break out of loop
            if (result == HTTP_NOTFOUND) break;
            if (result == HTTP_NOTMODIFIED) break;
            if (result == HTTP_ERROR) break;
            if (result == HTTP_CIRCULAR_REDIRECTS) break;


            //here this flag just means the server asked us to authenticate
            if (result == HTTP_AUTH_BASIC)
            {
                //if we're using OAUTH then try doing a refresh. Set a flag that means we'll give up if we fail again
                if (Info->AuthFlags & HTTP_AUTH_OAUTH)
                {
                    if (Info->AuthFlags & HTTP_AUTH_RETURN) break;
                    Info->Authorization=MCopyStr(Info->Authorization, "Bearer ", OAuthLookup(Info->Credentials, TRUE), NULL);
                    Info->AuthFlags |= HTTP_AUTH_RETURN;
                }
                //for normal authentication, if we've sent the authentication, or if we have no auth details, then give up
                else if (
                    (Info->AuthFlags & HTTP_AUTH_SENT) ||
                    (Info->AuthFlags & HTTP_AUTH_RETURN) ||
                    (StrEnd(Info->Authorization))
                ) break;
            }

            //if we got asked for proxy authentication bu have no auth details, then give up
            if (
                (result == HTTP_AUTH_PROXY) &&
                (StrEnd(Info->ProxyAuthorization))
            ) break;


            //must do this or STREAMClose destroys Info object!
            STREAMSetItem(Info->S, "HTTP:InfoStruct", NULL);
            STREAMClose(Info->S);
            Info->S=NULL;
        }
        else break;
    }


    return(Info->S);
}



STREAM *HTTPMethod(const char *Method, const char *URL, const char *ContentType, const char *ContentData, int ContentLength)
{
    HTTPInfoStruct *Info;
    STREAM *S;

    Info=HTTPInfoFromURL(Method, URL);
    HTTPInfoPOSTSetContent(Info, ContentType, ContentData, ContentLength, HTTP_POSTARGS);
    S=HTTPTransact(Info);
    return(S);
}


STREAM *HTTPGet(const char *URL)
{
    return(HTTPMethod("GET", URL, "","",0));
}


STREAM *HTTPPost(const char *URL, const char *ContentType, const char *Content)
{
    return(HTTPMethod("POST", URL, ContentType, Content, StrLen(Content)));
}


STREAM *HTTPWithConfig(const char *URL, const char *Config)
{
    char *Token=NULL;
    const char *ptr, *cptr, *p_Method="GET";
    STREAM *S;
    int Flags=0;


    ptr=GetToken(Config,"\\S",&Token, 0);

//if the first arg contains '=' then they've forgotten to supply a method specifier, so go with 'GET' and
//treat all the args as name=value pairs that are dealt with in HTTPInfoSetURL
    if (strchr(Token,'=')) ptr=Config;
    else
    {
        for (cptr=Token; *cptr !='\0'; cptr++)
        {
						switch(*cptr)
						{
            case 'w': p_Method="POST"; break;
            case 'W': p_Method="PUT"; break;
            case 'P': p_Method="PATCH"; break;
            case 'D': p_Method="DELETE"; break;
            case 'H': p_Method="HEAD"; break;
						}
        }
    }

    Token=MCopyStr(Token, URL, " ", ptr, NULL);
    S=HTTPMethod(p_Method, Token, "","", 0);

    DestroyString(Token);

    return(S);
}




int HTTPDownload(char *URL, STREAM *S)
{
    STREAM *Con;
		const char *ptr;

    Con=HTTPGet(URL);
    if (! Con) return(-1);

		ptr=STREAMGetValue(Con, "HTTP:ResponseCode");
		if ((! ptr) || (*ptr !='2'))
		{
			STREAMClose(Con);
			return(-1);
		}
    return(STREAMSendFile(Con, S, 0, SENDFILE_LOOP));
}


void HTTPSetFlags(int Flags)
{
    g_HTTPFlags=Flags;
}

int HTTPGetFlags()
{
    return(g_HTTPFlags);
}


