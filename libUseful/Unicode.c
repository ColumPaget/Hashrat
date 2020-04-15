#include "Unicode.h"

static int UnicodeLevel=0;

void UnicodeSetUTF8(int level)
{
    UnicodeLevel=level;
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



char *UnicodeStr(char *RetStr, int Code)
{
    char *Tempstr=NULL;

    if (UnicodeLevel == 0) return(AddCharToStr(RetStr, '?'));
    if (Code < 0x800)
    {
        Tempstr=FormatStr(Tempstr,"%c%c",128+64+((Code & 1984) >> 6), 128 + (Code & 63));
    }
    else
    {
        Tempstr=CopyStr(Tempstr, "?");
        if ((UnicodeLevel > 1) && (Code < 0x10000)) Tempstr=FormatStr(Tempstr,"%c%c%c", (Code >> 12) | 224, ((Code >> 6) & 63) | 128, (Code & 63) | 128);
    }
    RetStr=CatStr(RetStr,Tempstr);
    DestroyString(Tempstr);

    return(RetStr);
}


