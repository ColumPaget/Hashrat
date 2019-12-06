#include "inet.h"
#include "Http.h"
#include "GeneralFunctions.h"
#include "Markup.h"

char *ExtractFromWebpage(char *RetStr, char *URL, char *ExtractStr, int MinLength)
{
    STREAM *S;
    char *Tempstr=NULL;
    const char *ptr;
    ListNode *Vars;

    Vars=ListCreate();

    S=HTTPGet(URL);
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr,S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            StripLeadingWhitespace(Tempstr);

            if (StrLen(Tempstr) >=MinLength)
            {
                if (! StrValid(ExtractStr)) RetStr=CopyStr(RetStr,Tempstr);
                else
                {
                    ExtractVarsFromString(Tempstr,ExtractStr,Vars);
                    ptr=GetVar(Vars,"extract_item");
                    if (StrValid(ptr)) RetStr=CopyStr(RetStr,ptr);
                }
            }
            Tempstr=STREAMReadLine(Tempstr,S);
        }
        STREAMClose(S);
    }

    ListDestroy(Vars,(LIST_ITEM_DESTROY_FUNC) Destroy);
    DestroyString(Tempstr);

    StripTrailingWhitespace(RetStr);
    StripLeadingWhitespace(RetStr);

    return(RetStr);
}


char *GetExternalIP(char *RetStr)
{
    RetStr=CopyStr(RetStr,"");


    if (! StrValid(RetStr)) RetStr=ExtractFromWebpage(RetStr,"http://api.ipify.org", "",4);
    if (! StrValid(RetStr)) RetStr=ExtractFromWebpage(RetStr,"http://myexternalip.com/raw", "",4);
    if (! StrValid(RetStr)) RetStr=ExtractFromWebpage(RetStr,"http://checkip.dyndns.org", "*Current IP Address: $(extract_item)</body>",4);
    if (! StrValid(RetStr)) RetStr=ExtractFromWebpage(RetStr,"http://ip.appspot.com/", "",4);

    return(RetStr);
}

#define IPInfo_API_KEY "1261fcbf647ea02c165aa3bfa66810f0be453d8a1c2e7f653c0666d4e7e205f0"

int IPInfoDBGeoLocate(char *IP, ListNode *Vars)
{
    STREAM *S=NULL;
    char *TagType=NULL, *TagData=NULL, *Tempstr=NULL, *Token=NULL;
    const char *DesiredTags[]= {"CountryCode","CountryName","City","RegionName","Latitude","Longitude","TimeZone",NULL};
    const char *ptr;
    int result=FALSE;

    if (! IsIPAddress(IP)) Token=CopyStr(Token,LookupHostIP(IP));
    else Token=CopyStr(Token,IP);

    Tempstr=MCopyStr(Tempstr,"http://api.ipinfodb.com/v2/ip_query.php?key=",IPInfo_API_KEY,"&ip=",Token,"&timezone=true",NULL);

    S=HTTPGet(Tempstr);
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr,S);
        while (Tempstr)
        {
            ptr=XMLGetTag(Tempstr,NULL,&TagType,&TagData);
            while (ptr)
            {
                if (MatchTokenFromList(TagType,DesiredTags,0) > -1)
                {
                    //we can't re-use 'TagType', we still need it
                    ptr=XMLGetTag(ptr,NULL,&Token,&TagData);
                    SetVar(Vars,TagType,TagData);
                    result=TRUE;
                }
                ptr=XMLGetTag(ptr,NULL,&TagType,&TagData);
            }
            Tempstr=STREAMReadLine(Tempstr,S);
        }
    }

    STREAMClose(S);

    DestroyString(Tempstr);
    DestroyString(Token);
    DestroyString(TagType);
    DestroyString(TagData);

    return(result);
}


int IPGeoLocate(const char *IP, ListNode *Vars)
{
    STREAM *S=NULL;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    const char *DesiredTags[]= {"CountryCode","CountryName","City","RegionName","Latitude","Longitude","TimeZone",NULL};
    int result=FALSE;

    if (! StrValid(IP)) return(FALSE);

    if (! IsIPAddress(IP)) Token=CopyStr(Token, LookupHostIP(IP));
    else Token=CopyStr(Token,IP);

    Tempstr=MCopyStr(Tempstr,"http://freegeoip.net/csv/",Token,NULL);

    S=HTTPGet(Tempstr);
    if (S)
    {
        STREAMSetTimeout(S,100);
        Tempstr=STREAMReadDocument(Tempstr,S);
        ptr=GetToken(Tempstr, ",", &Token,0); //IP
        ptr=GetToken(ptr, ",", &Token,0); //CountryCode
        strlwr(Token);
        SetVar(Vars,"CountryCode",Token);
        ptr=GetToken(ptr, ",", &Token,0); //Country name
        SetVar(Vars,"CountryName",Token);
        ptr=GetToken(ptr, ",", &Token,0); //Region Code
        ptr=GetToken(ptr, ",", &Token,0); //Region Name
        SetVar(Vars,"RegionName",Token);
        ptr=GetToken(ptr, ",", &Token,0); //City
        SetVar(Vars,"City",Token);
        STREAMClose(S);
        result=TRUE;
    }


    DestroyString(Tempstr);
    DestroyString(Token);

    return(result);
}
