#include "OAuth.h"
#include "Http.h"
#include "Markup.h"
#include "DataParser.h"
#include "Encodings.h"
#include "Hash.h"
#include "Users.h"
#include "Entropy.h"
#include "URL.h"

static ListNode *OAuthTypes=NULL;
static ListNode *OAuthKeyChain=NULL;


void OAuthDestroy(void *p_OAuth)
{
    OAUTH *Ctx;
    ListNode *Curr;

    Ctx=(OAUTH *) p_OAuth;
    Curr=ListFindNamedItem(OAuthKeyChain, Ctx->Name);
    if (Curr) ListDeleteNode(Curr);

    DestroyString(Ctx->Name);
    DestroyString(Ctx->Stage1);
    DestroyString(Ctx->Stage2);
    DestroyString(Ctx->VerifyTemplate);
    DestroyString(Ctx->AccessToken);
    DestroyString(Ctx->RefreshToken);
    DestroyString(Ctx->RefreshURL);
    DestroyString(Ctx->VerifyURL);
    DestroyString(Ctx->VerifyCode);
    DestroyString(Ctx->Creds);
    DestroyString(Ctx->SavePath);
    ListDestroy(Ctx->Vars, Destroy);
    free(Ctx);
}



static OAUTH *OAuthKeyChainAdd(const char *Name, OAUTH *OA)
{
    ListNode *Node;
    OAUTH *Ctx;

    Ctx=OA;
    if (! OAuthKeyChain) OAuthKeyChain=ListCreate();

    Node=ListFindNamedItem(OAuthKeyChain, Name);
    if (Node)
    {
        Ctx=Node->Item;
        Ctx->AccessToken=CopyStr(Ctx->AccessToken, OA->AccessToken);
        Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, OA->AccessToken);
        CopyVars(Ctx->Vars, OA->Vars);
    }
    else ListAddNamedItem(OAuthKeyChain, Name, Ctx);

    return(Ctx);
}


void AddOAuthType(const char *Name, const char *Stage1Args, const char *Stage2Args, const char *VerifyTemplate)
{
    char *Tempstr=NULL;

    if (! OAuthTypes) OAuthTypes=ListCreate();
    Tempstr=MCopyStr(Tempstr, Stage1Args, ",", Stage2Args, ",", VerifyTemplate, NULL);
    SetVar(OAuthTypes, Name, Tempstr);

    Destroy(Tempstr);
}


static void SetupOAuthTypes()
{
    AddOAuthType( "empty",  "", "", "");
    AddOAuthType( "implicit",  "response_type=token&client_id=$(client_id)&redirect_uri=$(redirect_uri)&scope=basic&state=$(session)", "", "");
    AddOAuthType( "auth",  "", "code=$(code)&grant_type=authorization_code&redirect_uri=$(redirect_uri)", "$(url)?response_type=code&client_id=$(client_id)&redirect_uri=$(redirect_uri)&scope=$(scope)&state=$(session)");
    AddOAuthType( "auth-body",  "", "client_id=$(client_id)&client_secret=$(client_secret)&code=$(code)&grant_type=authorization_code&redirect_uri=$(redirect_uri)", "$(url)?response_type=code&client_id=$(client_id)&redirect_uri=$(redirect_uri)&scope=$(scope)&state=$(session)");
    AddOAuthType( "device",  "client_id=$(client_id)&scope=$(scope)", "client_id=$(client_id)&client_secret=$(client_secret)&code=$(device_code)&grant_type=http://oauth.net/grant_type/device/1.0", "");
    AddOAuthType( "password",  "client_name=$(client_id)&scope=$(scope)&redirect_uris=$(redirect_uri)&grant_type=password", "client_id=$(client_id)&client_secret=$(client_secret)&grant_type=password&username=$(username)&password=$(password)", "");
    AddOAuthType( "getpocket.com",  "consumer_key=$(client_id)&scope=$(scope)&redirect_uri=$(redirect_uri)", "consumer_key=$(client_id)&code=$(code)", "https://getpocket.com/auth/authorize?request_token=$(code)&redirect_uri=$(redirect_uri)");
    AddOAuthType( "pkce", "", "client_id=$(client_id)&client_secret=$(client_secret)&code=$(code)&grant_type=authorization_code&code_verifier=$(code_verifier)&redirect_uri=$(redirect_uri)", "$(url)?client_id=$(client_id)&response_type=code&code_challenge=$(code_challenge)&code_challenge_method=S256&redirect_uri=$(redirect_uri)&scope=$(scope)&token_access_type=$(token_access_type)");
}


//Only include Arguments in the URL if they have values. Some oauth implementations will balk at '&redirect_uri=&' but will happily accept
//no redirect_uri at all.
static char *OAuthBuildArgs(char *RetStr, const char *Template, ListNode *Vars)
{
    char *Tempstr=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;

    if (strchr(Template, '?'))
    {
        ptr=GetToken(Template, "?", &Value, 0);
        //when substituting vars in the proto://host:port part of the string
        //do not do http quoting
        RetStr=SubstituteVarsInString(RetStr, Value, Vars, 0);
        RetStr=CatStr(RetStr, "?");
    }
    else
    {
        RetStr=CopyStr(RetStr,"");
        ptr=Template;
    }

    Tempstr=SubstituteVarsInString(Tempstr, ptr, Vars, SUBS_HTTP_VARS);
    ptr=GetNameValuePair(Tempstr, "&", "=", &Name, &Value);
    while (ptr)
    {
        if (StrValid(Name) && StrValid(Value)) RetStr=MCatStr(RetStr, Name, "=", Value, "&", NULL);
        ptr=GetNameValuePair(ptr, "&", "=", &Name, &Value);
    }


    Destroy(Tempstr);
    Destroy(Name);
    Destroy(Value);

    return(RetStr);
}


void OAuthSetRedirectURI(OAUTH *Ctx, const char *URI)
{
    SetVar(Ctx->Vars, "redirect_uri", URI);
}



OAUTH *OAuthCreate(const char *Type, const char *Name, const char *ClientID, const char *ClientSecret, const char *Scopes, const char *RefreshURL)
{
    OAUTH *Ctx;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    if (OAuthTypes==NULL) SetupOAuthTypes();
    ptr=GetVar(OAuthTypes, Type);
    if (! StrValid(ptr)) return(NULL);

    Ctx=(OAUTH *) calloc(1,sizeof(OAUTH));

    ptr=GetToken(ptr,",",&(Ctx->Stage1), 0);
    ptr=GetToken(ptr,",",&(Ctx->Stage2), 0);
    ptr=GetToken(ptr,",",&(Ctx->VerifyTemplate), 0);
    Ctx->Name=CopyStr(Ctx->Name, Name);
    Ctx->Vars=ListCreate();
    SetVar(Ctx->Vars,"client_name", ClientID);
    SetVar(Ctx->Vars,"client_id", ClientID);
    SetVar(Ctx->Vars,"client_secret", ClientSecret);
    SetVar(Ctx->Vars,"scope", Scopes);
    SetVar(Ctx->Vars,"redirect_uri","urn:ietf:wg:oauth:2.0:oob");
    SetVar(Ctx->Vars,"connect_back_page","<html><body><h1>Code Accepted By Application</h1><body></html>");
    SetVar(Ctx->Vars,"refresh_url", RefreshURL);
    Ctx->AccessToken=CopyStr(Ctx->AccessToken, "");
    Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, "");
    Ctx->SavePath=MCopyStr(Ctx->SavePath, GetCurrUserHomeDir(), "/.oauth.creds",NULL);

    if (strcasecmp(Type, "getpocket.com")==0)
    {
        ptr=GetToken(ClientID,"-",&Token,0);
        Tempstr=MCopyStr(Tempstr, "pocketapp",Token,":authorizationFinished",NULL);
        SetVar(Ctx->Vars, "redirect_uri",Tempstr);

//Ctx->VerifyURL=MCopyStr(Ctx->VerifyURL,"https://getpocket.com/auth/authorize?request_token=",Ctx->AccessToken,"&redirect_uri=",Args,NULL);
    }
    else if (strcasecmp(Type, "implicit")==0) Ctx->Flags |= OAUTH_IMPLICIT;
    else if (strcasecmp(Type, "pkce")==0)
    {
        GenerateRandomBytes(&Token, 44, ENCODE_RBASE64);
        strrep(Token, '=', '\0');
        StripTrailingWhitespace(Token);
        SetVar(Ctx->Vars,"code_verifier",Token);
        HashBytes(&Tempstr, "sha256", Token, StrLen(Token), ENCODE_RBASE64);
        strrep(Tempstr, '=', '\0');
        StripTrailingWhitespace(Tempstr);
        SetVar(Ctx->Vars,"code_challenge",Tempstr);
    }


    DestroyString(Tempstr);
    DestroyString(Token);
    return(Ctx);
}







static int OAuthParseReply(OAUTH *Ctx, const char *ContentType, const char *Reply)
{
    ListNode *P=NULL, *Curr=NULL;
    const char *ptr;
    int RetVal=FALSE;

    if (! StrValid(ContentType)) return(FALSE);
    if (! StrValid(Reply)) return(FALSE);


    if (LibUsefulDebugActive()) fprintf(stderr, "OAuthParseReply: content-type=%s %s\n", ContentType, Reply);
    P=ParserParseDocument(ContentType, Reply);
    if (P)
    {
        ptr=ParserGetValue(P, "state");
        if (StrValid(ptr) && (strcmp(ptr, GetVar(Ctx->Vars, "session")) !=0) )
        {
            RaiseError(0, "OAuthParseReply", "State value [%s] does not match expected [%s]", ptr, GetVar(Ctx->Vars, "session"));
        }
        else
        {
            Curr=ListGetNext(P);
            while (Curr)
            {
                if (LibUsefulDebugActive()) fprintf(stderr, "OAuthParseReply: SetVar: [%s] [%s]\n", Curr->Tag, (char *) Curr->Item);
                SetVar(Ctx->Vars, Curr->Tag, (char *) Curr->Item);
                Curr=ListGetNext(Curr);
            }

            ptr=ParserGetValue(P, "access_token");
            if (StrValid(ptr)) Ctx->AccessToken=CopyStr(Ctx->AccessToken, ptr);

            ptr=ParserGetValue(P, "refresh_token");
            if (StrValid(ptr)) Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, ptr);

            Ctx->VerifyCode=CopyStr(Ctx->VerifyCode, ParserGetValue(P, "user_code"));
            Ctx->VerifyURL=CopyStr(Ctx->VerifyURL, ParserGetValue(P, "verification_url"));
            RetVal=TRUE;
        }

        ParserItemsDestroy(P);
    }
    else RaiseError(0, "OAuthParseReply", "Failed to parse '%s' document '%s'", ContentType, Reply);

    return(RetVal);
}


//Accept an oauth reply on stdin which would either be a 'verifycode' used in oob flow
//or else an entire redirect url pasted from a browser
static void OAuthHandleStdInRedirect(STREAM *S, OAUTH *Ctx)
{
    char *Tempstr=NULL;

    STREAMSetTimeout(S,0);
    Tempstr=STREAMReadLine(Tempstr, S);
    if ( (strncmp(Tempstr, "http:", 5)==0) || (strncmp(Tempstr, "https:", 6)==0) ) OAuthParseReply(Ctx, "application/x-www-form-urlencoded", Tempstr);
    else
    {
        StripTrailingWhitespace(Tempstr);
        Ctx->VerifyCode=CopyStr(Ctx->VerifyCode, Tempstr);
        SetVar(Ctx->Vars, "code", Ctx->VerifyCode);
    }

    Destroy(Tempstr);
}


//
static int OAuthAcceptRedirect(STREAM *Serv, OAUTH *Ctx)
{
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr, *tptr;
    int RetVal=FALSE;
    STREAM *S;

    S=STREAMServerAccept(Serv);
    if (S != NULL)
    {
        Tempstr=STREAMReadLine(Tempstr, S);

        if (Tempstr)
        {
            if (LibUsefulDebugActive()) fprintf(stderr, "OAUTH REDIRECT: %s", Tempstr);

            //GET (or possibly POST)
            ptr=GetToken(Tempstr,"\\S", &Token,0);
            //URL
            ptr=GetToken(ptr,"\\S", &Token,0);
            if (ptr)
            {
                tptr=strchr(Token, '?');
                if (tptr) tptr++;
                else tptr=Token;
                OAuthParseReply(Ctx, "application/x-www-form-urlencoded", tptr);
            }


            while (Tempstr)
            {
                StripTrailingWhitespace(Tempstr);
                if (! StrValid(Tempstr)) break;
                if (LibUsefulDebugActive()) fprintf(stderr, "OAUTH_REDIRECT: %s\n", Tempstr);
                Tempstr=STREAMReadLine(Tempstr, S);
            }
            RetVal=TRUE;
        }

        Tempstr=MCopyStr(Tempstr, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n",GetVar(Ctx->Vars,"connect_back_page"),NULL);
        STREAMWriteLine(Tempstr, S);
        STREAMClose(S);
    }

    DestroyString(Tempstr);
    DestroyString(Token);

    return(RetVal);
}



static char *OAuthReformatRedirectURL(char *RetStr, const char *RedirectURL)
{
    char *Proto=NULL, *Host=NULL, *Port=NULL, *Path=NULL;

    RetStr=CopyStr(RetStr, "");
    ParseURL(RedirectURL, &Proto, &Host, &Port, &Path, NULL, NULL, NULL);

    if (strcasecmp(Host, "localhost")==0) Host=CopyStr(Host, "127.0.0.1");
    else if (strcasecmp(Host, "127.0.0.1")==0) Host=CopyStr(Host, "127.0.0.1");
    else Host=CopyStr(Host, "0.0.0.0");

    if (strcasecmp(Proto, "https")==0) Proto=CopyStr(Proto, "tls");
    else if (strcasecmp(Proto, "http")==0) Proto=CopyStr(Proto, "tcp");

    RetStr=MCopyStr(RetStr, Proto, ":", Host, ":", Port, NULL);

    Destroy(Proto);
    Destroy(Host);
    Destroy(Port);
    Destroy(Path);

    return(RetStr);
}



int OAuthAwaitRedirect(OAUTH *Ctx, const char *RedirectURL, const char *URL, int Flags)
{
    STREAM *S, *Serv=NULL, *StdIn=NULL;
    char *RedirURL=NULL, *Tempstr=NULL;
    ListNode *Connections=NULL;

    RedirURL=OAuthReformatRedirectURL(RedirURL, RedirectURL);
    if (StrValid(RedirURL))
    {
        Tempstr=CopyStr(Tempstr, "rw ");

        //if we're using tls for the redirect url, then set it up here
        if (strncasecmp(RedirURL, "tls:", 4) ==0) Tempstr=MCatStr(Tempstr, " SSL:CertFile=", GetVar(Ctx->Vars, "SSL:CertFile"), " SSL:KeyFile=", GetVar(Ctx->Vars, "SSL:KeyFile"), NULL);

        Serv=STREAMServerNew(RedirURL, Tempstr);
        if (Serv==NULL)
        {
            RaiseError(0, "OAuthAwaitRedirect", "Failed to bind url: %s", RedirURL);
            return(FALSE);
        }
    }

    Connections=ListCreate();
    if (Serv) ListAddItem(Connections, Serv);
    if (Flags & OAUTH_STDIN)
    {
        StdIn=STREAMFromDualFD(0,1);
        if (StdIn) ListAddItem(Connections, StdIn);
    }

    S=STREAMSelect(Connections, NULL);
    if (S != NULL)
    {
        if (S==Serv) OAuthAcceptRedirect(S, Ctx);
        else OAuthHandleStdInRedirect(S, Ctx);

        OAuthFinalize(Ctx, URL);
    }

    //Close Serv, but just destroy StdIn, as we don't want to
    //actually close our stdin and stdout file descriptors
    STREAMClose(Serv);
    STREAMDestroy(StdIn);
    ListDestroy(Connections, NULL);

    DestroyString(Tempstr);
    DestroyString(RedirURL);

    return(TRUE);
}


int OAuthListen(OAUTH *Ctx, int Port, const char *URL, int Flags)
{
    int RetVal=FALSE;
    char *Tempstr=NULL;

    Tempstr=FormatStr(Tempstr, "tcp:127.0.0.1:%d", Port);
    RetVal=OAuthAwaitRedirect(Ctx, Tempstr, URL, Flags);

    Destroy(Tempstr);
    return(RetVal);
}


char *HTTPURLAddAuth(char *RetStr, const char *URL, const char *User, const char *Password)
{
    const char *ptr;
    char *Tempstr=NULL;


//some systems want client ID and client Secret to be sent as a login with 'basic' authentication
    ptr=GetToken(URL, ":", &Tempstr, 0);
    while (*ptr=='/') ptr++;
    RetStr=MCopyStr(RetStr, Tempstr, "://", User, ":", Password, "@", ptr, NULL);

    Destroy(Tempstr);
    return(RetStr);
}


//curl -X POST -d "client_id=CLIENT_ID_HERE&client_secret=CLIENT_SECRET_HERE&grant_type=password&username=YOUR_EMAIL&password=YOUR_PASSWORD" -Ss https://mastodon.social/oauth/token
int OAuthGrant(OAUTH *Ctx, const char *iURL, const char *PostArgs)
{
    STREAM *S;
    char *Tempstr=NULL, *URL=NULL;
    const char *ptr;
    int len, result=FALSE;

    if (LibUsefulDebugActive()) fprintf(stderr, "DEBUG: OAuthGrant: %s args: %s\n", iURL, PostArgs);
    URL=HTTPURLAddAuth(URL, iURL, GetVar(Ctx->Vars, "client_id"), GetVar(Ctx->Vars, "client_secret"));
    //S=HTTPMethod("POST",URL,"application/x-www-form-urlencoded; charset=UTF-8",PostArgs,StrLen(PostArgs));

    Tempstr=FormatStr(Tempstr, "w content-type='application/x-www-form-urlencoded; charset=UTF-8' content-length=%d", StrLen(PostArgs));
    if (StrValid(GetVar(Ctx->Vars, "User-Agent"))) Tempstr=MCatStr(Tempstr, " User-Agent=", GetVar(Ctx->Vars, "User-Agent"), NULL);

    S=STREAMOpen(URL, Tempstr);
    if (S)
    {
        STREAMWriteLine(PostArgs, S);
        STREAMCommit(S);
        sleep(1);
        Tempstr=STREAMReadDocument(Tempstr, S);

        if (LibUsefulDebugActive()) fprintf(stderr, "DEBUG: OAuthGrant Response: %s\n", Tempstr);
        result=OAuthParseReply(Ctx, STREAMGetValue(S, "HTTP:Content-Type"), Tempstr);
        STREAMClose(S);
    }
    else RaiseError(0, "OAuthGrant", "Failed to connect: %s", URL);

    DestroyString(Tempstr);
    DestroyString(URL);

    return(result);
}




int OAuthRefresh(OAUTH *Ctx, const char *iURL)
{
    char *Tempstr=NULL, *URL=NULL, *Args=NULL;
    const char *ptr;
    int result;


    /*
    POST, GET client_id (integer) :
    Your app's client_id (obtained during app registration)
    client_id and client_secrect can be provided via HTTP Basic Authentication see http://tools.ietf.org/html/rfc6750#section-2.1
    POST, GET client_secret (string) :
    Your app's client_secret (obtained during app registration)
    client_id and client_secrect can be provided via HTTP Basic Authentication see http://tools.ietf.org/html/rfc6750#section-2.1
    POST, GET grant_type (string) :
    The value must be refresh_token
    POST, GET refresh_token (string) :
    required
    */

    if (iURL) ptr=iURL;
    else ptr=GetVar(Ctx->Vars, "refresh_url");

    if (StrValid(ptr))
    {
        ptr=GetToken(ptr, "?", &Tempstr, 0);
        URL=CopyStr(URL, Tempstr);

        if (StrValid(ptr)) Args=SubstituteVarsInString(Args, ptr, Ctx->Vars, SUBS_HTTP_VARS);
        else
        {
            Tempstr=CopyStr(Tempstr, "grant_type=refresh_token&refresh_token=$(refresh_token)&client_id=$(client_id)&client_secret=$(client_secret)");
            //if (StrValid(GetVar(Ctx->Vars, "code"))) Tempstr=CatStr(Tempstr, "&code=$(code)");
            Args=SubstituteVarsInString(Args, Tempstr, Ctx->Vars, SUBS_HTTP_VARS);
        }

        result=OAuthGrant(Ctx, URL, Args);
        if (StrValid(Ctx->AccessToken)) OAuthSave(Ctx, "");

        //allow some time for new details to be saved at the other end
        sleep(1);
    }

    DestroyString(Tempstr);
    DestroyString(Args);
    DestroyString(URL);

    return(result);
}



int OAuthFinalize(OAUTH *Ctx, const char *URL)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=OAuthBuildArgs(Tempstr, Ctx->Stage2, Ctx->Vars);
    result=OAuthGrant(Ctx, URL, Tempstr);
    if (StrValid(Ctx->AccessToken)) OAuthSave(Ctx, "");

    DestroyString(Tempstr);
    return(result);
}

int OAuthImplicit(OAUTH *Ctx, const char *URL, const char *Args)
{
    HTTPInfoStruct *Info;
    char *Tempstr=NULL;
    int result=FALSE;
    STREAM *S;

    Tempstr=MCopyStr(Tempstr,URL,"?",Args,NULL);
    Info=HTTPInfoFromURL("GET", Tempstr);
//Info->Flags |= HTTP_NOREDIRECT;
    S=HTTPTransact(Info);
    if (S)
    {
        if (StrValid(Info->RedirectPath)) result=OAuthParseReply(Ctx, "url", Info->RedirectPath);
    }
    STREAMClose(S);

    DestroyString(Tempstr);

    return(result);
}


int OAuthStage1(OAUTH *Ctx, const char *URL)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=GetRandomAlphabetStr(Tempstr, 10);
    SetVar(Ctx->Vars, "session",Tempstr);
    SetVar(Ctx->Vars, "url", URL);
    Tempstr=OAuthBuildArgs(Tempstr, Ctx->Stage1, Ctx->Vars);

    if (LibUsefulDebugActive()) fprintf(stderr, "OAuthStage1: %s\n", Tempstr);
    if (StrValid(Tempstr))
    {
        if (Ctx->Flags & OAUTH_IMPLICIT) Ctx->VerifyURL=MCopyStr(Ctx->VerifyURL, URL, "?", Tempstr, NULL);
        //result=OAuthImplicit(Ctx, URL, Tempstr);
        else result=OAuthGrant(Ctx, URL, Tempstr);
    }

    if (StrValid(Ctx->VerifyTemplate)) Ctx->VerifyURL=OAuthBuildArgs(Ctx->VerifyURL, Ctx->VerifyTemplate, Ctx->Vars);

    DestroyString(Tempstr);
    return(result);
}


void OAuthSetUserCreds(OAUTH *Ctx, const char *UserName, const char *Password)
{
    SetVar(Ctx->Vars, "username", UserName);
    SetVar(Ctx->Vars, "password", Password);
}



void OAuthParse(OAUTH *Ctx, const char *Line)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    ptr=GetToken(Line,"\\S", &Ctx->Name, GETTOKEN_QUOTES);
    ptr=GetNameValuePair(ptr," ", "=", &Name, &Value);
    while (ptr)
    {
        SetVar(Ctx->Vars, Name, Value);
        ptr=GetNameValuePair(ptr," ", "=", &Name, &Value);
    }

    Ctx->AccessToken=CopyStr(Ctx->AccessToken, GetVar(Ctx->Vars,"access_token"));
    Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, GetVar(Ctx->Vars,"refresh_token"));

    DestroyString(Name);
    DestroyString(Value);
}


//cleans old entries out of the credentials file by loading all entries into a list
//so that newer entries overwrite old ones, then writing back to disk
static void OAuthCleanupCredsFile(const char *Path)
{
    char *Tempstr=NULL, *Name=NULL;
    ListNode *Items=NULL, *Curr;
    const char *ptr;
    STREAM *S;

    Items=ListCreate();
    S=STREAMOpen(Path, "rw");
    if (S)
    {
        STREAMLock(S, LOCK_EX);
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            if (StrValid(Tempstr))
            {
                ptr=GetToken(Tempstr, "\\S", &Name, GETTOKEN_QUOTES);
                SetVar(Items, Name, ptr);
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }

        STREAMTruncate(S, 0);
        STREAMSeek(S, SEEK_SET, 0);
        Curr=ListGetNext(Items);
        while (Curr)
        {
            Tempstr=MCopyStr(Tempstr, "'", Curr->Tag, "' ", Curr->Item, "\n", NULL);
            STREAMWriteLine(Tempstr, S);
            Curr=ListGetNext(Curr);
        }

        STREAMClose(S);
    }

    ListDestroy(Items, Destroy);
    Destroy(Tempstr);
    Destroy(Name);
}


int OAuthLoad(OAUTH *Ctx, const char *ReqName, const char *iPath)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL, *Name=NULL, *Path=NULL;
    const char *ptr;
    int result=FALSE;
    int MatchingLines=0;

    //if we are explictly passed an entry name then use that,
    //else use the one from the OAUTH object
    if (StrValid(ReqName)) Name=CopyStr(Name, ReqName);
    else Name=CopyStr(Name, Ctx->Name);

    //if we are explictly passed a file path then use that,
    //else use the one from the OAUTH object
    if (StrValid(iPath)) Path=CopyStr(Path, iPath);
    else Path=CopyStr(Path, Ctx->SavePath);

    Ctx->AccessToken=CopyStr(Ctx->AccessToken, "");
    Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, "");

    S=STREAMOpen(Path, "r");
    if (S)
    {
        STREAMLock(S, LOCK_SH);
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr, "\\S", &Token, GETTOKEN_QUOTES);
            if (strcmp(Token, Name)==0)
            {
                OAuthParse(Ctx, Tempstr);
                if (StrValid(Ctx->AccessToken) || StrValid(Ctx->RefreshToken)) result=TRUE;
                MatchingLines++;
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    //if the item we were looking for has more than 5 entries in the OAuth credentials file
    //then there's too many 'old' entries that have been refreshed, and we should clean up
    //the file
    if (MatchingLines > 5) OAuthCleanupCredsFile(Path);

    Ctx=OAuthKeyChainAdd(Name, Ctx);


    DestroyString(Tempstr);
    DestroyString(Token);
    DestroyString(Name);
    DestroyString(Path);

    return(result);
}



int OAuthSave(OAUTH *Ctx, const char *Path)
{
    STREAM *S;
    const char *Fields[]= {"client_id","client_secret","access_token","refresh_token","refresh_url","code",NULL};
    const char *ptr;
    char *Tempstr=NULL;
    int i;

    if (! StrValid(Path))
    {
        if (! StrValid(Ctx->SavePath)) return(FALSE);
        S=STREAMOpen(Ctx->SavePath,"aE");
    }
    else S=STREAMOpen(Path,"aE");
    if (S)
    {
        STREAMLock(S, LOCK_EX);
        Tempstr=MCopyStr(Tempstr, "'", Ctx->Name,"' ",NULL);
        for (i=0; Fields[i] !=NULL; i++)
        {
            ptr=GetVar(Ctx->Vars,Fields[i]);
            if (StrValid(ptr)) Tempstr=MCatStr(Tempstr, Fields[i], "='", ptr, "' ",NULL);
        }

        Tempstr=CatStr(Tempstr,"\n");

        STREAMWriteLine(Tempstr, S);
        STREAMClose(S);
    }
    else RaiseError(0, "OAuthSave", "Failed to open: %s", Path);

    DestroyString(Tempstr);

    return(TRUE);
}



const char *OAuthLookup(const char *Name, int Refresh)
{
    ListNode *Curr;
    OAUTH *OA;

    Curr=ListFindNamedItem(OAuthKeyChain, Name);
    if (Curr) OA=(OAUTH *) Curr->Item;
    else
    {
        OA=OAuthCreate("empty", Name, "", "", "", "");
        OA->SavePath=MCopyStr(OA->SavePath, GetCurrUserHomeDir(), "/.oauth.creds",NULL);
        OAuthLoad(OA, Name, NULL);
    }

    if (Refresh && StrValid(OA->RefreshToken)) OAuthRefresh(OA, NULL);

    if (StrValid(OA->AccessToken)) return(OA->AccessToken);

    return("");
}
