#include "HttpServer.h"
#include "Tokenizer.h"
#include "String.h"
#include "ContentType.h"
#include "StreamAuth.h"
#include "HttpUtil.h"

static void HTTPServerSetValue(STREAM *S, const char *Name, const char *Value)
{
    char *Tempstr=NULL;

//do not allow these to be set, as they will
//overwrite the true HTTP method and url
    if (CompareStrNoCase(Name, "HTTP-Version")==0) return;
    if (CompareStrNoCase(Name, "Method")==0) return;
    if (CompareStrNoCase(Name, "URL")==0) return;

    Tempstr=MCopyStr(Tempstr, "HTTP:", Name, NULL);
    STREAMSetValue(S, Tempstr, Value);

    Destroy(Tempstr);
}


void HTTPServerParseClientCookies(ListNode *Vars, const char *Str)
{
    const char *ptr;
    char *Name=NULL, *Value=NULL, *Tempstr=NULL;

    ptr=GetNameValuePair(Str, ";", "=", &Name, &Value);
    while (ptr)
    {
        StripTrailingWhitespace(Name);
        StripLeadingWhitespace(Name);
        StripTrailingWhitespace(Value);
        StripLeadingWhitespace(Value);

        //prepend with 'cookie' so we can find it in a STREAM var ist
        Tempstr=MCopyStr(Tempstr, "cookie:", Name, NULL);
        SetVar(Vars, Tempstr, Value);
        ptr=GetNameValuePair(ptr, ";", "=", &Name, &Value);
    }

    DestroyString(Name);
    DestroyString(Value);
    DestroyString(Tempstr);
}


void HTTPServerParseAuthorization(ListNode *Vars, const char *Str)
{
    char *Token=NULL, *Tempstr=NULL, *User=NULL, *Password=NULL;
    const char *ptr;

    ptr=Str;
    while (isspace(*ptr)) ptr++;
    ptr=GetToken(ptr, "\\S", &Token, 0);

    if (strcasecmp(Token, "basic")==0)
    {
        ptr=GetToken(ptr, "\\S", &Token, 0);
        HTTPDecodeBasicAuth(Token, &User, &Password);
        SetVar(Vars, "AUTH:User", User);
        SetVar(Vars, "AUTH:Password", Password);
    }
    else if (strcasecmp(Token, "bearer")==0)
    {
        ptr=GetToken(ptr, "\\S", &Token, 0);
        SetVar(Vars, "AUTH:Bearer", Token);
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(User);
    Destroy(Password);
}


void HTTPServerParseClientHeaders(STREAM *S)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    Tempstr=STREAMReadLine(Tempstr, S);
    StripTrailingWhitespace(Tempstr);
    ptr=GetToken(Tempstr, "\\S", &Token, 0);
    STREAMSetValue(S, "HTTP:Method", Token);
    ptr=GetToken(ptr, "\\S", &Token, 0);
    STREAMSetValue(S, "HTTP:URL", Token);
    STREAMSetValue(S, "HTTP:HTTP-Version", ptr);

    Tempstr=STREAMReadLine(Tempstr, S);
    while (Tempstr)
    {
        StripTrailingWhitespace(Tempstr);
        if (! StrValid(Tempstr)) break;

        ptr=GetToken(Tempstr, ":", &Token, 0);
        StripTrailingWhitespace(Token);
        while (isspace(*ptr)) ptr++;

        if (strcasecmp(Token, "Cookie")==0) HTTPServerParseClientCookies(S->Values, ptr);
        else if (strcasecmp(Token, "Authorization")==0) HTTPServerParseAuthorization(S->Values, ptr);
        else if (strcasecmp(Token, "Sec-Websocket-Key") == 0) STREAMSetValue(S, "WEBSOCKET:KEY", ptr);
        else if (strcasecmp(Token, "Sec-Websocket-Protocol") == 0) STREAMSetValue(S, "WEBSOCKET:PROTOCOL", ptr);
        //HTTPServerSetValue will ignore Method, URL and HTTP-Version so they can't be overridden by extra headers
        else HTTPServerSetValue(S, Token, ptr);

        Tempstr=STREAMReadLine(Tempstr, S);
    }

    Destroy(Tempstr);
    Destroy(Token);
}


void HTTPServerAccept(STREAM *S)
{
    HTTPServerParseClientHeaders(S);
    STREAMAuth(S);
}


#define HEADER_SEEN_CONNECTION 1
#define HEADER_SEEN_DATE       2


void HTTPServerSendHeaders(STREAM *S, int ResponseCode, const char *ResponseText, const char *Headers)
{
    char *Tempstr=NULL, *Output=NULL, *Hash=NULL;
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    int OutputSeen=0;

    Output=FormatStr(Output, "HTTP/1.1 %03d %s\r\n", ResponseCode, ResponseText);
    ptr=GetNameValuePair(Headers, "\\S", "=", &Name, &Value);
    while (ptr)
    {
        if (strcasecmp(Name, "Connection")==0) OutputSeen |= HEADER_SEEN_CONNECTION;
        if (strcasecmp(Name, "Date")==0) OutputSeen |= HEADER_SEEN_DATE;
        Output=MCatStr(Output, Name, ":", Value, "\r\n", NULL);
        ptr=GetNameValuePair(ptr, "\\S", "=", &Name, &Value);
    }

    if (! (OutputSeen & HEADER_SEEN_CONNECTION)) Output=CatStr(Output, "Connection: close\r\n");
    if (! (OutputSeen & HEADER_SEEN_DATE)) Output=MCatStr(Output, "Date: ", GetDateStr("%a, %d %b %Y %H:%M:%S GMT", "GMT"), "\r\n", NULL);

    Output=CatStr(Output, "\r\n");

    STREAMWriteLine(Output, S);
    STREAMFlush(S);

    Destroy(Output);
    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);
    Destroy(Hash);
}

int HTTPServerSendResponse(STREAM *S, const char *ResponseCode, const char *ResponseReason, const char *Content, int Length, const char *ContentType, const char *Headers)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=FormatStr(Tempstr, "Content-Length=%d ", Length);
    if (StrValid(ContentType)) Tempstr=MCatStr(Tempstr, "Content-Type=", ContentType, " ", Headers, NULL);
    HTTPServerSendHeaders(S, atoi(ResponseCode), ResponseReason, Tempstr);
    result=STREAMWriteBytes(S, Content, Length);

    Destroy(Tempstr);
    return(result);
}


int HTTPServerSendDocument(STREAM *S, const char *Content, int Length, const char *ContentType, const char *Headers)
{
    return(HTTPServerSendResponse(S, "200", "OKAY", Content, Length, ContentType, Headers));
}

int HTTPServerSendStatus(STREAM *S, const char *StatusCode, const char *StatusReason)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=FormatStr(Tempstr, "<html><body><h1>%s - %s</h1></body></html>\r\n", StatusCode, StatusReason);
    result=HTTPServerSendResponse(S, StatusCode, StatusReason, Tempstr, StrLen(Tempstr), "text/html", "");

    Destroy(Tempstr);

    return(result);
}


int HTTPServerSendFile(STREAM *S, const char *Path, const char *iContentType, const char *Headers)
{
    struct stat FStat;
    char *Tempstr=NULL, *ContentType=NULL;
    int bytes_sent=0;
    STREAM *F;

    if (stat(Path, &FStat) != 0) HTTPServerSendHeaders(S, 404, "NOT FOUND", NULL);
    else
    {
        if (StrValid(iContentType)) ContentType=CopyStr(ContentType, iContentType);
        else ContentType=CopyStr(ContentType, ContentTypeForFile(Path));

        F=STREAMOpen(Path, "r");
        if (! F) HTTPServerSendHeaders(S, 401, "FORBIDDEN", NULL);
        else
        {
            Tempstr=FormatStr(Tempstr, "Content-Length=%d ", FStat.st_size);
            if (StrValid(ContentType)) Tempstr=MCatStr(Tempstr, "Content-Type=", ContentType, " ", NULL);
            Tempstr=CatStr(Tempstr, Headers);
            HTTPServerSendHeaders(S, 200, "OKAY", Tempstr);
            bytes_sent=STREAMSendFile(F, S, 0, SENDFILE_LOOP);
            STREAMClose(F);
        }
    }
    Destroy(Tempstr);
    Destroy(ContentType);

    return(bytes_sent);
}
