#include "DataParser.h"


//Gets recursively called
const char *ParserParseItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel);


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



const char *ParserAddNewStructure(const char *Data, int ParserType, ListNode *Parent, int Type, const char *Name, int IndentLevel)
{
    ListNode *Item, *Node;
    char *Token=NULL;
    const char *ptr;

    Item=ListCreate();
    Item->Tag=CopyStr(Item->Tag,Name);
    if (Name) Node=ListAddNamedItem(Parent, Name, Item);
    else
    {
        if (StrValid(Parent->Tag)) Token=FormatStr(Token, "%s:%d",Parent->Tag, ListSize(Parent));
        else Token=FormatStr(Token, "item:%d",ListSize(Parent));
        Node=ListAddNamedItem(Parent, Token, Item);
    }

    Node->ItemType=Type;
    ptr=ParserParseItems(ParserType, Data, Item, IndentLevel);

    DestroyString(Token);
    return(ptr);
}


void ParserAddValue(ListNode *Parent, const char *Name, const char *Value)
{
ListNode *Node;

  if (StrValid(Name) || StrValid(Value))
  {
    Node=ListAddNamedItem(Parent, Name, CopyStr(NULL, Value));
    Node->ItemType=ITEM_VALUE;
  }
}

#define JSON_TOKENS ",|[|]|{|}|:|\r|\n"
static const char *ParserJSONItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr;
    char *Token=NULL, *PrevToken=NULL, *Name=NULL;
    ListNode *Node;
    int BreakOut=FALSE;

    ptr=GetToken(Doc, JSON_TOKENS, &Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    while (ptr)
    {
        switch (*Token)
        {
        case '[':
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ARRAY, Name, IndentLevel+1);
						Name=CopyStr(Name,"");
            break;

        case ']':
            //we can have an item right before a ']' that doesn't terminate with a ',' because the ']' terminates it
						ParserAddValue(Parent, Name, PrevToken);

            if (ptr && (*ptr==',')) ptr++;
            BreakOut=TRUE;
            break;

        case '{':
            ptr=ParserAddNewStructure(ptr, ParserType, Parent, ITEM_ENTITY, Name, IndentLevel+1);
						Name=CopyStr(Name,"");
            break;

        case '}':
            //we can have an item right before a '}' that doesn't terminate with a ',' because the '}' terminates it
						ParserAddValue(Parent, Name, PrevToken);

            if (ptr && (*ptr==',')) ptr++;
            BreakOut=TRUE;
            break;

        case ':':
            Name=CopyStr(Name, PrevToken);
            break;

        case '\r':
            //ignore and do nothing
            break;

        case '\n':
        case ',':
						ParserAddValue(Parent, Name, PrevToken);
            break;

        default:
            StripTrailingWhitespace(Token);
            StripQuotes(Token);
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




static const char *ParserYAMLItems(int ParserType, const char *Doc, ListNode *Parent, int IndentLevel)
{
    const char *ptr, *tptr;
    char *Token=NULL, *PrevToken=NULL, *Name=NULL;
    int count=0, BreakOut=FALSE;
    ListNode *Node;

    ptr=GetToken(Doc, "\n|#|[|]|{|}|:",&Token,GETTOKEN_MULTI_SEP|GETTOKEN_INCLUDE_SEP|GETTOKEN_HONOR_QUOTES);
    while (ptr)
    {
        switch(*Token)
        {
        case ':':
            Name=CopyStr(Name, PrevToken);
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
    ListNode *Node;
    int BreakOut=FALSE, NewKey=TRUE;


    ptr=Doc;
    while (ptr && (! BreakOut))
    {
        //while (isspace(*ptr)) ptr++;
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
						Name=CopyStr(Name,"");
						Token=CopyStr(Token,"");
            break;

				//these are all possible seperators in key=value lines
        case ' ':
        case '	':
        case ':':
        case '=':
				if (NewKey)
				{
					  Name=CopyStr(Name, PrevToken);
						//as this is the seperator in key=value so we do not
						//want to treat it as a token
						Token=CopyStr(Token, "");

						//from here on in anything will be a value, so clear variable out
						Value=CopyStr(Value, "");
						NewKey=FALSE;
				}
				else Value=CatStr(Value, Token);
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
						NewKey=TRUE;
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
    ListNode *Node;
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
    ListNode *Node;
    int BreakOut=FALSE, InTag=FALSE;


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
                else if (strcasecmp(Token, Name)==0) ParserAddValue(Parent, Name, PrevToken);
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

	if (Parent->ItemType==ITEM_ROOT) Node=ListAddTypedItem(Parent, ITEM_VALUE, Name, Value);
	else if (Parent->ItemType==ITEM_VALUE)
	{
			Parent->ItemType=ITEM_ENTITY;
			Parent->Item=ListCreate();
			Node=ListAddTypedItem((ListNode *) Parent->Item, ITEM_VALUE, Name, Value);
	}
	else Node=ListAddTypedItem((ListNode *) Parent->Item, ITEM_VALUE, Name, Value);

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
									if (Parent->ItemType==ITEM_VALUE) Parent->Item=CopyStr(Parent->Item, PrevToken);
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
        SetTypedVar(Parent, Name, Value, ITEM_VALUE);
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
    }
    return(NULL);
}



ListNode *ParserParseDocument(const char *TypeStr, const char *Doc)
{
    ListNode *Node, *Items;
    const char *Types[]= {"json","xml","rss","yaml","config","ini","url",NULL};
    const char *ptr;
    char *Token=NULL;
    int Type;

    if (! StrValid(TypeStr)) return(NULL);
    if (! StrValid(Doc)) return(NULL);

    GetToken(TypeStr,";",&Token,0);
    StripTrailingWhitespace(Token);
    StripLeadingWhitespace(Token);

    ptr=Token;
    if (strncmp(ptr,"application/",12)==0) ptr+=12;
    if (strncmp(ptr,"x-www-form-urlencoded",21)==0) ptr="url";

    Type=MatchTokenFromList(ptr,Types,0);

    Items=ListCreate();
    ptr=Doc;
    while (isspace(*ptr)) ptr++;
    if (*ptr=='{') ptr++;
    if (*ptr=='[') ptr++;
    ParserParseItems(Type, ptr, Items, 0);

    fflush(NULL);

    return(ParserOpenItem(Items,"/"));
}



void ListDump(ListNode *List)
{
    ListNode *Curr;

    Curr=ListGetNext(List);
    while (Curr)
    {
        printf("  %s\n",Curr->Tag);
        Curr=ListGetNext(Curr);
    }

}


ListNode *ParserFindItem(ListNode *Items, const char *Name)
{
    ListNode *Node, *Curr;
    char *Token=NULL;
    const char *ptr;


    ptr=Name;
    if (*ptr=='/')
    {
        Node=Items;
        ptr++;
        if (*ptr=='\0') return(Node);
    }
    else if (! Items->Side) Node=Items;
    else if (Items->Side->ItemType != ITEM_VALUE) Node=(ListNode *) Items->Side;


    if (Node && StrValid(ptr))
    {
        ptr=GetToken(ptr,"/",&Token,0);
        while (ptr)
        {
            if (Node->ItemType== ITEM_ROOT) Node=(ListNode *) ListFindNamedItem(Node,Token);
            else if (Node->ItemType != ITEM_VALUE) Node=(ListNode *) ListFindNamedItem((ListNode *) Node->Item,Token);

            if (! Node) break;
            ptr=GetToken(ptr,"/",&Token,0);
        }
    }
    else Node=ListGetNext(Items);

    DestroyString(Token);
    return(Node);
}



ListNode *ParserOpenItem(ListNode *Items, const char *Name)
{
    ListNode *Node;

    Node=ParserFindItem(Items, Name);
    if (Node)
    {
        if (Node->ItemType == ITEM_VALUE) return(NULL);
        if (Node->ItemType == ITEM_ROOT) return(Node);
        return((ListNode *) Node->Item);
    }
    else return(NULL);
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
            if (Node) Node=ListFindNamedItem(Node, Name);
            if (Node) return((const char *) Node->Item);
        }
    }

    return("");
}

