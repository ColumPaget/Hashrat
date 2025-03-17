#include "Markup.h"


//this part just parses the tag name
static const char *XMLParseTagName(const char *Input, char **Namespace, char **TagType)
{
    const char *ptr, *tptr;
    int len=0, InTag;

    ptr=Input;

    //it's permissible to have whitespace after the tag open, like < p >
    while (isspace(*ptr)) ptr++;


    len=0;
    //we must be in an XML tag for this function to be called
    InTag=TRUE;

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
                *TagType=AddCharToBuffer(*TagType, len, *ptr);
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
    StrTrunc(*TagType, len);
    strlwr(*TagType);

    return(ptr);
}



// this function parses either
// 1) data within a tag, that comes after the tag type name
// 2) data (text) between tags
static const char *XMLParseTagData(const char *Input, int TagHasName, char **TagData)
{
    int len=0;
    int InTag=TRUE; //as this part parses data in or out of a tag, InTag always starts as TRUE, even if parsing data between tags
    const char *ptr, *tptr;

    ptr=Input;
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
                    if (TagData) *TagData=AddCharToBuffer(*TagData,len,*ptr);
                    len++;
                    ptr++;
                }
            }
            if (TagData) *TagData=AddCharToBuffer(*TagData,len,*ptr);
            len++;
            ptr++;
            break;

        default:
            if (TagData) *TagData=AddCharToBuffer(*TagData,len,*ptr);
            len++;
            ptr++;
            break;
        }

    }

//If 'TagHasName' is set, then we are reading data *inside a tag*. In that
//case the tag can end with '/', like <br/>. If that's the case then
//strip any '/' and re-add it as ' /' to prevent it becoming
//confused with actual TagData
    if (TagHasName && TagData)
    {
        tptr=*TagData;
        if ((len > 0) && (tptr[len-1]=='/'))
        {
            len--;
            StrTrunc(*TagData, len);
            *TagData=CatStr(*TagData, " /");
        }

        if (TagHasName) StripLeadingWhitespace(*TagData);
    }

    return(ptr);
}

const char *XMLGetTag(const char *Input, char **Namespace, char **TagType, char **TagData)
{
    int TagHasName=FALSE;
    char *LocalTagType=NULL;
    const char *ptr;

    if (! StrValid(Input)) return(NULL);
    ptr=Input;

    if (TagType) *TagType=CopyStr(*TagType,"");
    if (TagData) *TagData=CopyStr(*TagData,"");

    if (*ptr=='<')
    {
        ptr++;
        TagHasName=TRUE;
        //parsing the tag type is too important to handle if user passes NULL for TagType
        //as we need it for parsing namespace if the user does not pass NULL for that
        //uso we use a local tag type variable
        ptr=XMLParseTagName(ptr, Namespace, &LocalTagType);
        if (TagType) *TagType=CopyStr(*TagType, LocalTagType);
    }


    ptr=XMLParseTagData(ptr, TagHasName, TagData);

    Destroy(LocalTagType);

    return(ptr);
}


int XMLIsEndTag(const char *TagType, const char *TagData)
{
    const char *ptr;
    int len;

    if (TagType && (*TagType=='/')) return(TRUE);

    if (TagData)
    {
        len=StrLen(TagData);
        ptr=TagData + (StrLen(TagData) - 1);
        if (*ptr=='/') return(TRUE);
    }

    return(FALSE);
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


