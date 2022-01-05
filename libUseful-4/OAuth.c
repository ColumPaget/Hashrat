#include "OAuth.h"
#include "Http.h"
#include "Markup.h"
#include "DataParser.h"


static ListNode *OAuthTypes=NULL;
static ListNode *OAuthKeyChain=NULL;


void SetupOAuthTypes()
{
    OAuthTypes=ListCreate();
    SetVar(OAuthTypes, "implicit", "response_type=token&client_id=$(client_id)&redirect_uri=$(redirect_uri)&scope=basic&state=$(session)");
    SetVar(OAuthTypes, "device", "client_id=$(client_id)&scope=$(scope),client_id=$(client_id)&client_secret=$(client_secret)&code=$(device_code)&grant_type=http://oauth.net/grant_type/device/1.0");
    SetVar(OAuthTypes, "password", "client_name=$(client_id)&scope=$(scope)&redirect_uris=$(redirect_uri)&grant_type=password,client_id=$(client_id)&client_secret=$(client_secret)&grant_type=password&username=$(username)&password=$(password)");
    SetVar(OAuthTypes, "getpocket.com", "consumer_key=$(client_id)&scope=$(scope)&redirect_uri=$(redirect_uri),consumer_key=$(client_id)&code=$(code),https://getpocket.com/auth/authorize?request_token=$(code)&redirect_uri=$(redirect_uri)");
    SetVar(OAuthTypes, "auth", ",client_id=$(client_id)&client_secret=$(client_secret)&code=$(code)&grant_type=authorization_code&redirect_uri=$(redirect_uri),$(url)?response_type=code&client_id=$(client_id)&redirect_uri=$(redirect_uri)&scope=$(scope)&state=$(session)");
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
    SetVar(Ctx->Vars,"client_name",ClientID);
    SetVar(Ctx->Vars,"client_id",ClientID);
    SetVar(Ctx->Vars,"client_secret",ClientSecret);
    Tempstr=HTTPQuote(Tempstr, Scopes);
    SetVar(Ctx->Vars,"scope",Tempstr);
    SetVar(Ctx->Vars,"redirect_uri","urn:ietf:wg:oauth:2.0:oob");
    SetVar(Ctx->Vars,"connect_back_page","<html><body><h1>Code Accepted By Application</h1><body></html>");
    Ctx->AccessToken=CopyStr(Ctx->AccessToken, "");
    Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, "");
    Ctx->RefreshURL=CopyStr(Ctx->RefreshURL, RefreshURL);
    Ctx->SavePath=MCopyStr(Ctx->SavePath, GetCurrUserHomeDir(), "/.oauth.creds",NULL);

    if (strcasecmp(Type, "getpocket.com")==0)
    {
        ptr=GetToken(ClientID,"-",&Token,0);
        Tempstr=MCopyStr(Tempstr, "pocketapp",Token,":authorizationFinished",NULL);
        SetVar(Ctx->Vars, "redirect_uri",Tempstr);

//Ctx->VerifyURL=MCopyStr(Ctx->VerifyURL,"https://getpocket.com/auth/authorize?request_token=",Ctx->AccessToken,"&redirect_uri=",Args,NULL);
    }
    else if (strcasecmp(Type, "implicit")==0) Ctx->Flags |= OAUTH_IMPLICIT;

    if (! OAuthKeyChain) OAuthKeyChain=ListCreate();
    ListAddNamedItem(OAuthKeyChain, Name, Ctx);

    DestroyString(Tempstr);
    DestroyString(Token);
    return(Ctx);
}




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





int OAuthParseReply(OAUTH *Ctx, const char *ContentType, const char *Reply)
{
    ListNode *P=NULL, *Curr=NULL;
    const char *ptr;

    if (! StrValid(ContentType)) return(FALSE);
    if (! StrValid(Reply)) return(FALSE);


    P=ParserParseDocument(ContentType, Reply);
    Curr=ListGetNext(P);
    while (Curr)
    {
        SetVar(Ctx->Vars, Curr->Tag, (char *) Curr->Item);
        Curr=ListGetNext(Curr);
    }

    ptr=ParserGetValue(P, "access_token");
    if (StrValid(ptr)) Ctx->AccessToken=CopyStr(Ctx->AccessToken, ptr);

    ptr=ParserGetValue(P, "refresh_token");
    if (StrValid(ptr)) Ctx->RefreshToken=CopyStr(Ctx->RefreshToken, ptr);

    Ctx->VerifyCode=CopyStr(Ctx->VerifyCode, ParserGetValue(P, "user_code"));
    Ctx->VerifyURL=CopyStr(Ctx->VerifyURL, ParserGetValue(P, "verification_url"));

    ParserItemsDestroy(P);

    return(TRUE);
}



int OAuthConnectBack(OAUTH *Ctx, int sock)
{
    int result;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr, *tptr;
    STREAM *S;

    result=IPServerAccept(sock, NULL);
    if (result > -1)
    {
        S=STREAMFromFD(result);
        Tempstr=STREAMReadLine(Tempstr, S);

        if (Tempstr)
        {
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
                if (StrLen(Tempstr) ==0) break;
                Tempstr=STREAMReadLine(Tempstr, S);
            }
        }

        Tempstr=MCopyStr(Tempstr, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n",GetVar(Ctx->Vars,"connect_back_page"),NULL);
        STREAMWriteLine(Tempstr, S);
        STREAMClose(S);
    }

    DestroyString(Tempstr);
    DestroyString(Token);
    if (result > -1) return(TRUE);
    return(FALSE);
}


int OAuthListen(OAUTH *Ctx, int Port, const char *URL, int Flags)
{
    int netfd, result;
    STREAM *S;
    char *Tempstr=NULL;
    fd_set fds;


    FD_ZERO(&fds);

    if (Port > 0)
    {
        netfd=IPServerInit(SOCK_STREAM, "127.0.0.1", Port);
        if (netfd==-1) return(FALSE);
    }

    FD_SET(netfd, &fds);
    if (Flags & OAUTH_STDIN) FD_SET(0, &fds);

    result=select(netfd+1, &fds, NULL, NULL, NULL);
    if (result > 0)
    {
        if (FD_ISSET(netfd, &fds)) OAuthConnectBack(Ctx, netfd);
        else if (FD_ISSET(0, &fds))
        {
            S=STREAMFromFD(0);
            STREAMSetTimeout(S,0);
            Tempstr=STREAMReadLine(Tempstr, S);
            if ( (strncmp(Tempstr, "http:", 5)==0) || (strncmp(Tempstr, "https:", 6)==0) ) OAuthParseReply(Ctx, "application/x-www-form-urlencoded", Tempstr);
            else
            {
                StripTrailingWhitespace(Tempstr);
                Ctx->VerifyCode=HTTPQuote(Ctx->VerifyCode, Tempstr);
                SetVar(Ctx->Vars, "code", Ctx->VerifyCode);
            }
            STREAMDestroy(S);
        }
        OAuthFinalize(Ctx, URL);
    }

    DestroyString(Tempstr);
    close(netfd);

    return(TRUE);
}


//curl -X POST -d "client_id=CLIENT_ID_HERE&client_secret=CLIENT_SECRET_HERE&grant_type=password&username=YOUR_EMAIL&password=YOUR_PASSWORD" -Ss https://mastodon.social/oauth/token
int OAuthGrant(OAUTH *Ctx, const char *URL, const char *PostArgs)
{
    STREAM *S;
    char *Tempstr=NULL;
    int len, result=FALSE;

    Tempstr=MCopyStr(Tempstr,URL,"?",PostArgs,NULL);
    S=HTTPMethod("POST",URL,"application/x-www-form-urlencoded; charset=UTF-8",PostArgs,StrLen(PostArgs));
    if (S)
    {
        sleep(1);
        Tempstr=STREAMReadDocument(Tempstr, S);

        result=OAuthParseReply(Ctx, STREAMGetValue(S, "HTTP:Content-Type"), Tempstr);
        STREAMClose(S);
    }

    DestroyString(Tempstr);

    return(result);
}




int OAuthRefresh(OAUTH *Ctx, const char *URL)
{
    char *Tempstr=NULL, *Args=NULL;
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

    ptr=GetVar(Ctx->Vars, "client_id");
    if (StrValid(ptr))
    {
        Tempstr=HTTPQuote(Tempstr, ptr);
        Args=MCopyStr(Args,"client_id=",Tempstr,NULL);
    }

    ptr=GetVar(Ctx->Vars, "client_secret");
    if (StrValid(ptr))
    {
        Tempstr=HTTPQuote(Tempstr, ptr);
        Args=MCatStr(Args,"&client_secret=",Tempstr,NULL);
    }

    if (StrValid(Ctx->RefreshToken))
    {
        Tempstr=HTTPQuote(Tempstr, Ctx->RefreshToken);
        Args=MCatStr(Args,"&refresh_token=",Tempstr,NULL);
    }

    Args=MCatStr(Args,"&grant_type=refresh_token",NULL);

    result=OAuthGrant(Ctx, URL, Args);

    DestroyString(Tempstr);
    DestroyString(Args);
    return(result);
}



int OAuthFinalize(OAUTH *Ctx, const char *URL)
{
    char *Tempstr=NULL;
    int result;

    Tempstr=SubstituteVarsInString(Tempstr, Ctx->Stage2, Ctx->Vars,0);
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
    SetVar(Ctx->Vars, "url",URL);
    Tempstr=SubstituteVarsInString(Tempstr, Ctx->Stage1, Ctx->Vars, 0);

    if (StrLen(Tempstr))
    {
        if (Ctx->Flags & OAUTH_IMPLICIT) result=OAuthImplicit(Ctx, URL, Tempstr);
        else result=OAuthGrant(Ctx, URL, Tempstr);
    }

    if (StrLen(Ctx->VerifyTemplate))
    {
        Ctx->VerifyURL=SubstituteVarsInString(Ctx->VerifyURL, Ctx->VerifyTemplate, Ctx->Vars,0);
    }

    DestroyString(Tempstr);
    return(result);
}


void OAuthSetUserCreds(OAUTH *Ctx, const char *UserName, const char *Password)
{
    char *Tempstr=NULL;

    Tempstr=HTTPQuote(Tempstr, UserName);
    SetVar(Ctx->Vars, "username", Tempstr);
    Tempstr=HTTPQuote(Tempstr, Password);
    SetVar(Ctx->Vars, "password", Tempstr);

    DestroyString(Tempstr);
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
    char *Tempstr=NULL, *Token=NULL, *Name=NULL, *Path;
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

    S=STREAMOpen(Path,"r");
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


    DestroyString(Tempstr);
    DestroyString(Token);
    DestroyString(Name);
    DestroyString(Path);

    return(result);
}



int OAuthSave(OAUTH *Ctx, const char *Path)
{
    STREAM *S;
    const char *Fields[]= {"client_id","client_secret","access_token","refresh_token",NULL};
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

    DestroyString(Tempstr);

    return(TRUE);
}



const char *OAuthLookup(const char *Name, int Refresh)
{
    ListNode *Curr;
    OAUTH *OA;

    Curr=ListFindNamedItem(OAuthKeyChain, Name);
    if (Curr)
    {
        OA=(OAUTH *) Curr->Item;
        if (Refresh && StrValid(OA->RefreshToken))
        {
            OAuthRefresh(OA, OA->RefreshURL);
            OAuthSave(OA, "");
        }
        if (StrValid(OA->AccessToken)) return(OA->AccessToken);
    }

    return("");
}
