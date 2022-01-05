#include "TerminalMenu.h"
#include "Terminal.h"

TERMMENU *TerminalMenuCreate(STREAM *Term, int x, int y, int wid, int high)
{
    TERMMENU *Item;

    Item=(TERMMENU *) calloc(1,sizeof(TERMMENU));
    Item->Term=Term;
    Item->x=x;
    Item->y=y;
    Item->wid=wid;
    Item->high=high;
    Item->Options=ListCreate();
    Item->MenuAttribs=CopyStr(Item->MenuAttribs, "~C~n");
    Item->MenuCursorLeft=CopyStr(Item->MenuCursorLeft, "~W~n");

    return(Item);
}





void TerminalMenuDraw(TERMMENU *Menu)
{
    ListNode *Curr, *Prev;
    char *Contents=NULL, *Tempstr=NULL, *Output=NULL;
    char *p_Attribs, *p_Cursor, *p_Color=NULL;
    int y, yend, count;

    y=Menu->y;
    yend=y+Menu->high;

    if (Menu->Options->Side)
    {
        Curr=Menu->Options->Side;
        Prev=ListGetPrev(Curr);
        count=0;
        while (Prev)
        {
            Curr=Prev;
            count++;
            if (count >= Menu->high) break;
            Prev=ListGetPrev(Curr);
        }
    }
    else Curr=ListGetNext(Menu->Options);

    while (Curr)
    {
        TerminalCursorMove(Menu->Term, Menu->x, y);
        if (Menu->Options->Side==Curr)
        {
            p_Attribs=Menu->MenuCursorLeft;
            p_Cursor="> ";
        }
        else if (Curr->Flags & TERMMENU_SELECTED)
        {
            p_Attribs=Menu->MenuAttribs;
            p_Cursor=" X";
        }
        else
        {
            p_Attribs=Menu->MenuAttribs;
            p_Cursor="  ";
        }

        Contents=ReplaceStr(Contents, Curr->Tag, "~0", p_Attribs);
        Contents=TerminalStrTrunc(Contents, Menu->wid-4);
        Tempstr=MCopyStr(Tempstr, p_Attribs, p_Cursor, Contents, NULL);

        Output=CopyStr(Output, "");
        TerminalFormatSubStr(Tempstr, &Output, Menu->Term);

        //length has two added for the leading space for the cursor

        count=TerminalStrLen(Contents);
        while (count < Menu->wid-2)
        {
            Output=CatStr(Output, " ");
            count++;
        }
        STREAMWriteString(Output, Menu->Term);
        STREAMWriteString(ANSI_NORM, Menu->Term);
        y++;
        if (y > yend) break;
        Curr=ListGetNext(Curr);
    }

    Tempstr=CopyStr(Tempstr, "");
    Tempstr=PadStrTo(Tempstr, ' ', Menu->wid);
    while (y <= yend)
    {
        TerminalCursorMove(Menu->Term, Menu->x, y);
        STREAMWriteString(Tempstr, Menu->Term);
        y++;
    }

    STREAMFlush(Menu->Term);

    Destroy(Contents);
    Destroy(Tempstr);
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
