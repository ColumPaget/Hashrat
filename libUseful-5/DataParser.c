#include "DataParser.h"


//Gets recursively called
const char *ParserParseItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel);
char *ParserExportItems(char *RetStr, int Type, int Indent, PARSER *P);

PARSER *ParserCreate()
{
    return(ListCreate());
}

void ParserItemsDestroy(ListNode *Items)
{
    ListNode *Curr;

    Curr=ListGetNext(Items);
    while (Curr)
    {
        if (Curr->ItemType==ITEM_ARRAY) ParserItemsDestroy((ListNode *) Curr->Item);
        else if (Curr->ItemType==ITEM_ENTITY) ParserItemsDestroy((ListNode *) Curr->Item);
        else DestroyString(Curr->Item);
        Curr=ListGetNext(Curr);
    }
    ListDestroy(Items, NULL);
}


int ParserIdentifyDocType(const char *TypeStr)
{
    const char *Types[]= {"json","xml","rss","yaml","config","ini","url","cmon",NULL};
    int Type;
    char *Token=NULL;
    const char *ptr;

    GetToken(TypeStr,";",&Token,0);
    StripTrailingWhitespace(Token);
    StripLeadingWhitespace(Token);

    ptr=Token;
    if (strncmp(ptr,"application/",12)==0) ptr+=12;
    if (strncmp(ptr,"x-www-form-urlencoded",21)==0) ptr="url";

    Type=MatchTokenFromList(ptr,Types,0);

    Destroy(Token);
    return(Type);
}



ListNode *ParserNewObject(ListNode *Parent, int Type, const char *Name)
{
    ListNode *Item, *Node;
    char *Token=NULL;

    Item=ListCreate();
    Item->ItemType=ITEM_INTERNAL_LIST;
    Item->Tag=CopyStr(Item->Tag,Name);
    if (StrValid(Name))
    {
        Node=ListAddNamedItem(Parent, Name, Item);
    }
    else
    {
        if (StrValid(Parent->Tag)) Token=FormatStr(Token, "%s:%d",Parent->Tag, ListSize(Parent));
        else Token=FormatStr(Token, "item:%d",ListSize(Parent));
        Node=ListAddNamedItem(Parent, Token, Item);
    }

    Node->ItemType=Type;
    DestroyString(Token);

    return(Node);
}


const char *ParserAddNewStructure(const char *Data, int ParserType, ListNode *Parent, int Type, const char *Name, int IndentLevel)
{
    const char *ptr;
    ListNode *Node;

    Node=ParserNewObject(Parent, Type, Name);
    ptr=ParserParseItems(ParserType, Data, (ListNode *) Node->Item, IndentLevel);

    return(ptr);
}


ListNode *ParserAddArray(ListNode *Parent, const char *Name)
{
    ListNode *Node;

    Node=ParserNewObject(Parent, ITEM_ARRAY, Name);
    if (Node) return(Node->Item);
    return(NULL);
}



ListNode *ParserAddValue(ListNode *Parent, const char *Name, const char *Value)
{
    ListNode *Node=NULL;
    int ItemType=ITEM_STRING;
    char *NewItem=NULL;

    if (StrValid(Name) || StrValid(Value))
    {
        switch (*Value)
        {
        case '"':
            ItemType=ITEM_STRING;
            break;
        case '\'':
            ItemType=ITEM_STRING;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            ItemType=ITEM_INTEGER;
            break;
        }

        NewItem=CopyStr(NULL, Value);
        Node=ListAddNamedItem(Parent, Name, NewItem);
        Node->ItemType=ItemType;
    }

//never do this. NewItem is now in the Parser List
//Destroy(NewItem);

    return(Node);
}



#define JSON_TOKENS ",|[|]|{|}|:|\r|\n"
static const char *ParserJSONItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr;
    char *Token=NULL, *PrevToken=NULL, *Name=NULL;
    int BreakOut=FALSE;

    ptr=GetToken(Doc, JSON_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    while (ptr)
    {
        switch (*Token)
        {
        case '[':
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ARRAY, Name, IndentLevel+1);
            Name=CopyStr(Name,"");
            PrevToken=CopyStr(PrevToken,"");
            break;

        case ']':
            //we can have an item right before a ']' that doesn't terminate with a ',' because the ']' terminates it
            StripQuotes(PrevToken);
            ParserAddValue(Parent, Name, PrevToken);

            if (ptr && (*ptr==',')) ptr++;
            BreakOut=TRUE;
            break;

        case '{':
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY, Name, IndentLevel+1);
            Name=CopyStr(Name,"");
            PrevToken=CopyStr(PrevToken,"");
            break;

        case '}':
            //we can have an item right before a '}' that doesn't terminate with a ',' because the '}' terminates it
            StripQuotes(PrevToken);
            ParserAddValue(Parent, Name, PrevToken);

            if (ptr && (*ptr==',')) ptr++;
            BreakOut=TRUE;
            break;

        case ':':
            Name=CopyStr(Name, PrevToken);
            StripQuotes(Name);
            break;

        case '\r':
            //ignore and do nothing
            break;

        case '\n':
        case ',':
            StripQuotes(PrevToken);
            ParserAddValue(Parent, Name, PrevToken);
            break;

        default:
            StripTrailingWhitespace(Token);
            PrevToken=CopyStr(PrevToken, Token);
            break;
        }

        if (BreakOut) break;
        if (! ptr) break;
        while (isspace(*ptr)) ptr++;
        ptr=GetToken(ptr, JSON_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    }

    DestroyString(PrevToken);
    DestroyString(Token);
    DestroyString(Name);
    return(ptr);
}


const char *ParserYAMLFoldedText(const char *Doc, char **RetStr)
{
    const char *ptr, *tptr;
    char *Token=NULL;
    int IndentLevel=0;
    int count=0;

    *RetStr=CopyStr(*RetStr, NULL);
    ptr=Doc;
    for (tptr=ptr; *tptr==' '; tptr++) IndentLevel++;
    while (*ptr != '\0')
    {
        count=0;
        for (tptr=ptr; *tptr==' '; tptr++) count++;
        if (count >= IndentLevel)
        {
            ptr=GetToken(tptr, "\n", &Token, 0);
            *RetStr=MCatStr(*RetStr, Token, " ",NULL);
        }
        else break;
    }

    Destroy(Token);
    return(ptr);
}



const char *ParserYAMLBlockText(const char *Doc, char **RetStr)
{
    const char *ptr, *tptr;
    char *Token=NULL;
    int IndentLevel=0;
    int count=0;

    *RetStr=CopyStr(*RetStr, NULL);
    ptr=Doc;
    for (tptr=ptr; *tptr==' '; tptr++) IndentLevel++;
    while (*ptr != '\0')
    {
        count=0;
        for (tptr=ptr; *tptr==' '; tptr++) count++;
        if (count >= IndentLevel)
        {
            ptr=GetToken(tptr, "\n", &Token, 0);
            *RetStr=MCatStr(*RetStr, Token, "\n",NULL);
        }
        else break;
    }

    Destroy(Token);
    return(ptr);
}




static const char *ParserYAMLItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr, *tptr;
    char *Token=NULL, *PrevToken=NULL, *Name=NULL;
    int count=0, BreakOut=FALSE;

    ptr=GetToken(Doc, "\n|#|[|]|{|}|:",&Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    while (ptr)
    {
        switch(*Token)
        {
        case ':':
            Name=CopyStr(Name, PrevToken);
            StripTrailingWhitespace(Name);
            StripLeadingWhitespace(Name);
            PrevToken=CopyStr(PrevToken,"");
            break;

        case '#':
            while ((*ptr != '\0') && (*ptr != '\n')) ptr++;
            break;

        case '\n':
            if (StrValid(PrevToken))
            {
                StripTrailingWhitespace(PrevToken);
                StripLeadingWhitespace(PrevToken);
                if (strcmp(PrevToken, ">")==0) ptr=ParserYAMLFoldedText(ptr, &PrevToken);
                if (strcmp(PrevToken, "|")==0) ptr=ParserYAMLBlockText(ptr, &PrevToken);
                StripQuotes(PrevToken);
                ParserAddValue(Parent, Name, PrevToken);
            }

            count=0;
            for (tptr=ptr; *tptr==' '; tptr++) count++;
            if (count > IndentLevel) ptr=ParserAddNewStructure(tptr, ParserType, Parent, ITEM_ENTITY, Name, count);
            else if (count < IndentLevel) BreakOut=TRUE;
            PrevToken=CopyStr(PrevToken,"");
            break;

        default:
            PrevToken=CopyStr(PrevToken, Token);
            break;
        }

        if (BreakOut)
        {
            break;
        }

        ptr=GetToken(ptr, "\n|#[|]|{|}|:",&Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    }

    DestroyString(PrevToken);
    DestroyString(Token);
    DestroyString(Name);

    return(ptr);
}



/*
 Parse callbacks for config files of the form:

name=value
name value
name: value

type name
{
	name=value
	name value
	name: value
}
*/

#define CONFIG_FILE_TOKENS " |	|#|=|:|;|{|}|\r|\n"

static int ParserConfigCheckForBrace(const char **Data)
{
    char *Token=NULL;
    const char *ptr;
    int result=FALSE;

    ptr=*Data;
    if (! ptr) return(FALSE);

    while (isspace(*ptr)) ptr++;
    GetToken(ptr, CONFIG_FILE_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    if (strcmp(Token, "{")==0)
    {
        *Data=ptr;
        result=TRUE;
    }
    DestroyString(Token);

    return(result);
}



static const char *ParserConfigItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr;
    char *Token=NULL, *PrevToken=NULL, *Name=NULL, *Value=NULL;
    int BreakOut=FALSE, NewKey=TRUE;


    ptr=Doc;
    while (ptr && (! BreakOut))
    {
        ptr=GetToken(ptr, CONFIG_FILE_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);

        switch (*Token)
        {
        case '#':
            ptr=GetToken(ptr,"\n",&Token,0);
            break;

        case '{':
            StripLeadingWhitespace(Name);
            StripTrailingWhitespace(Name);

            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY, Name, IndentLevel+1);
            //if we finished parsing a structure, we must reset everything for a new data line
            Name=CopyStr(Name,"");
            Token=CopyStr(Token,"");
            NewKey=TRUE;
            break;

        //these are all possible seperators in key=value lines
        case ' ':
        case '	':
        case ':':
        case '=':
            //new key indicates that this is the first separator of this line. Further ones will be treated as characters rather than separators
            if (NewKey)
            {
                Name=CopyStr(Name, PrevToken);
                //as this is the seperator in key=value so we do not //want to treat it as a token
                Token=CopyStr(Token, "");

                //from here on in anything will be a value, so clear variable out
                Value=CopyStr(Value, "");
                NewKey=FALSE;
            }
            else Value=CatStr(Value, Token);
            while (isspace(*ptr)) ptr++;
            break;

        case '\r':
            break;

        case '}':
            BreakOut=TRUE;
        //fall through


        case '\n':
            if (ParserConfigCheckForBrace(&ptr))
            {
                Name=MCatStr(Name, " ", PrevToken, NULL);
                Value=CopyStr(Value, "");
                break;
            }
        //fall through



        case ';':
            if (StrValid(Value))
            {
                StripLeadingWhitespace(Value);
                StripTrailingWhitespace(Value);
                ParserAddValue(Parent, Name, Value);
                Name=CopyStr(Name,"");
                Value=CopyStr(Value,"");
                //we don't want \r \n or ; tokens included in anything
                Token=CopyStr(Token,"");
            }
            //any of \n ; or } indicate we start reading a fresh entry, but this newkey will not work for }
            //as } causes us to return from ParserAddNewDataStructure and we must set NewKey=TRUE outside of that
            NewKey=TRUE;

            //spaces at the start of the new line are not separators, bypass them
            while (isspace(*ptr)) ptr++;
            break;

        default:
            Value=CatStr(Value, Token);
            break;
        }
        PrevToken=CopyStr(PrevToken, Token);
    }

    DestroyString(PrevToken);
    DestroyString(Token);
    DestroyString(Name);
    DestroyString(Value);
    return(ptr);
}



#define XML_TOKENS " |	|<|>|=|!|/|\r|\n"

static const char *ParserRSSEnclosure(ListNode *Parent, const char *Data)
{
    const char *ptr;
    char *Token=NULL, *Name=NULL;
    int InTag=TRUE;

    ptr=Data;
    while (ptr && InTag)
    {
        ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_QUOTES);
        switch (*Token)
        {
        case '\0':
        case '>':
            InTag=FALSE;
            break;

        case ' ':
            break;
        case '=':
            ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_QUOTES);
            StripQuotes(Token);
            ParserAddValue(Parent, Name, Token);
            break;
        default:
            Name=MCopyStr(Name, "enclosure_",Token,NULL);
            break;
        }
    }

    DestroyString(Token);
    DestroyString(Name);
    return(ptr);
}


static const char *ParserRSSItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr;
    char *Token=NULL, *PrevToken=NULL, *Name=NULL;
    int BreakOut=FALSE, InTag=FALSE;

    PrevToken=CopyStr(PrevToken, "");
    Name=CopyStr(Name, "");

    ptr=Doc;
    while (ptr && (! BreakOut))
    {
        ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);

        switch (*Token)
        {
        case '>':
            InTag=FALSE;
            while (isspace(*ptr)) ptr++;
            break;

        case '<':
            InTag=TRUE;
            while (isspace(*ptr)) ptr++;
            ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);

            switch (*Token)
            {
            case '/':
                ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);
                if (strcasecmp(Token,"item")==0) BreakOut=TRUE;
                else if (strcasecmp(Token,"image")==0) BreakOut=TRUE;
                else if (strcasecmp(Token,"channel")==0) /*ignore */ ;
                else if (strcasecmp(Token,"rss")==0) /*ignore */ ;
                //if this is a 'close' for a previous 'open' then add all the data we collected
                else if (strcasecmp(Token, Name)==0)
                {
                    StripQuotes(Token);
                    ParserAddValue(Parent, Name, PrevToken);
                }
                break;

            case 'i':
            case 'I':
                if (strcasecmp(Token,"item")==0)
                {
                    ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY, NULL, IndentLevel+1);
                }
                else if (strcasecmp(Token,"image")==0)
                {
                    ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY, Token, IndentLevel+1);
                }
                else Name=CopyStr(Name, Token);
                break;

            case '!':
                if (strncmp(ptr,"[CDATA[",7)==0)
                {
                    ptr=GetToken(ptr+7, "]]", &Token,0);
                    PrevToken=CatStr(PrevToken, Token);
                }
                break;

            case '?': //ignore ?xml and the like
                PrevToken=CopyStr(PrevToken, "");
                break;

            default:
                PrevToken=CopyStr(PrevToken,"");
                if (strcasecmp(Token,"channel")==0) /*ignore */ ;
                else if (strcasecmp(Token,"rss")==0) /*ignore */ ;
                /*someone's always got to be different. 'enclosure' breaks the structure of RSS */
                else if (strcasecmp(Token,"enclosure")==0) ptr=ParserRSSEnclosure(Parent, ptr);
                else Name=CopyStr(Name, Token);
                break;
            }
            break;

        default:
            if (! InTag) PrevToken=CatStr(PrevToken, Token);
            break;
        }
    }

    DestroyString(PrevToken);
    DestroyString(Token);
    DestroyString(Name);
    return(ptr);
}



static ListNode *ParserXMLItemsAddSubItem(ListNode *Parent, const char *Name, char *Value)
{
    ListNode *Node=NULL;

    if (Parent->ItemType==ITEM_ROOT) Node=ListAddTypedItem(Parent, ITEM_STRING, Name, Value);
    else if (ParserItemIsValue(Parent)) //if the parent is currently set as a value, then change it to an entity so we can add subitems
    {
        Parent->ItemType=ITEM_ENTITY;
        Parent->Item=ListCreate();
        Node=ListAddTypedItem((ListNode *) Parent->Item, ITEM_STRING, Name, Value);
    }
    else Node=ListAddTypedItem((ListNode *) Parent->Item, ITEM_STRING, Name, Value);

    return(Node);
}


static const char *ParserXMLItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr;
    char *Token=NULL, *PrevToken=NULL;
    ListNode *Node;
    int BreakOut=FALSE;


    ptr=Doc;
    while (ptr && (! BreakOut))
    {
        ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);

        switch (*Token)
        {
        case '<':
            while (isspace(*ptr)) ptr++;
            ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);

            switch (*Token)
            {
            case '/':
                ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);
                if (StrValid(Parent->Tag) && (strcasecmp(Token, Parent->Tag)==0))
                {
                    if (ParserItemIsValue(Parent)) Parent->Item=CopyStr(Parent->Item, PrevToken);
                    DestroyString(PrevToken);
                    DestroyString(Token);
                    return(ptr);
                }
                break;

            case '!':
                if (strncmp(ptr,"[CDATA[",7)==0)
                {
                    ptr=GetToken(ptr+7, "]]", &Token,0);
                    PrevToken=CatStr(PrevToken, Token);
                    while ((*ptr != '\0') && (*ptr != '>')) ptr++;
                    if (*ptr=='>') ptr++;
                }
                break;

            case '?': //ignore ?xml and the like
                PrevToken=CopyStr(PrevToken, "");
                break;


            default:
                //this will be the name of the tag
                PrevToken=CopyStr(PrevToken, Token);

                //consume anything after the tag
                ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);
                while (ptr && (strcmp(Token, ">") !=0) )
                {
                    if (strcmp(Token,"/")==0)
                    {
                        ParserXMLItemsAddSubItem(Parent, PrevToken, CopyStr(NULL, "?"));
                        PrevToken=CopyStr(PrevToken, "");
                    }
                    ptr=GetToken(ptr, XML_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP);
                }

                if (ptr && StrValid(PrevToken))
                {
                    while (isspace(*ptr)) ptr++;
                    Node=ParserXMLItemsAddSubItem(Parent, PrevToken, NULL);
                    ptr=ParserXMLItems(ParserType, ptr, Node, IndentLevel + 1);
                }
                break;
            }
            break;

        default:
            PrevToken=CatStr(PrevToken, Token);
            break;
        }
    }

    DestroyString(PrevToken);
    DestroyString(Token);
    return(ptr);
}




#define CMON_TOKENS " |	|#|=|:|;|{|}|[|]|\r|\n"

static const char *ParserCMONItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr;
    char *Token=NULL, *PrevToken=NULL;
    int BreakOut=FALSE;


    ptr=Doc;
    while (ptr && (! BreakOut))
    {
        //while (isspace(*ptr)) ptr++;
        ptr=GetToken(ptr, CMON_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);

        switch (*Token)
        {
        case '#':
            ptr=GetToken(ptr,"\n",&Token,0);
            break;

        case ':':
            StripQuotes(PrevToken);
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY_LINE, PrevToken, IndentLevel+1);
            Token=CopyStr(Token,"");
            break;

        case '{':
            StripQuotes(PrevToken);
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY, PrevToken, IndentLevel+1);
            Token=CopyStr(Token,"");
            break;

        case '[':
            StripQuotes(PrevToken);
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ARRAY, PrevToken, IndentLevel+1);
            Token=CopyStr(Token,"");
            break;

        case '=':
            if (Parent->ItemType==ITEM_ENTITY_LINE) ptr=GetToken(ptr, "\\S|\n", &Token, GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
            else ptr=GetToken(ptr, "\n", &Token, GETTOKEN_QUOTES);
            StripLeadingWhitespace(Token);
            StripTrailingWhitespace(Token);
            StripQuotes(PrevToken);
            StripQuotes(Token);
            ParserAddValue(Parent, PrevToken, Token);
            break;

        case '\n':
            if (Parent->ItemType==ITEM_ENTITY_LINE) BreakOut=TRUE;
            break;

        case ';':
        case '}':
        case ']':
            BreakOut=TRUE;
            break;

        case ' ':
        case '	':
        case '\r':
            break;

        default:
            PrevToken=CopyStr(PrevToken, Token);
            StripTrailingWhitespace(PrevToken);
            StripLeadingWhitespace(PrevToken);
            break;
        }
    }

    DestroyString(PrevToken);
    DestroyString(Token);
    return(ptr);
}









static const char *ParserURLItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    ptr=strchr(Doc, '?');
    if (! ptr) ptr=strchr(Doc, '#');
    if (! ptr) ptr=Doc;

    ptr=GetNameValuePair(ptr, "&", "=", &Name, &Value);
    while (ptr)
    {
        SetTypedVar(Parent, Name, Value, ITEM_STRING);
        ptr=GetNameValuePair(ptr, "&", "=", &Name, &Value);
    }

    DestroyString(Name);
    DestroyString(Value);

    return(NULL);
}


const char *ParserParseItems(int Type, const char *Doc, ListNode *Parent, int IndentLevel)
{
    switch (Type)
    {
    case PARSER_XML:
        return(ParserXMLItems(Type, Doc, Parent, IndentLevel));
        break;
    case PARSER_RSS:
        return(ParserRSSItems(Type, Doc, Parent, IndentLevel));
        break;
    case PARSER_JSON:
        return(ParserJSONItems(Type, Doc, Parent, IndentLevel));
        break;
    case PARSER_YAML:
        return(ParserYAMLItems(Type, Doc, Parent, IndentLevel));
        break;
    case PARSER_CONFIG:
        return(ParserConfigItems(Type, Doc, Parent, IndentLevel));
        break;
    case PARSER_URL:
        return(ParserURLItems(Type, Doc, Parent, IndentLevel));
        break;
    case PARSER_CMON:
        return(ParserCMONItems(Type, Doc, Parent, IndentLevel));
        break;

    }
    return(NULL);
}



ListNode *ParserParseDocument(const char *TypeStr, const char *Doc)
{
    const char *ptr;
    ListNode *Items;
    int Type;

    if (! StrValid(Doc))
    {
        RaiseError(0, "ParserParseDocument", "Empty Document Supplied");
        return(NULL);
    }

    Type=ParserIdentifyDocType(TypeStr);
    if (Type==-1)
    {
        RaiseError(0, "ParserParseDocument", "Unknown Document Type: %s", TypeStr);
        return(NULL);
    }

    Items=ListCreate();
    ptr=Doc;
    while (isspace(*ptr)) ptr++;
    if (*ptr=='{') ptr++;
    if (*ptr=='[') ptr++;
    ParserParseItems(Type, ptr, Items, 0);

    fflush(NULL);

    return(ParserOpenItem(Items,"/"));
}




ListNode *ParserFindItem(ListNode *Items, const char *Name)
{
    ListNode *Node=NULL;
    char *Token=NULL;
    const char *ptr;

    if (! Items) return(NULL);
    ptr=Name;
    if (*ptr=='/')
    {
        Node=Items;
        ptr++;
        if (*ptr=='\0') return(Node);
    }
    else if (! Items->Side) Node=Items;
    else if (! ParserItemIsValue(Items->Side)) Node=(ListNode *) Items->Side;


    if (Node && StrValid(ptr))
    {
        ptr=GetToken(ptr,"/",&Token,0);
        while (ptr)
        {
            if (Node->ItemType== ITEM_ROOT) Node=(ListNode *) ListFindNamedItem(Node,Token);
            else if (! ParserItemIsValue(Node)) Node=(ListNode *) ListFindNamedItem((ListNode *) Node->Item,Token);

            if (! Node) break;
            ptr=GetToken(ptr,"/",&Token,0);
        }
    }
    else Node=ListGetNext(Items);

    DestroyString(Token);
    return(Node);
}

ListNode *ParserSubItems(ListNode *Node)
{
    if (Node->ItemType == ITEM_STRING) return(NULL);
    if (Node->ItemType == ITEM_INTEGER) return(NULL);
    if (Node->ItemType == ITEM_ROOT) return(Node);
    return((ListNode *) Node->Item);
}


ListNode *ParserOpenItem(ListNode *Items, const char *Name)
{
    ListNode *Node;

    Node=ParserFindItem(Items, Name);
    if (Node) return(ParserSubItems(Node));
    return(NULL);
}


int ParserItemIsValue(ListNode *Node)
{
    if (Node->ItemType == ITEM_STRING) return(TRUE);
    if (Node->ItemType == ITEM_INTEGER) return(TRUE);
    return(FALSE);
}


const char *ParserGetValue(ListNode *Items, const char *Name)
{
    ListNode *Node;

    if (Items)
    {
        Node=Items;
        if (strchr(Name,'/'))
        {
            Node=ParserFindItem(Node, Name);
            if (Node) return((const char *) Node->Item);
        }
        else
        {
            if (Node->ItemType==ITEM_ENTITY) Node=(ListNode *) Node->Item;
            else if (Node->ItemType==ITEM_ENTITY_LINE) Node=(ListNode *) Node->Item;

            if (Node) Node=ListFindNamedItem(Node, Name);
            if (Node) return((const char *) Node->Item);
        }
    }

    return("");
}



char *ParserExportJSON(char *RetStr, int Type,  ListNode *Item)
{
    switch (Item->ItemType)
    {
    case ITEM_STRING:
        if (StrValid(Item->Tag)) RetStr=MCatStr(RetStr, "\"", Item->Tag, "\": \"", (const char *) Item->Item, "\"", NULL);
        else RetStr=MCatStr(RetStr, "\"", (const char *) Item->Item, "\"", NULL);
        break;
    case ITEM_INTEGER:
        if (StrValid(Item->Tag)) RetStr=MCatStr(RetStr, "\"", Item->Tag, "\": ", (const char *) Item->Item, NULL);
        else RetStr=CatStr(RetStr, (const char *) Item->Item);
        break;
    case ITEM_ENTITY:
        RetStr=MCatStr(RetStr, "\"", Item->Tag, "\":\n{\n", NULL);
        RetStr=ParserExportItems(RetStr, Type, 0, (PARSER *) Item->Item);
        RetStr=MCatStr(RetStr, "\n}\n", NULL);
        break;
    case ITEM_ARRAY:
        RetStr=MCatStr(RetStr, "\"", Item->Tag, "\":\n[\n", NULL);
        RetStr=ParserExportItems(RetStr, Type, 0, (PARSER *) Item->Item);
        RetStr=MCatStr(RetStr, "\n]\n", NULL);
        break;

    }

    if (Item->Next) RetStr=CatStr(RetStr, ",\n");

    return(RetStr);
}


char *ParserExportXML(char *RetStr, int Type, ListNode *Item)
{
    switch (Item->ItemType)
    {
    case ITEM_STRING:
    case ITEM_INTEGER:
        if (StrValid(Item->Tag)) RetStr=MCatStr(RetStr, "<", Item->Tag, ">", (const char *) Item->Item, "</", Item->Tag, ">\n", NULL);
        break;

    case ITEM_ENTITY:
        RetStr=MCatStr(RetStr, "<", Item->Tag, ">\n", NULL);
        RetStr=ParserExportItems(RetStr, Type, 0, (PARSER *) Item->Item);
        RetStr=MCatStr(RetStr, "</", Item->Tag, ">\n", NULL);
        break;
    }

    return(RetStr);
}


char *ParserExportYAML(char *RetStr, int Type, int Indent, ListNode *Item)
{
    char *Token=NULL;
    const char *ptr;

    if (Indent > 0) RetStr=PadStr(RetStr, ' ', Indent);
    switch (Item->ItemType)
    {
    case ITEM_STRING:
    case ITEM_INTEGER:
        if (StrValid(Item->Tag))
        {
            if (strchr((const char *) Item->Item, '\n'))
            {
                RetStr=MCatStr(RetStr, Item->Tag, ":|\n", NULL);
                ptr=GetToken((const char *) Item->Item, "\n", &Token, 0);
                while (ptr)
                {
                    RetStr=PadStr(RetStr, ' ', Indent+2);
                    RetStr=MCatStr(RetStr, Token, "\n", NULL);
                    ptr=GetToken(ptr, "\n", &Token, 0);
                }
            }
            else RetStr=MCatStr(RetStr, Item->Tag, ": ", (const char *) Item->Item, "\n", NULL);
        }
        break;

    case ITEM_ENTITY:
        RetStr=MCatStr(RetStr, Item->Tag, ":\n", NULL);
        RetStr=ParserExportItems(RetStr, Type, Indent+2, (PARSER *) Item->Item);
        break;
    }

    Destroy(Token);

    return(RetStr);
}




char *ParserExportCMON(char *RetStr, int Type, int Indent, ListNode *Item)
{
    char *Token=NULL;
    const char *ptr;

    if (Indent > 0) RetStr=PadStr(RetStr, ' ', Indent);
    switch (Item->ItemType)
    {
    case ITEM_STRING:
    case ITEM_INTEGER:
        Token=QuoteCharsInStr(Token, (const char *) Item->Item, "\r\n");
        if (StrValid(Item->Tag)) RetStr=MCatStr(RetStr, "'", Item->Tag, "'='", Token, "' ", NULL);
        else RetStr=MCatStr(RetStr, "'", Token, "' ", NULL);
        if (Indent==1) RetStr=CatStr(RetStr, " ");
        else RetStr=CatStr(RetStr, "\n");
        break;

    case ITEM_ARRAY:
        RetStr=MCatStr(RetStr, Item->Tag, " [\n", NULL);
        RetStr=ParserExportItems(RetStr, Type, 0, (PARSER *) Item->Item);
        RetStr=CatStr(RetStr, "]\n");
        break;

    case ITEM_ENTITY:
        if (Indent==0)
        {
            RetStr=MCatStr(RetStr, "'", Item->Tag, "': ", NULL);
            RetStr=ParserExportItems(RetStr, Type, 1, (PARSER *) Item->Item);
            RetStr=CatStr(RetStr, "\n");
        }
        else
        {
            RetStr=MCatStr(RetStr, Item->Tag, " { ", NULL);
            RetStr=ParserExportItems(RetStr, Type, 0, (PARSER *) Item->Item);
            RetStr=CatStr(RetStr, " }");
        }
        break;
    }

    Destroy(Token);

    return(RetStr);
}



char *ParserExportItems(char *RetStr, int Type, int Indent, PARSER *P)
{
    ListNode *Curr;

    Curr=ListGetNext(P);
    while (Curr)
    {
        switch (Type)
        {
        case PARSER_JSON:
            RetStr=ParserExportJSON(RetStr, Type, Curr);
            break;
        case PARSER_XML:
            RetStr=ParserExportXML(RetStr, Type, Curr);
            break;
        case PARSER_YAML:
            RetStr=ParserExportYAML(RetStr, Type, Indent, Curr);
            break;
        case PARSER_CMON:
            RetStr=ParserExportCMON(RetStr, Type, Indent, Curr);
            break;
        }
        Curr=ListGetNext(Curr);
    }

    return(RetStr);
}

char *ParserExport(char *RetStr, const char *Format, PARSER *P)
{
    int Type;

    Type=ParserIdentifyDocType(Format);
    if (Type==-1)
    {
        RaiseError(0, "ParserExport", "Unknown Document Type: %s", Format);
        return(RetStr);
    }

    if (Type==PARSER_JSON) RetStr=CatStr(RetStr, "{\n");
    RetStr=ParserExportItems(RetStr, Type, 0, P);
    if (Type==PARSER_JSON) RetStr=CatStr(RetStr, "}");

    return(RetStr);
}
