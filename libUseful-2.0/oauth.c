#include "oauth.h"
#include "http.h"

void ParseTagData(char *TagName,char *TagData,char **RType,char **RName,char **RValue)
{
char *Name=NULL, *Value=NULL, *ptr;

ptr=GetNameValuePair(TagData," ","=",&Name,&Value);
while (ptr)
{
if (strcasecmp(Name,"type")==0) *RType=HtmlDeQuote(*RType,Value);
if (strcasecmp(Name,"name")==0) *RName=HtmlDeQuote(*RName,Value);
if (strcasecmp(Name,"value")==0) *RValue=HtmlDeQuote(*RValue,Value);
if (strcasecmp(Name,"method")==0) *RType=HtmlDeQuote(*RType,Value);
if (strcasecmp(Name,"action")==0) *RValue=HtmlDeQuote(*RValue,Value);

ptr=GetNameValuePair(ptr," ","=",&Name,&Value);
}

DestroyString(Name);
DestroyString(Value);
}


void OAuthParseForm(char *HTML, char *SubmitType, char **SubmitURL, ListNode *HiddenVals, ListNode *QueryVals)
{
char *TagName=NULL, *TagData=NULL, *Type=NULL, *Name=NULL, *Value=NULL, *ptr;

ptr=XMLGetTag(HTML,NULL,&TagName,&TagData);
while (ptr)
{
if (strcmp(TagName,"input")==0)	
{
	ParseTagData(TagName,TagData,&Type,&Name,&Value);

  if (strcasecmp(Type,"hidden")==0) SetVar(HiddenVals,Name,Value);
  if (strcasecmp(Type,"submit")==0) SetVar(HiddenVals,Name,Value);
  if (strcasecmp(Type,"text")==0) SetVar(QueryVals,Name,Value);
  if (strcasecmp(Type,"password")==0) SetVar(QueryVals,Name,Value);
}

if (strcmp(TagName,"form")==0)	
{
	ParseTagData(TagName,TagData,&Type,&Name,SubmitURL);
}


ptr=XMLGetTag(ptr,NULL,&TagName,&TagData);
}

DestroyString(TagName);
DestroyString(TagData);
DestroyString(Type);
DestroyString(Name);
DestroyString(Value);
}




void OAuthParseJSON(char *JSON, ListNode *Vars)
{
char *ptr, *ptr2, *Token=NULL, *Name=NULL, *Value=NULL;

StripLeadingWhitespace(JSON);
StripTrailingWhitespace(JSON);
ptr=JSON+StrLen(JSON)-1;
if (*ptr=='}') *ptr='\0';
ptr=JSON;
if (*ptr=='{') ptr++;
ptr=GetToken(ptr,",",&Token,0);
while (ptr)
{
printf("TOK: %s\n",Token);
ptr2=GetToken(Token,":",&Name,0);
StripTrailingWhitespace(Name);
StripQuotes(Name);
ptr2=GetToken(ptr2,":",&Value,GETTOKEN_QUOTES);
StripLeadingWhitespace(Value);
StripTrailingWhitespace(Value);
StripQuotes(Value);
printf("JSON: %s=%s\n",Name,Value);
SetVar(Vars,Name,Value);
ptr=GetToken(ptr,",",&Token,0);
}


DestroyString(Name);
DestroyString(Value);
DestroyString(Token);
}



void OAuthDeviceLogin(char *LoginURL, char *ClientID, char *Scope, char **DeviceCode, char **UserCode, char **NextURL)
{
char *Tempstr=NULL, *Encode=NULL;
ListNode *Vars=NULL;
STREAM *S;

Vars=ListCreate();

Encode=HTTPQuote(Encode,ClientID);
Tempstr=MCopyStr(Tempstr,LoginURL,"?client_id=",Encode,NULL);
Encode=HTTPQuote(Encode,Scope);
Tempstr=MCatStr(Tempstr,"&scope=",Encode,NULL);

S=HTTPMethod("POST",Tempstr,"","");

if (S)
{
Encode=CopyStr(Encode,"");
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
StripTrailingWhitespace(Tempstr);
Encode=CatStr(Encode,Tempstr);
Tempstr=STREAMReadLine(Tempstr,S);
}

OAuthParseJSON(Encode, Vars);

*NextURL=CopyStr(*NextURL,GetVar(Vars,"verification_url"));
*DeviceCode=CopyStr(*DeviceCode,GetVar(Vars,"device_code"));
*UserCode=CopyStr(*UserCode,GetVar(Vars,"user_code"));
}


ListDestroy(Vars,DestroyString);
DestroyString(Tempstr);
DestroyString(Encode);
STREAMClose(S);
}



void OAuthDeviceGetAccessToken(char *TokenURL, char *ClientID, char *ClientSecret, char *DeviceCode, char **AccessToken, char **RefreshToken)
{
char *Tempstr=NULL, *Encode=NULL;
ListNode *Vars=NULL;
STREAM *S;

Vars=ListCreate();

Encode=HTTPQuote(Encode,ClientID);
Tempstr=MCopyStr(Tempstr,TokenURL,"?client_id=",Encode,NULL);
Encode=HTTPQuote(Encode,ClientSecret);
Tempstr=MCatStr(Tempstr,"&client_secret=",Encode,NULL);
Tempstr=MCatStr(Tempstr,"&code=",DeviceCode,NULL);
Tempstr=MCatStr(Tempstr,"&grant_type=","http://oauth.net/grant_type/device/1.0",NULL);

S=HTTPMethod("POST",Tempstr,"","");
if (S)
{
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
StripTrailingWhitespace(Tempstr);
printf("OA: %s\n",Tempstr);
OAuthParseJSON(Tempstr, Vars);
Tempstr=STREAMReadLine(Tempstr,S);
}
}

*AccessToken=CopyStr(*AccessToken,GetVar(Vars,"access_token"));
*RefreshToken=CopyStr(*RefreshToken,GetVar(Vars,"refresh_token"));

ListDestroy(Vars,DestroyString);
DestroyString(Tempstr);
DestroyString(Encode);
}



void OAuthDeviceRefreshToken(char *TokenURL, char *ClientID, char *ClientSecret, char *RefreshToken, char **AccessToken)
{
char *Tempstr=NULL, *Encode=NULL;
ListNode *Vars=NULL;
STREAM *S;

Vars=ListCreate();

Tempstr=MCopyStr(Tempstr,TokenURL,"?client_id=",ClientID,NULL);
Tempstr=MCatStr(Tempstr,"&client_secret=",ClientSecret,NULL);
Tempstr=MCatStr(Tempstr,"&refresh_token=",RefreshToken,NULL);
Tempstr=MCatStr(Tempstr,"&grant_type=","refresh_token",NULL);

S=HTTPMethod("POST",Tempstr,"","");
if (S)
{
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
StripTrailingWhitespace(Tempstr);
OAuthParseJSON(Tempstr, Vars);

Tempstr=STREAMReadLine(Tempstr,S);
}
} 

*AccessToken=CopyStr(*AccessToken,GetVar(Vars,"access_token"));
//*RefreshToken=CopyStr(*RefreshToken,GetVar(Vars,"refresh_token"));

ListDestroy(Vars,DestroyString);
DestroyString(Tempstr);
DestroyString(Encode);
}




void OAuthInstalledAppURL(char *LoginURL, char *ClientID, char *Scope, char *RedirectURL, char **NextURL)
{
char *Encode=NULL;

Encode=HTTPQuote(Encode,ClientID);
*NextURL=MCopyStr(*NextURL,LoginURL,"?response_type=code&redirect_uri=",RedirectURL,"&client_id=",Encode,NULL);
Encode=HTTPQuote(Encode,Scope);
*NextURL=MCatStr(*NextURL,"&scope=",Encode,NULL);


DestroyString(Encode);
}


void OAuthInstalledAppGetAccessToken(char *TokenURL, char *ClientID, char *ClientSecret, char *AuthCode, char *RedirectURL, char **AccessToken, char **RefreshToken)
{
char *Tempstr=NULL, *Encode=NULL;
ListNode *Vars=NULL;
STREAM *S;

Vars=ListCreate();

Tempstr=MCopyStr(Tempstr,TokenURL,"?client_id=",ClientID,NULL);
Tempstr=MCatStr(Tempstr,"&client_secret=",ClientSecret,NULL);
Tempstr=MCatStr(Tempstr,"&code=",AuthCode,NULL);
Tempstr=MCatStr(Tempstr,"&redirect_uri=",RedirectURL,NULL);
Tempstr=MCatStr(Tempstr,"&grant_type=","authorization_code",NULL);

S=HTTPMethod("POST",Tempstr,"","");
if (S)
{
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
StripTrailingWhitespace(Tempstr);
printf("OA: %s\n",Tempstr);
OAuthParseJSON(Tempstr, Vars);
Tempstr=STREAMReadLine(Tempstr,S);
}
}

*AccessToken=CopyStr(*AccessToken,GetVar(Vars,"access_token"));
*RefreshToken=CopyStr(*RefreshToken,GetVar(Vars,"refresh_token"));

ListDestroy(Vars,DestroyString);
DestroyString(Tempstr);
DestroyString(Encode);
}


