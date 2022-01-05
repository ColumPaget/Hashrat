#include "Unicode.h"

static int GlobalUnicodeLevel=0;
static ListNode *UnicodeNamesCache=NULL;

void UnicodeSetUTF8(int level)
{
    GlobalUnicodeLevel=level;
}


char *StrAddUnicodeChar(char *RetStr, int uchar)
{
    switch (uchar)
    {
    //non-breaking space
    case 0x00a0:
        RetStr=AddCharToStr(RetStr,' ');
        break;
        break;


    //en-dash and em-dash
    case 0x2010:
    case 0x2011:
    case 0x2012:
    case 0x2013:
    case 0x2014:
    case 0x2015:
        RetStr=AddCharToStr(RetStr,'-');
        break;

    //2019 is apostrophe in unicode. presumably it gives you as special, pretty apostrophe, but it causes hell with
    //straight ansi terminals, so we remap it here
    case 0x2018:
    case 0x2019:
        RetStr=AddCharToStr(RetStr,'\'');
        break;


    case 0x201a:
        RetStr=AddCharToStr(RetStr,',');
        break;


    case 0x201b:
        RetStr=AddCharToStr(RetStr,'`');
        break;


    //left and right double quote. We simplify down to just double quote
    case 0x201c:
    case 0x201d:
    case 0x201e:
        RetStr=AddCharToStr(RetStr,'"');
        break;

    case 0x2024:
        RetStr=CatStr(RetStr,".");
        break;

    case 0x2025:
        RetStr=CatStr(RetStr,"..");
        break;


    case 0x2026:
        RetStr=CatStr(RetStr,"...");
        break;

    case 0x2039:
        RetStr=CatStr(RetStr,"<");
        break;

    case 0x203A:
        RetStr=CatStr(RetStr,">");
        break;

    case 0x2044:
        RetStr=AddCharToStr(RetStr,'/');
        break;

    case 0x204e:
    case 0x2055:
        RetStr=AddCharToStr(RetStr,'*');
        break;


    default:
        RetStr=UnicodeStr(RetStr, uchar);
        break;
    }

    return(RetStr);
}


unsigned int UnicodeDecode(const char **ptr)
{
    unsigned int val=0;

//unicode bit pattern 110
    if (((**ptr) & 224) == 192)
    {
        val=((**ptr) & 31) << 6;
        if (ptr_incr(ptr, 1) != 1) return(0);
        val |= (**ptr) & 127;
    }
    else if (((**ptr) & 224) == 224)
    {
        val=((**ptr) & 31) << 12;
        if (ptr_incr(ptr, 1) != 1) return(0);
        val |= ((**ptr) & 31) << 6;
        if (ptr_incr(ptr, 1) != 1) return(0);
        val |= (**ptr) & 127;
    }

    return(val);
}



char *UnicodeEncodeChar(char *RetStr, int UnicodeLevel, int Code)
{
    char *Tempstr=NULL;

    if ((Code==0) || (UnicodeLevel == 0)) return(AddCharToStr(RetStr, '?'));
    if (Code < 0x800)
    {
        Tempstr=FormatStr(Tempstr,"%c%c",128+64+((Code & 1984) >> 6), 128 + (Code & 63));
    }
    else if (Code < 0x10000)
    {
        Tempstr=CopyStr(Tempstr, "?");
        if ((UnicodeLevel > 1) && (Code < 0x10000)) Tempstr=FormatStr(Tempstr,"%c%c%c", (Code >> 12) | 224, ((Code >> 6) & 63) | 128, (Code & 63) | 128);
    }
    else
    {
        Tempstr=CopyStr(Tempstr, "?");
        if ((UnicodeLevel > 2) && (Code < 0x1FFFF)) Tempstr=FormatStr(Tempstr,"%c%c%c%c", (Code >> 18) | 240, ((Code >> 12) & 63) | 128, ((Code >> 6) & 63) | 128, (Code & 63) | 128);
    }

    RetStr=CatStr(RetStr,Tempstr);
    DestroyString(Tempstr);

    return(RetStr);
}


char *UnicodeStr(char *RetStr, int Code)
{
    return(UnicodeEncodeChar(RetStr, GlobalUnicodeLevel, Code));
}



char *UnicodeStrFromNameAtLevel(char *RetStr, int UnicodeLevel, const char *Name)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;
    ListNode *Node;
    int code=0;


    if (! UnicodeNamesCache) UnicodeNamesCache=ListCreate();
    Node=ListFindNamedItem(UnicodeNamesCache, Name);
    if (Node)
    {
        code=strtol((const char *) Node->Item, NULL, 16);
        return(UnicodeStr(RetStr, code));
    }

    Tempstr=CopyStr(Tempstr, LibUsefulGetValue("Unicode:NamesFile"));
    if (StrValid(Tempstr)) Tempstr=CopyStr(Tempstr, getenv("UNICODE_NAMES_FILE"));
    if (! StrValid(Tempstr)) Tempstr=CopyStr(Tempstr, "/etc/unicode-names.conf");

    S=STREAMOpen(Tempstr, "r");
    if (S)
    {
        Tempstr=STREAMReadLine(Tempstr, S);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            ptr=GetToken(Tempstr, "\\S", &Token, 0);
            if (strcasecmp(Token, Name)==0)
            {
                SetVar(UnicodeNamesCache, Name, ptr);
                code=strtol(ptr, NULL, 16);
                break;
            }
            Tempstr=STREAMReadLine(Tempstr, S);
        }
        STREAMClose(S);
    }

    Destroy(Tempstr);
    Destroy(Token);

    return(UnicodeEncodeChar(RetStr, UnicodeLevel, code));
}



char *UnicodeStrFromName(char *RetStr, const char *Name)
{
    return(UnicodeStrFromNameAtLevel(RetStr, GlobalUnicodeLevel, Name));
}
