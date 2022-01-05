#include "Gemini.h"
#include "URL.h"

STREAM *GeminiOpen(const char *URL, const char *Config)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL;
    char *Proto=NULL, *Host=NULL, *PortStr=NULL;
    const char *ptr;

    ParseURL(URL, &Proto, &Host, &PortStr, NULL, NULL, NULL, NULL);
    if (! StrValid(PortStr)) PortStr=CopyStr(PortStr, "1965");

    Tempstr=MCopyStr(Tempstr, "tls://", Host, ":", PortStr, NULL);
    S=STREAMCreate();
    if (STREAMConnect(S, Tempstr, Config))
    {
        Tempstr=MCopyStr(Tempstr, URL, "\r\n", NULL);
        STREAMWriteLine(Tempstr, S);
        STREAMFlush(S);

        Tempstr=STREAMReadLine(Tempstr, S);
        ptr=GetToken(Tempstr, "\\S", &Token, 0);
        if (StrValid(Token))
        {
            STREAMSetValue(S, "gemini:StatusCode", Token);
            switch(*Token)
            {
            case '1':
                STREAMSetValue(S, "gemini:Query", ptr);
                break;

            case '2':
                STREAMSetValue(S, "gemini:ContentType", ptr);
                break;

            case '3':
                Token=ResolveURL(Token, URL, ptr);
                if (! STREAMConnect(S, Token, Config))
                {
                    STREAMClose(S);
                    S=NULL;
                }
                break;
            }
        }
    }

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(PortStr);
    Destroy(Proto);
    Destroy(Host);

    return(S);
}
