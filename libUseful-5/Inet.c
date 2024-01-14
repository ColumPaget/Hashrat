#include "Inet.h"
#include "Http.h"
#include "GeneralFunctions.h"
#include "Markup.h"
#include "IPAddress.h"

char *ExtractFromWebpage(char *RetStr, const char *URL, const char *ExtractStr, int MinLength)
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
    const char *Services[]= {"https://api.my-ip.io/ip.txt", "http://api.ipify.org", "http://myexternalip.com/raw", "http://checkip.dyndns.org,*Current IP Address: $(extract_item)</body>", "https://ip.seeip.org/", "https://ipapi.co/ip", "https://ifconfig.me/ip", "http://extreme-ip-lookup.com/,\"query\" : \"$(extract_item)\"", "https://duckduckgo.com/?q=what%27s+my+ip&ia=answer,*\"Answer\":\"Your IP address is $(extract_item) ","https://www.ionos.co.uk/tools/ip-address,*Your IP is:</h2><div class=\"text-md-center text-lg-center heading-1 ml-md-0 ml-lg-0 mw-lg-10 mw-md-10 mx-md-auto mx-lg-auto pt-12\">$(extract_item)<", "http://my-ip-is.appspot.com/plain", NULL};

    unsigned int i, max, start;
    char *Token=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr,"");

    //count items in array. avoid sizeof as it's not consistent c/c++
    for (max=0; Services[max] !=NULL; max++);

    //pick a random start position, and to through all servers from that start
    start=(time(NULL) + rand()) % max;
    for (i=start; (i < max) && (! IsIPAddress(RetStr)); i++)
    {
        ptr=GetToken(Services[i], ",", &Token, 0);
        RetStr=ExtractFromWebpage(RetStr, Token, ptr, 4);
    }

    for (i=0; (i < start) && (! IsIPAddress(RetStr)); i++)
    {
        ptr=GetToken(Services[i], ",", &Token, 0);
        RetStr=ExtractFromWebpage(RetStr, Token, ptr, 4);
    }

    Destroy(Token);
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
