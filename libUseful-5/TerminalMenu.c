#include "TerminalMenu.h"
#include "Terminal.h"

TERMMENU *TerminalMenuCreate(STREAM *Term, int x, int y, int wid, int high)
{
    return(TerminalWidgetNew(Term, x, y, wid, high, "attribs= cursor_attribs=~e selected_attribs=~e cursor=>"));
}



static ListNode *MenuGetTop(TERMMENU *Menu)
{
    ListNode *Curr, *Prev;
    int count;

    if (Menu->Options->Side)
    {
        Curr=Menu->Options->Side;
        Prev=ListGetPrev(Curr);
        count=0;
        while (Prev)
        {
            count++;
            if (count >= Menu->high) break;
            Curr=Prev;
            Prev=ListGetPrev(Curr);
        }
    }
    else Curr=ListGetNext(Menu->Options);

    return(Curr);
}


static void TerminalGetLineAttributes(TERMMENU *Menu, ListNode *Curr, char **p_Attribs, char **LineStart, char **LineEnd)
{
    //start with default attribs
    *p_Attribs=Menu->Attribs;
    *LineEnd=CopyPadStr(*LineEnd, "", ' ', TerminalStrLen(Menu->CursorRight));

    if (Menu->Options->Side == Curr)
    {
        *p_Attribs=Menu->CursorAttribs;
        *LineStart=MCopyStr(*LineStart, Menu->Attribs, Menu->CursorAttribs, Menu->CursorLeft, NULL);
        *LineEnd=MCopyStr(*LineEnd, Menu->CursorAttribs, Menu->CursorRight, NULL);
    }
    else *LineStart=CopyPadStr(*LineStart, Menu->Attribs, ' ', TerminalStrLen(Menu->CursorLeft));

    if (Curr->Flags & TERMMENU_SELECTED)
    {
        *p_Attribs=Menu->SelectedAttribs;
        *LineStart=MCatStr(*LineStart, Menu->SelectedAttribs, "*", NULL);
    }
    else *LineStart=CatStr(*LineStart, " ");
}



static char *TerminalMenuFormatItem(char *Output, TERMMENU *Menu, ListNode *Curr)
{
    int count, margins;
    char *LineStart=NULL, *LineEnd=NULL, *Contents=NULL, *Tempstr=NULL;
    char *SingleLineCR=NULL;
    char *p_Attribs;

    margins=TerminalStrLen(Menu->CursorLeft) + TerminalStrLen(Menu->CursorRight) + 1;
    TerminalGetLineAttributes(Menu, Curr, &p_Attribs, &LineStart, &LineEnd);

    //single line mode uses a carriage return to return to the start of the line,
    //rather than a full scale menu with cursor positioning
    if (Menu->y == -1) SingleLineCR=CopyStr(SingleLineCR, "\r");
    else SingleLineCR=CopyStr(SingleLineCR, "");

    if (StrValid(p_Attribs)) Contents=ReplaceStr(Contents, Curr->Tag, "~0", p_Attribs);
    else Contents=CopyStr(Contents, Curr->Tag);

    Contents=TerminalStrTrunc(Contents, Menu->wid - margins);
    count=TerminalStrLen(Contents);
    while (count < (Menu->wid - margins))
    {
        Contents=CatStr(Contents, " ");
        count++;
    }


    Tempstr=MCopyStr(Tempstr, SingleLineCR, LineStart, Contents, LineEnd, NULL);

    Output=CopyStr(Output, "");
    TerminalFormatSubStr(Tempstr, &Output, Menu->Term);

    Destroy(SingleLineCR);
    Destroy(LineStart);
    Destroy(LineEnd);
    Destroy(Contents);
    Destroy(Tempstr);

    return(Output);
}


//If a menu has fewer items in it than can fill it's full size,
//then we need to pad it with extra lines to it's full size.
void TerminalMenuPadToEnd(TERMMENU *Menu, int start, int end)
{
    int y;
    char *Output=NULL;

    y=start;
    Output=PadStrTo(Output, ' ', Menu->wid);
    while (y <= end)
    {
        TerminalCursorMove(Menu->Term, Menu->x, y);
        STREAMWriteString(Output, Menu->Term);
        y++;
    }

    Destroy(Output);
}


void TerminalMenuDraw(TERMMENU *Menu)
{
    ListNode *Curr;
    char *Output=NULL;
    int y, yend, count;
    //single line mode is a special case
    //that doesn't use 'cursor move' to build
    //a menu at a specific screen position,
    //but instead creates a simplified single-line menu
    //at the current cursor position
    int singleline=FALSE;

    if (Menu->y==-1)
    {
        y=0;
        singleline=TRUE;
        yend=1;
    }
    else
    {
        y=Menu->y;
        yend=y+Menu->high;
    }

    Curr=MenuGetTop(Menu);
    while (Curr)
    {
        if (! singleline) TerminalCursorMove(Menu->Term, Menu->x, y);
        Output=TerminalMenuFormatItem(Output, Menu, Curr);

        //length has two added for the leading space for the cursor

        STREAMWriteString(Output, Menu->Term);
        STREAMWriteString(ANSI_NORM, Menu->Term);
        y++;
        if (y >= yend) break;
        Curr=ListGetNext(Curr);
    }


    if (! singleline) TerminalMenuPadToEnd(Menu, y, yend);
    STREAMFlush(Menu->Term);

    Destroy(Output);
}


ListNode *TerminalMenuTop(TERMMENU *Menu)
{
    Menu->Options->Side=ListGetNext(Menu->Options);

    return(Menu->Options->Side);
}

ListNode *TerminalMenuUp(TERMMENU *Menu)
{
    if (Menu->Options->Side)
    {
        if (Menu->Options->Side->Prev && (Menu->Options->Side->Prev != Menu->Options)) Menu->Options->Side=Menu->Options->Side->Prev;
    }
    else Menu->Options->Side=ListGetNext(Menu->Options);

    return(Menu->Options->Side);
}

ListNode *TerminalMenuDown(TERMMENU *Menu)
{
    if (Menu->Options->Side)
    {
        if (Menu->Options->Side->Next) Menu->Options->Side=Menu->Options->Side->Next;
    }
    else Menu->Options->Side=ListGetNext(Menu->Options);

    return(Menu->Options->Side);
}

ListNode *TerminalMenuOnKey(TERMMENU *Menu, int key)
{
    int i;

    switch (key)
    {
    case TKEY_HOME:
        TerminalMenuTop(Menu);
        break;

    case TKEY_UP:
    case TKEY_CTRL_W:
        TerminalMenuUp(Menu);
        break;

    case TKEY_DOWN:
    case TKEY_CTRL_S:
        TerminalMenuDown(Menu);
        break;

    case TKEY_PGUP:
        for (i=0; i < Menu->high; i++)
        {
            if (Menu->Options->Side)
            {
                if (Menu->Options->Side->Prev && (Menu->Options->Side->Prev != Menu->Options->Side->Head)) Menu->Options->Side=Menu->Options->Side->Prev;
            }
            else Menu->Options->Side=ListGetPrev(Menu->Options);
        }
        break;

    case TKEY_PGDN:
        for (i=0; i < Menu->high; i++)
        {
            if (Menu->Options->Side)
            {
                if (Menu->Options->Side->Next) Menu->Options->Side=Menu->Options->Side->Next;
            }
            else Menu->Options->Side=ListGetNext(Menu->Options);
        }
        break;

    case ' ':
        if (Menu->Options->Side)
        {
            //if Flags for the list head item has 'TERMMENU_SELECTED' set, then toggle that value for the
            //list item
            if (Menu->Options->Flags & TERMMENU_SELECTED) Menu->Options->Side->Flags ^= TERMMENU_SELECTED;
        }
        break;

    case '\r':
    case '\n':
    case TKEY_CTRL_D:
        return(Menu->Options->Side);
        break;
    }
    TerminalMenuDraw(Menu);

    return(NULL);
}



ListNode *TerminalMenuProcess(TERMMENU *Menu)
{
    ListNode *Node;
    int key;

    TerminalMenuDraw(Menu);

    key=TerminalReadChar(Menu->Term);
    while (1)
    {
        if ((key == ESCAPE) || (key == TKEY_CTRL_A))
        {
            Node=NULL;
            break;
        }
        Node=TerminalMenuOnKey(Menu, key);
        if (Node) break;
        key=TerminalReadChar(Menu->Term);
    }


    return(Node);
}


ListNode *TerminalMenu(STREAM *Term, ListNode *Options, int x, int y, int wid, int high)
{
    TERMMENU *Menu;
    ListNode *Node;
    int key;

    Menu=TerminalMenuCreate(Term, x, y, wid, high);
    Menu->Options = Options;
    if (! Menu->Options->Side) Menu->Options->Side=ListGetNext(Menu->Options);
    Node=TerminalMenuProcess(Menu);
    Menu->Options=NULL;

    TerminalMenuDestroy(Menu);
    return(Node);
}


char *TerminalMenuFromText(char *RetStr, STREAM *Term, const char *Options, int x, int y, int wid, int high)
{
    ListNode *MenuList, *Node;
    const char *ptr;
    char *Token=NULL;

    MenuList=ListCreate();
    ptr=GetToken(Options, "|", &Token, 0);
    while (ptr)
    {
        ListAddNamedItem(MenuList, Token, NULL);
        ptr=GetToken(ptr, "|", &Token, 0);
    }

    Node=TerminalMenu(Term, MenuList, x, y, wid, high);
    if (Node) RetStr=CopyStr(RetStr, Node->Tag);
    else
    {
        Destroy(RetStr);
        RetStr=NULL;
    }
    ListDestroy(MenuList, Destroy);
    Destroy(Token);

    return(RetStr);
}
