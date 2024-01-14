#include "HttpUtil.h"
#include "Encodings.h"


void HTTPDecodeBasicAuth(const char *Auth, char **UserName, char **Password)
{
    char *Tempstr=NULL;
    const char *ptr;

    Tempstr=DecodeToText(Tempstr, Auth, ENCODE_BASE64);
    ptr=GetToken(Tempstr, ":", UserName, 0);
    ptr=GetToken(ptr, ":", Password, 0);

    Destroy(Tempstr);
}


char *HTTPUnQuote(char *RetBuff, const char *Str)
{
    char *RetStr=NULL, *Token=NULL;
    const char *ptr;
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

    StrLenCacheAdd(RetStr, olen);

    DestroyString(Token);
    return(RetStr);
}


char *HTTPQuoteChars(char *RetBuff, const char *Str, const char *CharList)
{
    char *RetStr=NULL, *Token=NULL;
    const char *ptr;
    int olen=0, ilen;

    RetStr=CopyStr(RetStr,"");
    ilen=StrLen(Str);

    for (ptr=Str; ptr < (Str+ilen); ptr++)
    {
        if (strchr(CharList,*ptr))
        {
            // replacing ' ' with '+' should work, but some servers seem to no longer support it
            /*
            	if (*ptr==' ')
            	{
            RetStr=AddCharToBuffer(RetStr,olen,'+');
            	olen++;
            	}
            	else
            */
            {
                Token=FormatStr(Token,"%%%02X",*ptr);
                StrLenCacheAdd(RetStr, olen);
                RetStr=CatStr(RetStr,Token);
                olen+=StrLen(Token);
            }
        }
        else
        {
            RetStr=AddCharToBuffer(RetStr,olen,*ptr);
            olen++;
        }
    }


    RetStr[olen]='\0';
    StrLenCacheAdd(RetStr, olen);

    DestroyString(Token);
    return(RetStr);
}



char *HTTPQuote(char *RetBuff, const char *Str)
{
    return(HTTPQuoteChars(RetBuff, Str, " \t\r\n\"#%()[]{}<>?&!,+':;/@"));
}


