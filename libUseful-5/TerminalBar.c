#include "TerminalBar.h"
#include "Terminal.h"


//this function exists elsewhere, but we don't really want to make it available to users
void TerminalInternalConfig(const char *Config, int *ForeColor, int *BackColor, int *Flags, int *wide, int *high);

void TerminalBarUpdate(TERMBAR *TB, const char *Text)
{
    int rows, cols, x=0, y=0, TextLen;
    char *Str=NULL;

    TextLen=TerminalStrLen(Text);
    TerminalGeometry(TB->Term, &cols, &rows);
    TerminalCommand(TERM_CURSOR_SAVE, 0, 0, TB->Term);

    if (TB->Flags & TERMBAR_UPPER) y=0;
    else y=rows-1;

    TerminalCommand(TERM_CURSOR_MOVE, 0, y, TB->Term);

    Str=MCopyStr(Str, ANSICode(TB->ForeColor, TB->BackColor, TB->Flags), Text, NULL);
    Str=PadStr(Str, ' ', cols-TextLen);


    TerminalPutStr(Str, TB->Term);
    TerminalCommand(TERM_CURSOR_UNSAVE, 0, 0, TB->Term);

    DestroyString(Str);
}







char *TerminalBarReadText(char *RetStr, TERMBAR *TB, int Flags, const char *Prompt)
{
    int inchar, PromptLen;
    TKEY_CALLBACK_FUNC Func;
    char *DisplayString=NULL;
    const char *ptr;
    int cols, rows;

    RetStr=CopyStr(RetStr,"");
    PromptLen=StrLen(Prompt);
    TerminalGeometry(TB->Term, &cols, &rows);
    TerminalBarUpdate(TB, Prompt);
    TB->TextLen=0;

    inchar=TerminalReadChar(TB->Term);
    while (inchar != EOF)
    {
        Func=STREAMGetItem(TB->Term, "KeyCallbackFunc");
        if (Func) Func(TB->Term, inchar);

        switch (inchar)
        {
        case STREAM_TIMEOUT:
        case STREAM_NODATA:
            DestroyString(DisplayString);
            return(RetStr);
            break;

        case '\n':
            RetStr=CopyStrLen(RetStr, TB->Text, TB->TextLen);
            TB->TextLen=0;
            DestroyString(DisplayString);
            return(RetStr);
            break;

        case '\b':
            TB->Text[TB->TextLen]='\0';
            if (TB->TextLen > 0) TB->TextLen--;
            break;

        default:
            if ((inchar > 31) && (inchar < 127))
            {
                TB->Text=AddCharToBuffer(TB->Text,TB->TextLen,inchar);
                TB->TextLen++;
            }
            break;
        }

        if ((TB->TextLen + PromptLen) > (cols-1)) ptr=TB->Text + (TB->TextLen - (cols - 1 - PromptLen));
        else ptr=TB->Text;

        if (Flags & TERM_HIDETEXT) ptr=NULL ;
        else if (Flags & TERM_SHOWSTARS) DisplayString=CopyPadStr(DisplayString, Prompt, '*', StrLen(ptr));
        else DisplayString=MCopyStr(DisplayString, Prompt, ptr, NULL);

        TerminalBarUpdate(TB, DisplayString);
        inchar=TerminalReadChar(TB->Term);
    }

    DestroyString(DisplayString);
    DestroyString(RetStr);
    return(NULL);
}


void TerminalBarMenuUpdate(TERMBAR *TB, ListNode *Items)
{
    ListNode *Curr;
    char *Tempstr=NULL, *LPad=NULL, *RPad=NULL;

    LPad=PadStr(LPad, ' ', TerminalStrLen(TB->CursorLeft));
    RPad=PadStr(RPad, ' ', TerminalStrLen(TB->CursorRight));
    Curr=ListGetNext(Items);
    while (Curr)
    {
        if (Items->Side==Curr)
        {
            Tempstr=MCatStr(Tempstr, TB->CursorLeft, Curr->Tag, TB->CursorRight,NULL);
        }
        else Tempstr=MCatStr(Tempstr, LPad, Curr->Tag, RPad, NULL);

        Curr=ListGetNext(Curr);
    }

    TerminalBarUpdate(TB, Tempstr);

    DestroyString(Tempstr);
    DestroyString(LPad);
    DestroyString(RPad);
}



char *TerminalBarMenu(char *RetStr, TERMBAR *TB, const char *ItemStr)
{
    ListNode *Items, *Curr;
    const char *ptr;
    char *Token=NULL;
    int inchar, Done=FALSE;

    Items=ListCreate();
    ptr=GetToken(ItemStr, ",", &Token, GETTOKEN_QUOTES);
    while (ptr)
    {
        ListAddNamedItem(Items, Token, NULL);
        ptr=GetToken(ptr, ",", &Token, GETTOKEN_QUOTES);
    }

    Curr=ListGetNext(Items);
    Items->Side=Curr;

    TerminalBarMenuUpdate(TB, Items);
    inchar=TerminalReadChar(TB->Term);
    while (! Done)
    {
        switch (inchar)
        {
        case EOF:
            Done=TRUE;
            break;

        case '<':
        case TKEY_LEFT:
            Curr=ListGetPrev(Items->Side);
            if (Curr) Items->Side=Curr;
            break;

        case '>':
        case TKEY_RIGHT:
            Curr=ListGetNext(Items->Side);
            if (Curr) Items->Side=Curr;
            break;

        case '\r':
        case '\n':
            RetStr=CopyStr(RetStr, Items->Side->Tag);
            Done=TRUE;
            break;
        }

        if (Done) break;
        TerminalBarMenuUpdate(TB, Items);
        inchar=TerminalReadChar(TB->Term);
    }


    DestroyString(Token);
    ListDestroy(Items, NULL);

    return(RetStr);
}





void TerminalBarsInit(STREAM *S)
{
    int top=0, bottom=0, cols, rows;
    ListNode *Curr;
    char *Tempstr=NULL;
    TERMBAR *TB;

    TerminalGeometry(S, &cols, &rows);
    Curr=ListGetNext(S->Items);
    while (Curr)
    {
        if (strcmp(Curr->Tag, "termbar")==0)
        {
            TB=(TERMBAR *) Curr->Item;
            if (TB->Flags & TERMBAR_UPPER) top=1;
            else bottom=1;
        }
        Curr=ListGetNext(Curr);
    }

    if ((top > 0) || (bottom > 0))
    {
        if (top > 0)
        {
            Tempstr=FormatStr(Tempstr, "%d", top);
            STREAMSetValue(S, "Terminal:top",Tempstr);
        }

        if (bottom > 0)
        {
            Tempstr=FormatStr(Tempstr, "%d", bottom);
            STREAMSetValue(S, "Terminal:bottom",Tempstr);
        }

        //set scrolling region to be top+2 because x and y for terminal are indexed from one, so
        //this is treally 'top+1'
        TerminalCommand(TERM_SCROLL_REGION, top+2, rows-(top+bottom), S);

        //move the cursor into the scrolling region
        TerminalCommand(TERM_CURSOR_MOVE, 0, top+2, S);
    }

    Destroy(Tempstr);
}



void TerminalBarSetConfig(TERMBAR *TB, const char *Config)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    //first check for options only used in terminal bars
    ptr=GetNameValuePair(Config, " ","=",&Name,&Value);
    while (ptr)
    {
        switch (*Name)
        {
        case 'm':
        case 'M':
            if (strcasecmp(Name,"MenuPadLeft")==0) TB->MenuPadLeft=CopyStr(TB->MenuPadLeft, Value);
            if (strcasecmp(Name,"MenuPadRight")==0) TB->MenuPadRight=CopyStr(TB->MenuPadRight, Value);
            if (strcasecmp(Name,"CursorLeft")==0) TB->CursorLeft=CopyStr(TB->CursorLeft, Value);
            if (strcasecmp(Name,"CursorRight")==0) TB->CursorRight=CopyStr(TB->CursorRight, Value);
            break;

        case 'x':
        case 'X':
            if (strcasecmp(Name, "x")==0) TB->x=atoi(Value);
            break;

        case 'y':
        case 'Y':
            if (strcasecmp(Name, "y")==0) TB->y=atoi(Value);
            break;
        }
        ptr=GetNameValuePair(ptr, " ","=",&Name,&Value);
    }

    //then check for default options, backcolor and forecolor reversed because terminal bars
    //are inverse text
    TerminalInternalConfig(Config, &(TB->BackColor), &(TB->ForeColor), &(TB->Flags), NULL, NULL);

    DestroyString(Name);
    DestroyString(Value);
}


TERMBAR *TerminalBarCreate(STREAM *Term, const char *Config, const char *Text)
{
    TERMBAR *TB;
    char *Tempstr=NULL;
    const char *ptr;

    TB=(TERMBAR *) calloc(1,sizeof(TERMBAR));
    TB->Term=Term;
    TB->Flags = ANSI_INVERSE;
    TB->MenuPadLeft=CopyStr(NULL, "  ");
    TB->MenuPadRight=CopyStr(NULL, "  ");
    TB->CursorLeft=CopyStr(NULL, " [");
    TB->CursorRight=CopyStr(NULL, "] ");

    TerminalBarSetConfig(TB, Config);
    STREAMSetItem(Term, "termbar", TB);

    TerminalBarsInit(Term);

    if (StrValid(Text)) TerminalBarUpdate(TB, Text);
    DestroyString(Tempstr);

    return(TB);
}


void TerminalBarDestroy(TERMBAR *TB)
{
    DestroyString(TB->MenuPadLeft);
    DestroyString(TB->MenuPadRight);
    DestroyString(TB->CursorLeft);
    DestroyString(TB->CursorRight);
    free(TB);
}


