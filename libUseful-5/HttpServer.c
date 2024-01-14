#include "HttpServer.h"
#include "Tokenizer.h"
#include "String.h"
#include "ContentType.h"
#include "StreamAuth.h"

static void HTTPServerSetValue(STREAM *S, const char *Name, const char *Value)
{
    char *Tempstr=NULL;

//do not allow these to be set, as they will
//overwrite the true HTTP method and url
    if (strcmp(Name, "Method")==0) return;
    if (strcmp(Name, "URL")==0) return;

    Tempstr=MCopyStr(Tempstr, "HTTP:", Name, NULL);
    STREAMSetValue(S, Tempstr, Value);

    Destroy(Tempstr);
}

void HTTPServerParseClientCookies(ListNode *Vars, const char *Str)
{
    const char *ptr;
    char *Name=NULL, *Value=NULL, *Tempstr=NULL;
    ListNode *Item;

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
    char *Token=NULL;
    const char *ptr;

    ptr=Str;
    while (isspace(*ptr)) ptr++;
    ptr=GetToken(ptr, "\\S", &Token, 0);

    if (strcasecmp(Token, "basic")==0)
    {
        ptr=GetToken(ptr, "\\S", &Token, 0);
        SetVar(Vars, "Auth:Basic", Token);
    }
    else if (strcasecmp(Token, "bearer")==0)
    {
        ptr=GetToken(ptr, "\\S", &Token, 0);
        SetVar(Vars, "Auth:Bearer", Token);
    }

    Destroy(Token);
}


void HTTPServerParseClientHeaders(STREAM *S)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    Tempstr=STREAMReadLine(Tempstr, S);
    StripTrailingWhitespace(Tempstr);
    ptr=GetToken(Tempstr, "\\S", &Token, 0);
    STREAMSetValue(S, "HTTP:Method", Token);
    STREAMSetValue(S, "HTTP:URL", ptr);

    Tempstr=STREAMReadLine(Tempstr, S);
    while (Tempstr)
    {
        StripTrailingWhitespace(Tempstr);
        if (! StrValid(Tempstr)) break;

        ptr=GetToken(Tempstr, ":", &Token, 0);
        StripTrailingWhitespace(Token);
        if (strcasecmp(Token, "Cookie:")==0) HTTPServerParseClientCookies(S->Values, ptr);
        else if (strcasecmp(Token, "Authorization")==0) HTTPServerParseAuthorization(S->Values, ptr);
        else if (strcasecmp(Token, "Sec-Websocket-Key") == 0) STREAMSetValue(S, "WEBSOCKET:KEY", ptr);
        else if (strcasecmp(Token, "Sec-Websocket-Protocol") == 0) STREAMSetValue(S, "WEBSOCKET:PROTOCOL", ptr);
        else HTTPServerSetValue(S, Token, ptr);

        Tempstr=STREAMReadLine(Tempstr, S);
    }

    Destroy(Tempstr);
    Destroy(Token);
}


void HTTPServerAccept(STREAM *S)
{
HTTPServerParseClientHeaders(S);
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


int HTTPServerSendDocument(STREAM *S, const char *Bytes, int Length, const char *ContentType, const char *Headers)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=FormatStr(Tempstr, "Content-Length=%d ", Length);
    if (StrValid(ContentType)) Tempstr=MCatStr(Tempstr, "Content-Type=", ContentType, " ", Headers, NULL);
    HTTPServerSendHeaders(S, 200, "OKAY", Tempstr);
    result=STREAMWriteBytes(S, Bytes, Length);

    Destroy(Tempstr);
    return(result);
}


int HTTPServerSendFile(STREAM *S, const char *Path, const char *iContentType, const char *Headers)
{
    struct stat FStat;
    char *Tempstr=NULL, *ContentType=NULL;
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
            STREAMSendFile(F, S, 0, SENDFILE_LOOP);
            STREAMClose(F);
        }
    }
    Destroy(Tempstr);
    Destroy(ContentType);
}
