/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_OAUTH_H
#define LIBUSEFUL_OAUTH_H

/*
GOD, WHERE TO EVEN BEGIN?

OAuth is not really a standard, most sites seem to implement it differently. These functions help with
implementing OAuth 2.0 authentication. It's complicated. More than it should be.

OAuth works via a system of granting 'access tokens' to an application. In order to grant such a token
the user normally has to log in via a browser and confirm the grant. This has the advantage of never 
storing the users real username and password, all that's stored is the access token. Some sites revoke
access tokens after a period of time, but they supply a 'refresh token' in addition to the access token.
The application can use the refresh token to get a fresh access token without user involvement.

In order for an application to identify itself it usually uses an API key, or a 'client id' and 'client secret'.

There's a lot of different oauth implementations, called 'flows'. libUseful knows of several 
different flow types, more will likely be added in future. The most common flow (e.g. used by google)
is called 'auth flow' and you use it like this:


OAUTH *Ctx;
char *Tempstr=NULL;

Ctx=OAuthCreate("auth","gdrive", GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET,"https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v4/token");
OAuthStage1(Ctx, "https://accounts.google.com/o/oauth2/v2/auth");
printf("GOTO: %s and type in code %s\n",Ctx->VerifyURL, Ctx->VerifyCode);
Tempstr=STREAMReadLine(Tempstr, StdIO);
StripTrailingWhitespace(Tempstr);
SetVar(Ctx->Vars, "code", Tempstr);
OAuthFinalize(Ctx, "https://www.googleapis.com/oauth2/v4/token");

printf("AccessToken: %s RefreshToken: %s\n",Ctx->AccessToken, Ctx->RefreshToken);



OAuthCreate sets up an OAuth context. The first argument is the flow type, the second argument is a unique name for this oauth session, then come the client-id and client-secret which will have been provided by the website when you registered the application. The forth argument is what's known as a 'scope', google is unusual in sometimes using URLs as scopes, normally it's a word like 'basic' or 'advanced'. The scope defines what subset of permissions you want to use on the site. Finally there's the 'refresh URL', which is a URL to connect to to exchange a refresh token for another access token. Note that the URLs supplied here dont include arguments, as these are auto-configured to the flow type.

The first step of the process, 'OAuthStage1' results in a 'VerifyURL' that the user must go to and type in a 'VerifyCode'. They will then be asked to acknowledge that they want to grant access to this application. If they grant access then they'll be given another code which they then paste back into the application. The application then uses this code to finalize the transaction and get an access token and perhaps a refresh token too (depending on the site's oauth implementation).

To use an oauth token with libUseful streams you have to supply an extra argument to the URL. For example

S=STREAMOpen("https://www.googleapis.com/drive/v2/about oauth=gdrive","");

or

S=HTTPMethod("POST", "https://www.googleapis.com/drive/v2/about oauth=gdrive","text/plain", myText, StrLen(myText));

The 'oauth=gdrive' argument specifies authentication using the session we created with the name 'gdrive' above.

an OAUTH object contains a member called 'SavePath', and this holds the path to a file that the tokens are written to. By default this file is named '.oauth.creds' and is in the users home directory. Unfortunately these details are not automaticaly loaded, so we have to user 'OAuthLoad' to load them, like so:


Ctx=OAuthCreate("auth","gdrive", GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET,"https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v4/token");
OAuthLoad(Ctx, "");

The second argument to OAuthLoad is a path that overrides Ctx->SavePath. If this argument is left blank the value of Ctx->SavePath is used. 

This means that in normal usage, after the initial getting the access token, you just call OAuthCreate and OAuthLoad, and everything should update automatically. You should only have to re-authenticate if your credential file gets deleted or something.

The default 'auth' method tells a user to go to the website, and copy back a special code that the website will display to them. Google supports this system, but other websites don't. Some only support a method whereby you got to specific URL, login and grant app permissions, and then your webbrowser is redirected to a URL of your choice to send the authentication details. For most libUseful applications this URL will be local, something like 'http://127.0.0.1:1111'. Unfortunately you are often required to specify what this URL will be at registration, so the application can't just pick a random port. This method is handled like so:


Ctx=OAuthCreate("auth","gdrive", GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET,"https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v4/token");
SetVar(Ctx->Vars,"redirect_uri","http://localhost:8989/google.callback");
OAuthStage1(Ctx, "https://www.deviantart.com/oauth2/authorize");
printf("GOTO: %s \n",Ctx->VerifyURL);

OAuthListen(Ctx, 8989, "https://www.deviantart.com/oauth2/token", 0);



The user is sent to the VerifyURL, but in this method they won't be asked to enter a code. If the application can launch a webrowser itself it can just do that with the VerifyURL passed to the browser on the command line, and not have to tell the user to do so. The user grants permission in the browser, and the browser connects to the 'redirect_uri' of "http://localhost:8989/google.callback" and pushes the secret code that's needed to complete authorization. 





Some sites have alternative oauth flows. getpocket.com has it's own system. libUseful can interact with it like so:

Ctx=OAuthCreate("getpocket.com","pocket", POCKET_ID, "","","");

OAuthStage1(Ctx, "https://getpocket.com/v3/oauth/request");
printf("GOTO: %s and type in code %s\n",Ctx->VerifyURL, Ctx->VerifyCode);

read(0,&result, 1);
OAuthFinalize(Ctx, "https://getpocket.com/v3/oauth/authorize");
}


Since libUseful-4.71 PKCE flow is supported, which is basically the same as auth-flow, but with some extra cryptographic elements that are handled internally by libUseful

    Ctx=OAuthCreate("pkce", FS->URL, DROPBOX_CLIENT_ID, "", "", "https://www.dropbox.com/oauth2/token");
    if (! OAuthLoad(Ctx, FS->URL, ""))
    {
        SetVar(Ctx->Vars, "redirect_uri", ""); //no redirect_uri used in dropbox authentication
        OAuthStage1(Ctx, "https://www.dropbox.com/oauth2/authorize");
        printf("GOTO: %s\n",Ctx->VerifyURL);
        printf("Login and/or grant access, then cut and past the access code back to this program.\n\nAccess Code: ");
        fflush(NULL);

        Tempstr=SetStrLen(Tempstr,1024);
        read(0,Tempstr,1024);
        StripTrailingWhitespace(Tempstr);
        SetVar(Ctx->Vars, "code", Tempstr);
        OAuthFinalize(Ctx, "https://api.dropbox.com/oauth2/token");
    } 
          
    printf("AccessToken: %s RefreshToken: %s\n",Ctx->AccessToken, Ctx->RefreshToken);


*/


#include "includes.h"

#define OAUTH_IMPLICIT 1
#define OAUTH_STDIN 2

typedef struct
{
    int Flags;
    char *Name;
    char *Stage1;
    char *Stage2;
    char *VerifyTemplate;
    char *AccessToken;
    char *RefreshToken;
    char *RefreshURL;
    char *VerifyURL;
    char *VerifyCode;
    char *Creds;
    char *SavePath;
    ListNode *Vars;
} OAUTH;


#ifdef __cplusplus
extern "C" {
#endif


// add template for a new/custom oauth type
void AddOAuthType(const char *Name, const char *Stage1Args, const char *Stage2Args, const char *VerifyTemplate);


OAUTH *OAuthCreate(const char *Type, const char *Name, const char *ClientID, const char *ClientSecret, const char *Scopes, const char *RefreshURL);
void OAuthDestroy(void *p_OAUTH);

int OAuthParseReply(OAUTH *Ctx, const char *ContentType, const char *Reply);

int OAuthRegister(OAUTH *Ctx, const char *URL);
int OAuthSave(OAUTH *Ctx, const char *Path);
int OAuthLoad(OAUTH *Ctx, const char *Name, const char *Path);

int OAuthRefresh(OAUTH *Ctx, const char *URL);

int OAuthStage1(OAUTH *Ctx, const char *URL);
int OAuthFinalize(OAUTH *Ctx, const char *URL);
void OAuthSetUserCreds(OAUTH *Ctx, const char *UserName, const char *Password);

int OAuthConnectBack(OAUTH *Ctx, int sock);
int OAuthListen(OAUTH *Ctx, int Port, const char *URL, int Flags);
const char *OAuthLookup(const char *Name, int Refresh);

#ifdef __cplusplus
}
#endif

#endif

