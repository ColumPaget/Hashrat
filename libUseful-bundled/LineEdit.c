#include "LineEdit.h"
#include "KeyCodes.h"

TLineEdit *LineEditCreate(int Flags)
{
    TLineEdit *LE;

    LE=(TLineEdit *) calloc(1, sizeof(TLineEdit));
    LE->Flags = Flags;
    if (Flags & LINE_EDIT_HISTORY)
    {
        LE->History=ListCreate();
        LE->MaxHistory=30;
    }

    return(LE);
}

void LineEditDestroy(TLineEdit *LE)
{
    if (LE)
    {
        if ((LE->Flags & LINE_EDIT_HISTORY) && LE->History) ListDestroy(LE->History, NULL);
        Destroy(LE->Line);
        free(LE);
    }
}


void LineEditSetText(TLineEdit *LE, const char *Text)
{
    LE->Line=CopyStr(LE->Line, Text);
    LE->Len=StrLen(LE->Line);
    LE->Cursor=LE->Len;
}


void LineEditClearHistory(TLineEdit *LE)
{
    if (LE->History) ListClear(LE->History, NULL);
}


ListNode *LineEditSwapHistory(TLineEdit *LE, ListNode *NewHist)
{
    ListNode *Old;

    Old=LE->History;
    LE->History=NewHist;

    return(Old);
}


void LineEditAddToHistory(TLineEdit *LE, const char *Text)
{
    ListNode *Node;

    if (! LE->History) return;

    if (StrValid(Text))
    {

        if ( (LE->MaxHistory > 0) && (ListSize(LE->History) > LE->MaxHistory) )
        {
            Node=ListGetNext(LE->History);
            ListDeleteNode(Node);
        }

        Node=ListGetLast(LE->History);

        if (Node && Node->Tag)
        {
            if (strcmp(Node->Tag, Text) !=0) ListAddNamedItem(LE->History, Text, NULL);
        }
        else ListAddNamedItem(LE->History, Text, NULL);
    }

}

int LineEditHandleChar(TLineEdit *LE, int Char)
{
    char *Tempstr=NULL;
    char *ptr;

    switch (Char)
    {
    //seems like control-c sends this
    case STREAM_NODATA:
    case ESCAPE:
        return(LINE_EDIT_CANCEL);
        break;

    case STREAM_TIMEOUT:
        break;

    case TKEY_UP:
        if (LE->History)
        {
            if (LE->History->Side)
            {
                if (LE->History->Side != ListGetNext(LE->History)) LE->History->Side=ListGetPrev(LE->History->Side);
            }
            else LE->History->Side=ListGetLast(LE->History);

            if (LE->History->Side) LineEditSetText(LE, LE->History->Side->Tag);
        }
        break;

    case TKEY_DOWN:
        if (LE->History)
        {
            if (LE->History->Side)
            {
                LE->History->Side=ListGetNext(LE->History->Side);
                if (LE->History->Side) LineEditSetText(LE, LE->History->Side->Tag);
                //if ListGetNext returned NULL then we must be at the end of the history
                else LineEditSetText(LE, "");
            }
        }
        break;

    case TKEY_LEFT:
        if ((! (LE->Flags & LINE_EDIT_NOMOVE)) && (LE->Cursor > 0)) LE->Cursor--;
        break;

    case TKEY_RIGHT:
        if ((! (LE->Flags & LINE_EDIT_NOMOVE)) && (LE->Cursor < LE->Len)) LE->Cursor++;
        break;

    case TKEY_HOME:
        if ((! (LE->Flags & LINE_EDIT_NOMOVE)) && (LE->Cursor < LE->Len)) LE->Cursor=0;
        break;

    case TKEY_END:
        if ((! (LE->Flags & LINE_EDIT_NOMOVE)) && (LE->Cursor < LE->Len)) LE->Cursor=LE->Len;
        break;

    case TKEY_DELETE:
        if (LE->Cursor < LE->Len)
        {
            ptr=LE->Line + LE->Cursor;
            if (LE->Cursor < LE->Len) memmove(ptr, ptr +1, LE->Len - LE->Cursor);
            LE->Len--;
            StrTrunc(LE->Line, LE->Len);
        }
        break;

    case TKEY_BACKSPACE:
    case TKEY_ERASE: //this is 'del'
        if (LE->Cursor > 0)
        {
            LE->Cursor--;
            ptr=LE->Line + LE->Cursor;
            if (LE->Cursor < LE->Len) memmove(ptr, ptr +1, LE->Len - LE->Cursor);
            LE->Len--;
            StrTrunc(LE->Line, LE->Len);
        }
        break;

    case TKEY_ENTER:
        if (LE->History) LE->History->Side=NULL;
        return(LINE_EDIT_ENTER);
        break;

    default:
        if (LE->Cursor == LE->Len) LE->Line=AddCharToBuffer(LE->Line, LE->Cursor, Char);
        else
        {
            Tempstr=CopyStrLen(Tempstr, LE->Line, LE->Cursor);
            Tempstr=AddCharToBuffer(Tempstr, LE->Cursor, Char);
            Tempstr=CatStr(Tempstr, LE->Line + LE->Cursor);
            LE->Line=CopyStr(LE->Line, Tempstr);
        }
        LE->Cursor++;
        LE->Len++;
        break;
    }

    Destroy(Tempstr);

    return(LE->Cursor);
}


char *LineEditDone(char *RetStr, TLineEdit *LE)
{
    RetStr=CopyStr(RetStr, LE->Line);
    if (LE->Flags & LINE_EDIT_HISTORY) LineEditAddToHistory(LE,  LE->Line);
    LE->Line=CopyStr(LE->Line, "");
    LE->Len=0;
    LE->Cursor=0;

    return(RetStr);
}
