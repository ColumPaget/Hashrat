#include "http.h"
#include "DataProcessing.h"
#include "ConnectionChain.h"
#include "Hash.h"
#include "ParseURL.h"
#include "Time.h"
#include "base64.h"

const char *HTTP_AUTH_BY_TOKEN="AuthTokenType";
ListNode *Cookies=NULL;
ListNode *HTTPVars=NULL;
int g_Flags=0;




void HTTPInfoDestroy(void *p_Info)
{
HTTPInfoStruct *Info;

if (! p_Info) return;
Info=(HTTPInfoStruct *) p_Info;
DestroyString(Info->Host);
DestroyString(Info->Method);
DestroyString(Info->Doc);
DestroyString(Info->Destination);
DestroyString(Info->ResponseCode);
DestroyString(Info->PreviousRedirect);
DestroyString(Info->RedirectPath);
DestroyString(Info->ContentType);
DestroyString(Info->Timestamp);
DestroyString(Info->PostData);
DestroyString(Info->PostContentType);
DestroyString(Info->Proxy);
DestroyString(Info->Authorization);
DestroyString(Info->ProxyAuthorization);

ListDestroy(Info->ServerHeaders,DestroyString);
ListDestroy(Info->CustomSendHeaders,DestroyString);
free(Info);
}



void HTTPSetVar(char *Name, char *Var)
{
	if (! HTTPVars) HTTPVars=ListCreate();
	SetVar(HTTPVars,Name,Var);
}



//These functions relate to adding a 'Data processor' to the stream that
//will decode chunked HTTP transfers

typedef struct
{
char *Buffer;
int ChunkSize;
int BuffLen;
} THTTPChunk;

int HTTPChunkedInit(TProcessingModule *Mod, const char *Args)
{

Mod->Data=(THTTPChunk *) calloc(1, sizeof(THTTPChunk));

return(TRUE);
}

int HTTPChunkedRead(TProcessingModule *Mod, const char *InBuff, int InLen, char **OutBuff, int *OutLen, int Flush)
{
int len=0, val=0;
THTTPChunk *Chunk;
char *ptr, *vptr;

Chunk=(THTTPChunk *) Mod->Data;
if (InLen > 0)
{
len=Chunk->BuffLen+InLen;
Chunk->Buffer=SetStrLen(Chunk->Buffer,len);
memcpy(Chunk->Buffer+Chunk->BuffLen,InBuff,InLen);
Chunk->BuffLen=len;
Chunk->Buffer[len]='\0';
}
else len=Chunk->BuffLen;
ptr=Chunk->Buffer;

if (Chunk->ChunkSize==0)
{
	//if chunksize == 0 then read the size of the next chunk

	//if there's nothing in our buffer, and nothing being added, then
	//we've already finished!
  if ((Chunk->BuffLen==0) && (InLen==0)) return(STREAM_CLOSED);

	vptr=ptr;
	//skip past any leading '\r' or '\n'
	if (*vptr=='\r') vptr++;
	if (*vptr=='\n') vptr++;

	ptr=strchr(vptr,'\n');


	//sometimes people seem to miss off the final '\n', so if we get told there's no more data
	//we should use a '\r' if we've got one
	if ((! ptr) && (InLen==0)) ptr=strchr(vptr,'\r');
	if (ptr)
	{
	  *ptr='\0';
		ptr++;
	}
	else return(0);
	
	Chunk->ChunkSize=strtol(vptr,NULL,16);
	Chunk->BuffLen=Chunk->Buffer+len-ptr;


	if (Chunk->BuffLen > 0)	memmove(Chunk->Buffer,ptr,Chunk->BuffLen);
	//in case it went negative in the above calcuation
	else Chunk->BuffLen=0;


	//if we got chunksize of 0 then we're done, return STREAM_CLOSED
	if (Chunk->ChunkSize==0) return(STREAM_CLOSED);
}
else if (len >= Chunk->ChunkSize)
{
	val=Chunk->ChunkSize;

	//We should really grow OutBuff to take all the data
	//but for the sake of simplicity we'll just use the space
	//supplied
	if (val > *OutLen) val=*OutLen;
	memcpy(*OutBuff,Chunk->Buffer,val);

	ptr=Chunk->Buffer+val;
	Chunk->BuffLen-=val;
	Chunk->ChunkSize-=val;
	memmove(Chunk->Buffer, ptr, Chunk->BuffLen);
}

if (Chunk->ChunkSize < 0) Chunk->ChunkSize=0;

return(val);
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



char *HTTPUnQuote(char *RetBuff, char *Str)
{
char *RetStr=NULL, *Token=NULL, *ptr;
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

DestroyString(Token);
return(RetStr);
}


char *HTTPQuoteChars(char *RetBuff, char *Str, char *CharList)
{
char *RetStr=NULL, *Token=NULL, *ptr;
int olen=0, ilen;

RetStr=CopyStr(RetStr,"");
ilen=StrLen(Str);

for (ptr=Str; ptr < (Str+ilen); ptr++)
{
if (strchr(CharList,*ptr))
{
		Token=FormatStr(Token,"%%%02X",*ptr); 
		RetStr=CatStr(RetStr,Token);
		olen+=StrLen(Token);
}
else
{
		 RetStr=AddCharToBuffer(RetStr,olen,*ptr); 
		 olen++;
}
}


RetStr[olen]='\0';
DestroyString(Token);
return(RetStr);
}



char *HTTPQuote(char *RetBuff, char *Str)
{
char *RetStr=NULL, *Token=NULL, *ptr;
int olen=0, ilen;

RetStr=CopyStr(RetStr,"");
ilen=StrLen(Str);

for (ptr=Str; ptr < (Str+ilen); ptr++)
{
switch (*ptr)
{
		case ' ':
			RetStr=AddCharToStr(RetStr,'+');
		break;

		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case '\t':
		case '?':
		case '&':
		case '!':
		case ',':
		case '+':
		case '\'':
		case ':':
		case ';':
		case '/':
		case '\r':
		case '\n':
		Token=FormatStr(Token,"%%%02X",*ptr); 
		RetStr=CatStr(RetStr,Token);
		olen+=StrLen(Token);
		break;

		default:
	//	 RetStr=AddCharToBuffer(RetStr,olen,*ptr); 
		 RetStr=AddCharToStr(RetStr,*ptr); 
		 olen++;
		break;
}

}

DestroyString(Token);
return(RetStr);
}


void HTTPInfoSetAuth(HTTPInfoStruct *Info, char *Logon, char *Password, int Type)
{
char **p_Auth;

if (Type & HTTP_AUTH_PROXY) p_Auth=&Info->ProxyAuthorization;
else p_Auth=&Info->Authorization;

*p_Auth=MCopyStr(*p_Auth,":",Logon,":",Password, NULL);

}


void HTTPInfoSetValues(HTTPInfoStruct *Info, char *Host, int Port, char *Logon, char *Password, char *Method, char *Doc, char *ContentType, int ContentLength)
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

if (StrLen(Logon) || StrLen(Password)) HTTPInfoSetAuth(Info, Logon, Password, HTTP_AUTH_BASIC);
}




HTTPInfoStruct *HTTPInfoCreate(char *Host, int Port, char *Logon, char *Password, char *Method, char *Doc, char *ContentType, int ContentLength)
{
HTTPInfoStruct *Info;
char *ptr;

Info=(HTTPInfoStruct *) calloc(1,sizeof(HTTPInfoStruct));
HTTPInfoSetValues(Info, Host, Port, Logon, Password, Method, Doc, ContentType, ContentLength);

Info->ServerHeaders=ListCreate();
Info->CustomSendHeaders=ListCreate();
//SetVar(Info->CustomSendHeaders,"Accept","*/*");

if (g_Flags) Info->Flags=g_Flags;

ptr=LibUsefulGetValue("HTTP:Proxy");
if (StrLen(ptr)) 
{
	Info->Proxy=CopyStr(Info->Proxy,ptr);
	strlwr(Info->Proxy);
	if (strncmp(Info->Proxy,"http:",5)==0) Info->Flags |= HTTP_PROXY;
	else if (strncmp(Info->Proxy,"https:",6)==0) Info->Flags |= HTTP_PROXY;
	else Info->Flags=HTTP_TUNNEL;
}

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


HTTPInfoStruct *HTTPInfoFromURL(char *Method, char *URL)
{
HTTPInfoStruct *Info;
char *Proto=NULL, *User=NULL, *Pass=NULL, *Token=NULL;


Info=HTTPInfoCreate("", 80, "", "", Method, "", "",0);
if (strcasecmp(Method,"POST")==0) ParseURL(URL, &Proto, &Info->Host, &Token, &User, &Pass,&Info->Doc,&Info->PostData);
else ParseURL(URL, &Proto, &Info->Host, &Token, &User, &Pass,&Info->Doc, NULL);
if (StrLen(Token)) Info->Port=atoi(Token);

if (StrLen(User) || StrLen(Pass)) HTTPInfoSetAuth(Info,User, Pass, HTTP_AUTH_BASIC);

if (StrLen(Proto) && (strcmp(Proto,"https")==0)) Info->Flags |= HTTP_SSL;

if (StrLen(Info->PostData))
{
	Info->PostContentType=CopyStr(Info->PostContentType,"application/x-www-form-urlencoded");
	Info->PostContentLength=StrLen(Info->PostData);
}

DestroyString(User);
DestroyString(Pass);
DestroyString(Token);
DestroyString(Proto);

return(Info);
}




void HTTPParseCookie(HTTPInfoStruct *Info, char *Str)
{
	char *startptr, *endptr;
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


void HTTPHandleWWWAuthenticate(HTTPInfoStruct *Info, char *Line)
{
char *ptr, *ptr2, *Token=NULL, *Name=NULL, *Value=NULL;
const char *AuthTypeStrings[]={"Basic","Digest",NULL};
char *Realm=NULL, *QOP=NULL, *Nonce=NULL, *Opaque=NULL;
typedef enum {AUTH_BASIC, AUTH_DIGEST} TAuthTypes;
int result;

ptr=Line;
while (isspace(*ptr)) ptr++;
ptr=GetToken(ptr," ",&Token,0);

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
if (strcasecmp(Name,"qop")==0)  QOP=CopyStr(QOP,Value);
if (strcasecmp(Name,"nonce")==0) Nonce=CopyStr(Nonce,Value);
if (strcasecmp(Name,"opaque")==0) Opaque=CopyStr(Opaque,Value);
}

result=MatchTokenFromList(Token,AuthTypeStrings,0);
switch (result)
{
case AUTH_BASIC: Info->AuthFlags |= HTTP_AUTH_BASIC; break;
//case AUTH_DIGEST: Info->Authorization=MCopyStr(Info->Authorization,"digest:",Realm,":", QOP, ":", Nonce, ":", Opaque, ":", NULL); break;
}

DestroyString(Token);
DestroyString(Value);
DestroyString(Name);
DestroyString(Realm);
DestroyString(QOP);
DestroyString(Nonce);
DestroyString(Opaque);
}




void HTTPParseHeader(STREAM *S, HTTPInfoStruct *Info, char *Header)
{
char *Token=NULL, *Tempstr=NULL;
char *ptr;

if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"HEADER: %s\n",Header);
ptr=GetToken(Header,":",&Token,0);
while (isspace(*ptr)) ptr++;

Tempstr=MCopyStr(Tempstr,"HTTP:",Token,NULL);
STREAMSetValue(S,Tempstr,ptr);
ListAddNamedItem(Info->ServerHeaders,Token,CopyStr(NULL,ptr));

if (StrLen(Token) && StrLen(ptr))
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
					strlwr(ptr);


					if (
							(strcmp(ptr,"gzip")==0) ||
							(strcmp(ptr,"x-gzip")==0)
						)
					{
						Info->Flags |= HTTP_GZIP;
					}
					if (
							(strcmp(ptr,"deflate")==0) 
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
			if (strcasecmp(Token,"Location")==0)
			{
				if (
							(strncasecmp(ptr,"http:",5)==0) ||
							(strncasecmp(ptr,"https:",6)==0) 
						)
				{
					Info->RedirectPath=CopyStr(Info->RedirectPath,ptr);
				}
				else Info->RedirectPath=FormatStr(Info->RedirectPath,"http://%s:%d%s",Info->Host,Info->Port,ptr);
			}
			break;

			case 'W':
			case 'w':
				if (strcasecmp(Token,"WWW-Authenticate")==0) HTTPHandleWWWAuthenticate(Info,ptr);
			break;

			case 'S':
			case 's':
				if (strcasecmp(Token,"Set-Cookie")==0) HTTPParseCookie(Info,ptr);
				else if (strcasecmp(Token,"Status")==0) 
				{
					//'Status' overrides the response
			    Info->ResponseCode=CopyStrLen(Info->ResponseCode,ptr,3);
  			  STREAMSetValue(S,"HTTP:ResponseCode",Info->ResponseCode);
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
				strlwr(ptr);
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



char *HTTPHeadersAppendAuth(char *RetStr, char *AuthHeader, HTTPInfoStruct *Info, char *AuthInfo)
{
char *SendStr=NULL, *Tempstr=NULL, *ptr;
char *HA1=NULL, *HA2=NULL, *ClientNonce=NULL, *Digest=NULL;

	if (! StrLen(AuthInfo)) return(RetStr);

	SendStr=CatStr(RetStr,"");

	//Authentication by an opaque authentication token that is handled 
	//elsewhere, and is set as the 'Password'
	if (Info->AuthFlags & HTTP_AUTH_TOKEN)
	{
    SendStr=MCatStr(SendStr,AuthHeader,": ",AuthInfo,"\r\n",NULL);
    Info->AuthFlags |= HTTP_AUTH_SENT;
	}
  else if (Info->AuthFlags & HTTP_AUTH_DIGEST)
  {
		/*
    AuthCounter++;
    Tempstr=FormatStr(Tempstr,"%s:%s:%s",AuthInfo->Logon,AuthInfo->AuthRealm,AuthInfo->Password);
    HashBytes(&HA1,"md5",Tempstr,StrLen(Tempstr),0);
    Tempstr=FormatStr(Tempstr,"%s:%s",Info->Method,Info->Doc);
    HashBytes(&HA2,"md5",Tempstr,StrLen(Tempstr),0);

    for (i=0; i < 10; i++)
    {
			Tempstr=FormatStr(Tempstr,"%x",rand() % 255);
			ClientNonce=CatStr(ClientNonce,Tempstr);
    }

    Tempstr=FormatStr(Tempstr,"%s:%s:%08d:%s:auth:%s",HA1,AuthInfo->AuthNonce,AuthCounter,ClientNonce,HA2);
    HashBytes(&Digest,"md5",Tempstr,StrLen(Tempstr),0);
    Tempstr=FormatStr(Tempstr,"%s: Digest username=\"%s\",realm=\"%s\",nonce=\"%s\",uri=\"%s\",qop=auth,nc=%08d,cnonce=\"%s\",response=\"%s\"\r\n",AuthHeader,AuthInfo->Logon,AuthInfo->AuthRealm,AuthInfo->AuthNonce,Info->Doc,AuthCounter,ClientNonce,Digest);
    SendStr=CatStr(SendStr,Tempstr);
    Info->AuthFlags |= HTTP_AUTH_SENT;
		*/
  }
	else
  {
		//Realm
		ptr=GetToken(AuthInfo,":",&Tempstr,0);

		//We should now have Logon:Password
    Digest=SetStrLen(Digest,StrLen(ptr) *2);

    to64frombits((unsigned char *) Digest, (unsigned char *) ptr,strlen(ptr));
    SendStr=MCatStr(SendStr,AuthHeader,": Basic ",Digest,"\r\n",NULL);
    Info->AuthFlags |= HTTP_AUTH_SENT;
  }


DestroyString(HA1);
DestroyString(HA2);
DestroyString(ClientNonce);
DestroyString(Digest);
DestroyString(Tempstr);

return(SendStr);
}


void HTTPSendHeaders(STREAM *S, HTTPInfoStruct *Info)
{
char *SendStr=NULL, *Tempstr=NULL, *ptr;
ListNode *Curr;

STREAMClearDataProcessors(S);
SendStr=CopyStr(SendStr,Info->Method);
SendStr=CatStr(SendStr," ");

if (Info->Flags & HTTP_PROXY) Tempstr=HTTPInfoToURL(Tempstr, Info);
else Tempstr=HTTPQuoteChars(Tempstr,Info->Doc," ");
SendStr=CatStr(SendStr,Tempstr);

if (Info->Flags & HTTP_VER1_0) SendStr=CatStr(SendStr," HTTP/1.0\r\n");
else SendStr=MCatStr(SendStr," HTTP/1.1\r\n","Host: ",Info->Host,"\r\n",NULL);

if (StrLen(Info->PostContentType) >0)
{
	Tempstr=FormatStr(Tempstr,"Content-type: %s\r\n",Info->PostContentType);
	SendStr=CatStr(SendStr,Tempstr);
}

if (Info->PostContentLength > 0) 
{
	Tempstr=FormatStr(Tempstr,"Content-Length: %d\r\n",Info->PostContentLength);
	SendStr=CatStr(SendStr,Tempstr);
}

if (StrLen(Info->Destination))
{
	Tempstr=FormatStr(Tempstr,"Destination: %s\r\n",Info->Destination);
	SendStr=CatStr(SendStr,Tempstr);
}

/* If we have authorisation details then send them */
if (Info->Authorization) SendStr=HTTPHeadersAppendAuth(SendStr, "Authorization", Info, Info->Authorization);
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
			if (StrLen(Tempstr)) Tempstr=CatStr(Tempstr,", deflate");
			else Tempstr=CatStr(Tempstr,"deflate");
		}
	}

	if (StrLen(Tempstr)) SendStr=MCatStr(SendStr,"Accept-Encoding: ",Tempstr,"\r\n",NULL);
	else SendStr=CatStr(SendStr,"Accept-Encoding: *.*\r\n");
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

ptr=LibUsefulGetValue("HTTP:User-Agent");
if (StrLen(ptr)) SendStr=MCatStr(SendStr,"User-Agent: ",ptr, "\r\n",NULL);

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
STREAMFlush(S);

DestroyString(Tempstr);
DestroyString(SendStr);
}




void HTTPReadHeaders(STREAM *S, HTTPInfoStruct *Info)
{
char *Tempstr=NULL, *ptr;


ListClear(Info->ServerHeaders,DestroyString);
Info->ContentLength=0;

//Not needed
//Info->RedirectPath=CopyStr(Info->RedirectPath,"");
Info->Flags &= ~(HTTP_CHUNKED | HTTP_GZIP | HTTP_DEFLATE);
Tempstr=STREAMReadLine(Tempstr,S);
if (Tempstr)
{
	if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"RESPONSE: %s\n",Tempstr);
  if (strncmp(Tempstr,"HTTP/",5)==0)
  {
    ptr=strchr(Tempstr,' ');
    ptr++;

    Info->ResponseCode=CopyStrLen(Info->ResponseCode,ptr,3);
    STREAMSetValue(S,"HTTP:ResponseCode",Info->ResponseCode);
  
  }
Tempstr=STREAMReadLine(Tempstr,S);
}

while (Tempstr)
{
StripTrailingWhitespace(Tempstr);
if (StrLen(Tempstr)==0) break;
HTTPParseHeader(S, Info,Tempstr);
Tempstr=STREAMReadLine(Tempstr,S);
}

//Handle Response Codes
if (StrLen(Info->ResponseCode))
{
		if (*Info->ResponseCode=='3') 
		{
			//No longer a flag, HTTP Redirect is now just a response code
			//Info->Flags |= HTTP_REDIRECT;
		}

		if (strcmp(Info->ResponseCode,"401")==0) 
		{
			if (Info->AuthFlags) Info->AuthFlags |= HTTP_AUTH_BASIC;
		}

		if (strcmp(Info->ResponseCode,"407")==0) 
		{
			if (Info->AuthFlags) Info->AuthFlags |= HTTP_AUTH_PROXY;
		}

}

S->BytesRead=0;
DestroyString(Tempstr);
}



int HTTPProcessResponse(HTTPInfoStruct *HTTPInfo)
{
int result=HTTP_ERROR;
char *Proto=NULL, *PortStr=NULL;
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
	ParseURL(HTTPInfo->RedirectPath, &Proto, &HTTPInfo->Host, &PortStr,NULL, NULL,&HTTPInfo->Doc,NULL);
	HTTPInfo->Port=atoi(PortStr);

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

DestroyString(Proto);
DestroyString(PortStr);

return(result);
}

STREAM *HTTPSetupConnection(HTTPInfoStruct *Info, int ForceHTTPS)
{
char *Tempstr=NULL, *Host=NULL, *Token=NULL, *ptr;
char *Logon=NULL, *Pass=NULL;
int Port=0, Flags=0;
STREAM *S;

S=STREAMCreate();

if (Info->Flags & HTTP_PROXY)
{
	ParseURL(Info->Proxy, &Tempstr, &Host, &Token, &Logon, &Pass, NULL,NULL);
	//&Info->ProxyAuthorization->Logon,
	Port=atoi(Token);
	
	
	if (ForceHTTPS) Tempstr=CopyStr(Tempstr,"https");

	if (strcasecmp(Tempstr,"https")==0) Flags |= CONNECT_SSL; 
}
else
{
	Host=CopyStr(Host,Info->Host);
	Port=Info->Port;

	if (Info->Flags & HTTP_SSL) Flags |= CONNECT_SSL;
	if (ForceHTTPS) 
	{
		Flags |= CONNECT_SSL;
	}

	if (Port==0)
	{
		if (Flags & CONNECT_SSL) Port=443;
		else Port=80;
	}
}

Tempstr=CopyStr(Tempstr,GetVar(HTTPVars,"Tunnel"));
ptr=GetToken(Tempstr,",",&Token,0);
while (ptr)
{
	STREAMAddConnectionHop(S,Token);
	ptr=GetToken(ptr,",",&Token,0);
}

if (Info->Flags & HTTP_TUNNEL) STREAMAddConnectionHop(S,Info->Proxy);
if (STREAMConnectToHost(S,Host,Port,Flags))
{
	HTTPSendHeaders(S,Info);
}
else
{
	STREAMClose(S);
	S=NULL;
}

Info->S=S;

DestroyString(Tempstr);
DestroyString(Token);
DestroyString(Host);

return(S);
}



STREAM *HTTPConnect(HTTPInfoStruct *Info)
{
STREAM *S=NULL;

if (
		(g_Flags & HTTP_REQ_HTTPS) ||
		(g_Flags & HTTP_TRY_HTTPS)
	)
{
S=HTTPSetupConnection(Info, TRUE);
if (g_Flags & HTTP_REQ_HTTPS) return(S);
}

if (!S) S=HTTPSetupConnection(Info, FALSE);

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

			if (StrLen(Info->PostData)) 
			{
				STREAMWriteLine(Info->PostData,Info->S);
				if (Info->Flags & HTTP_DEBUG) fprintf(stderr,"\n%s\n",Info->PostData);
			}
			else
			{
				if (strcasecmp(Info->Method,"POST")==0) break;
				if (strcasecmp(Info->Method,"PUT")==0) break;
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

		if (result == HTTP_OKAY) break;
		if (result == HTTP_NOTFOUND) break;
		if (result == HTTP_NOTMODIFIED) break;
		if (result == HTTP_ERROR) break;
		if (result == HTTP_CIRCULAR_REDIRECTS) break;


		if (result == HTTP_AUTH_BASIC) 
		{
					if (
									(Info->AuthFlags & HTTP_AUTH_SENT) ||
									(StrLen(Info->Authorization)==0) 
			 			)
					{
						if (result == HTTP_AUTH_BASIC) break;
						if (result == HTTP_AUTH_DIGEST) break;
					}
		}

		if (
					(result == HTTP_AUTH_PROXY) && 
					(StrLen(Info->ProxyAuthorization)==0) 
			 )
		{
			 break;
		}

		STREAMClose(Info->S);
		Info->S=NULL;
	}
	else break;
}


return(Info->S);
}



STREAM *HTTPMethod(char *Method, char *URL, char *Logon, char *Password, char *ContentType, char *ContentData, int ContentLength)
{
HTTPInfoStruct *Info;
STREAM *S;


Info=HTTPInfoFromURL(Method, URL);

if (StrLen(ContentType))
{
Info->PostContentType=CopyStr(Info->PostContentType,ContentType);
Info->PostData=CopyStr(Info->PostData,ContentData);
Info->PostContentLength=ContentLength;
}

if (StrLen(Logon) || StrLen(Password))
{
	if (Logon==HTTP_AUTH_BY_TOKEN) HTTPInfoSetAuth(Info,"", Password, HTTP_AUTH_TOKEN);
	else HTTPInfoSetAuth(Info,Logon, Password, HTTP_AUTH_BASIC);
}
S=HTTPTransact(Info);

HTTPInfoDestroy(Info);
return(S);
}


STREAM *HTTPGet(char *URL, char *Logon, char *Password)
{
return(HTTPMethod("GET", URL, Logon, Password,"","",0));
}


STREAM *HTTPPost(char *URL, char *Logon, char *Password, char *ContentType, char *Content)
{
return(HTTPMethod("POST", URL, Logon, Password, ContentType, Content, StrLen(Content)));
}


void HTTPCopyToSTREAM(STREAM *Con, STREAM *S)
{
char *Tempstr=NULL;
int result;

Tempstr=SetStrLen(Tempstr,BUFSIZ);
result=STREAMReadBytes(Con, Tempstr,BUFSIZ);
while (result > 0)
{
	STREAMWriteBytes(S,Tempstr,result);
	result=STREAMReadBytes(Con, Tempstr,BUFSIZ);
}

}


int HTTPDownload(char *URL,char *Login,char *Password, STREAM *S)
{
STREAM *Con;
Con=HTTPGet(URL,Login,Password);
if (! Con) return(FALSE);
HTTPCopyToSTREAM(Con, S);
return(TRUE);
}

void HTTPSetUserAgent(char *AgentName)
{
LibUsefulSetValue("HTTP:User-Agent",AgentName);
}

void HTTPSetProxy(char *Proxy)
{
LibUsefulSetValue("HTTP:Proxy",Proxy);
}

void HTTPSetFlags(int Flags)
{
g_Flags=Flags;
}

int HTTPGetFlags()
{
return(g_Flags);
}


