#include "Markup.h"

const char *XMLGetTag(const char *Input, char **Namespace, char **TagType, char **TagData)
{
    const char *ptr, *tptr;
    char *wptr;
    int len=0, InTag=FALSE, TagHasName=FALSE;

    if (! StrValid(Input)) return(NULL);
    ptr=Input;

//This ensures we are never dealing with nulls
    if (! *TagType) *TagType=CopyStr(*TagType,"");
    if (! *TagData) *TagData=CopyStr(*TagData,"");

    if (*ptr=='<')
    {
        ptr++;
        while (isspace(*ptr)) ptr++;

        len=0;
        InTag=TRUE;
        TagHasName=TRUE;

        //if we start with a slash tag, then add that to the tag name
        if (*ptr=='/')
        {
            *TagType=AddCharToBuffer(*TagType,len,*ptr);
            len++;
            ptr++;
        }

        while (InTag)
        {
            switch (*ptr)
            {
            //These all cause us to end. NONE OF THEM REQUIRE ptr++
            //ptr++ will happen when we read tag data
            case '>':
            case '\0':
            case ' ':
            case '	':
            case '\n':
                InTag=FALSE;
                break;

            //If a namespace return value is supplied, break the name up into namespace and
            //tag. Otherwise don't
            case ':':
                if (Namespace)
                {
                    tptr=*TagType;
                    if (*tptr=='/')
                    {
                        tptr++;
                        len=1;
                    }
                    else len=0;
                    *Namespace=CopyStr(*Namespace,tptr);
                }
                else
                {
                    *TagType=AddCharToBuffer(*TagType,len,*ptr);
                    len++;
                }
                ptr++;

                break;



            case '\r':
                ptr++;
                break;

            default:
                *TagType=AddCharToBuffer(*TagType,len,*ptr);
                len++;
                ptr++;
                break;
            }

        }
    }

    StrTrunc(*TagType, len);
    while (isspace(*ptr)) ptr++;


    len=0;
    InTag=TRUE;
    while (InTag)
    {
        switch (*ptr)
        {
        //End of tag, skip '>' and fall through
        case '>':
            ptr++;

        //Start of next tag or end of text
        case '<':
        case '\0':
            InTag=FALSE;
            break;

        //Quotes!
        case '\'':
        case '\"':

            //Somewhat ugly code. If we're dealing with an actual tag, then TagHasName
            //will be set. This means we're dealing with data within a tag and quotes mean something.
            //Otherwise we are dealing with text outside of tags and we just add the char to
            //TagData and continue
            if (TagHasName)
            {
                tptr=ptr;
                while (*tptr != *ptr)
                {
                    *TagData=AddCharToBuffer(*TagData,len,*ptr);
                    len++;
                    ptr++;
                }
            }
            *TagData=AddCharToBuffer(*TagData,len,*ptr);
            len++;
            ptr++;
            break;

        default:
            *TagData=AddCharToBuffer(*TagData,len,*ptr);
            len++;
            ptr++;
            break;
        }

    }

//End of Parse TagData. Strip any '/'
    wptr=*TagData;
    if ((len > 0) && (wptr[len-1]=='/')) len--;
    wptr[len]='\0';
    StrLenCacheAdd(*TagData, len);

    strlwr(*TagType);
    while (isspace(*ptr)) ptr++;

    return(ptr);
}





char *HTMLQuote(char *RetBuff, const char *Str)
{
    char *RetStr=NULL, *Token=NULL;
    const char *ptr;
    int len;

    RetStr=CopyStr(RetStr,"");
    len=StrLen(Str);

    for (ptr=Str; ptr < (Str+len); ptr++)
    {

        switch (*ptr)
        {
        case '&':
            RetStr=CatStr(RetStr,"&amp;");
            break;
        case '<':
            RetStr=CatStr(RetStr,"&lt;");
            break;
        case '>':
            RetStr=CatStr(RetStr,"&gt;");
            break;

        default:
            RetStr=AddCharToStr(RetStr,*ptr);
            break;
        }

    }

    DestroyString(Token);
    return(RetStr);
}




char *HTMLUnQuote(char *RetStr, const char *Data)
{
    char *Output=NULL, *Token=NULL;
    const char *ptr;
    int len=0;

    Output=CopyStr(RetStr,Output);
    ptr=Data;

    while (ptr && (*ptr != '\0'))
    {
        if (*ptr=='&')
        {
            ptr++;
            ptr=GetToken(ptr,";",&Token,0);

            if (strncmp(Token,"#x", 2)==0)
            {
                Output=AddCharToBuffer(Output,len,strtol(Token+2,NULL,16));
                len++;
            }
            else if (*Token=='#')
            {
                Output=AddCharToBuffer(Output,len,strtol(Token+1,NULL,10));
                len++;
            }
            else if (strcmp(Token,"amp")==0)
            {
                Output=AddCharToBuffer(Output,len,'&');
                len++;
            }
            else if (strcmp(Token,"lt")==0)
            {
                Output=AddCharToBuffer(Output,len,'<');
                len++;
            }
            else if (strcmp(Token,"gt")==0)
            {
                Output=AddCharToBuffer(Output,len,'>');
                len++;
            }
            else if (strcmp(Token,"quot")==0)
            {
                Output=AddCharToBuffer(Output,len,'"');
                len++;
            }
            else if (strcmp(Token,"apos")==0)
            {
                Output=AddCharToBuffer(Output,len,'\'');
                len++;
            }
            else if (strcmp(Token,"tilde")==0)
            {
                Output=AddCharToBuffer(Output,len,'~');
                len++;
            }
            else if (strcmp(Token,"circ")==0)
            {
                Output=AddCharToBuffer(Output,len,'^');
                len++;
            }




        }
        else
        {
            Output=AddCharToBuffer(Output,len,*ptr);
            len++;
            ptr++;
        }
    }

    StrLenCacheAdd(Output, len);
    DestroyString(Token);

    return(Output);
}


