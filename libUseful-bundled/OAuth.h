/*
Copyright (c) 2015 Colum Paget <colums.projects@googlemail.com>
* SPDX-License-Identifier: GPL-3.0
*/


#ifndef LIBUSEFUL_OAUTH_H
#define LIBUSEFUL_OAUTH_H

/*
OH GOD, WHERE TO EVEN BEGIN?

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

The default 'auth' method tells a user to go to the website, and copy back a special code that the website will display to them. Google supports this system, but other websites don't. Some only support a method whereby you got to specific URL, login and grant app permissions, and then your webbrowser is redirected to a URL of your choice to send the authentication details. For most libUseful applications this URL will be local, something like 'http://127.0.0.1:8989'. Unfortunately you are often required to specify what this URL will be at registration, so the application can't just pick a random port. This method is handled like so:


Ctx=OAuthCreate("auth","gdrive", GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET,"https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v4/token");
SetVar(Ctx->Vars,"redirect_uri","http://localhost:8989/google.callback");
OAuthStage1(Ctx, "https://www.googleapis.com/oauth2/authorize");
printf("GOTO: %s \n",Ctx->VerifyURL);

OAuthListen(Ctx, 8989, "https://www.googleapis.com/oauth2/v4/token", 0);

The 'redirect_uri' set with 'SetVar' is used to tell the OAuth service to use our browser to connect back to port 8989 on the localhost. 

The "VerifyURL" generated by OAuthState1 is the URL the user must go to in their browseer in order to log in and give permission. If the application can launch a webrowser itself it can just do that with the VerifyURL passed to the browser on the command line, otherwise it will have to message the user to go to that URL as in our example. Once the user grants permission, one of two things can happen. Some sites offer the option of copying a special code back to the application (see OAUTH_STDIN discussion below) but most expect the browser to connect to the 'redirect_uri' ( in this case "http://localhost:8989/google.callback") and push the secret code/details that's needed to complete authorization. 

'OAuthListen' is used to handle receiving the code/details. In most OAuth flows the browser is going to connect to a port (in this case 8989) at the local host in order to send a code or other type of authorization message. It will then do the next stage of authentication at the supplied url. This is the method used in most desktop applications. Thus OAuthListen explicitly expects a connection on the local interface, at IP address 127.0.0.1. In some situations the authenticating browser website can't or won't connect to the authenticating application, but provides a code or a url containing the code, that can be pasted back to the application. In order to handle this scenario the OAUTH_STDIN flag can be used:

#define OAUTH_STDIN 2

OAuthListen(Ctx, 8989, "https://www.googleapis.com/oauth2/v4/token", OAUTH_STDIN);

In this scenario the app will listen for the appropriate data both on port 8989 and also from stdin. If only stdin is required, the port can be set to '0'.

For server-side apps there is a scenario where the browser and the application to be authenticated run on different hosts. Thus listening on interface 127.0.0.1 isn't sufficient. In these scenarios the alternative function 'int OAuthAwaitRedirect(OAUTH *Ctx, const char *RedirectURL, const char *URL, int Flags)' can be used. it Functions just like OAuthListen, except that a full url can be supplied, like so:

OAuthAwaitRedirect(Ctx, "https://myserver.somewhere.com:8989/oauth", "https://www.googleapis.com/oauth2/v4/token", OAUTH_STDIN);

If the port we are listening on is going to be connected over the internet, then it's usual to use https. This requires an SSL/TLS certificate and key to be supplied, like so:

Ctx=OAuthCreate("auth","gdrive", GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET,"https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v4/token");
SetVar(Ctx->Vars,"redirect_uri","http://localhost:8989/google.callback");
SetVar(Ctx->Vars,"SSL:CertFile","/etc/ssl/certs/myserver.cert");
SetVar(Ctx->Vars,"SSL:KeyFile","/etc/ssl/certs/myserver.key");
OAuthStage1(Ctx, "https://www.googleapis.com/oauth2/authorize");
printf("GOTO: %s \n",Ctx->VerifyURL);
OAuthAwaitRedirect(Ctx, "https://myserver.somewhere.dcom:8989/oauth", "https://www.googleapis.com/oauth2/v4/token", OAUTH_STDIN);


Auth flow does not include the cilent_id and client_secret in the arguments POST-ed in the request body. Instead it sends them as though they were a username and password using HTTP Basic authentication. For sites implementing auth flow, but where the entire details are included in the POST body rather than in the authentication headers, use the method "auth-body"

Ctx=OAuthCreate("auth-body","gdrive", GOOGLE_CLIENT_ID, GOOGLE_CLIENT_SECRET,"https://www.googleapis.com/auth/drive", "https://www.googleapis.com/oauth2/v4/token");



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


Implicit grant flow is supported under the flow name 'implicit', although this is little used.

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
    char *RefreshURL; //no longer used, kept for the time being for compatiblity with existing code. now use Var 'refresh_url'.
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

int OAuthRegister(OAUTH *Ctx, const char *URL);
int OAuthSave(OAUTH *Ctx, const char *Path);
int OAuthLoad(OAUTH *Ctx, const char *Name, const char *Path);

int OAuthRefresh(OAUTH *Ctx, const char *URL);

int OAuthStage1(OAUTH *Ctx, const char *URL);
int OAuthFinalize(OAUTH *Ctx, const char *URL);
void OAuthSetUserCreds(OAUTH *Ctx, const char *UserName, const char *Password);

int OAuthListen(OAUTH *Ctx, int Port, const char *URL, int Flags);
const char *OAuthLookup(const char *Name, int Refresh);

int OAuthAwaitRedirect(OAUTH *Ctx, const char *RedirectURL, const char *URL, int Flags);

#ifdef __cplusplus
}
#endif

#endif

