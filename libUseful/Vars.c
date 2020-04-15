#include "includes.h"
#include "defines.h"
#include "List.h"
#include "Time.h"

ListNode *SetDetailVar(ListNode *Vars, const char *Name, const char *Data, int ItemType, time_t Time)
{
    ListNode *Node;

    if (! Vars) return(NULL);
    Node=ListFindTypedItem(Vars,ItemType,Name);
    if (Node) Node->Item=(void *) CopyStr((char *) Node->Item,Data);
    else Node=ListAddTypedItem(Vars,ItemType,Name,CopyStr(NULL,Data));

//    Node->ItemType=ItemType;
    if (Time) ListNodeSetTime(Node, Time);
    return(Node);
}


ListNode *SetTypedVar(ListNode *Vars, const char *Name, const char *Data, int ItemType)
{
    return(SetDetailVar(Vars, Name, Data, ItemType, 0));
}


ListNode *SetVar(ListNode *Vars, const char *Name, const char *Data)
{
    return(SetDetailVar(Vars, Name, Data,0, 0));
}


ListNode *FindTypedVar(ListNode *Vars, const char *Name, int Type)
{
    ListNode *Node;

    Node=ListFindTypedItem(Vars, Type, Name);
    if (Node)
    {
        if ((Vars->Flags & LIST_FLAG_TIMEOUT) && (ListNodeGetTime(Node) > 0) && (ListNodeGetTime(Node) < GetTime(TIME_CACHED)))
        {
            DestroyString((char *) Node->Item);
            ListDeleteNode(Node);
            return(NULL);
        }
        return(Node);
    }
    return(NULL);
}


const char *GetTypedVar(ListNode *Vars, const char *Name, int Type)
{
    ListNode *Node;

    Node=FindTypedVar(Vars, Name, Type);
    if (Node) return((const char *) Node->Item);
    return("");
}

void UnsetTypedVar(ListNode *Vars, const char *Name, int Type)
{
    ListNode *Node;

    if (! Vars) return;
    Node=FindTypedVar(Vars, Name, Type);
    if (Node)
    {
        DestroyString(Node->Item);
        ListDeleteNode(Node);
    }
}


void ClearVars(ListNode *Vars)
{
    ListNode *Curr;

    if (! Vars) return;
    Curr=ListGetNext(Vars);
    while (Curr)
    {
        DestroyString(Curr->Item);
        ListDeleteNode(Curr);
        Curr=ListGetNext(Curr);
    }


}



void CopyVars(ListNode *Dest, ListNode *Source)
{
    ListNode *Curr;

    if (! Dest) return;
    if (! Source) return;
    Curr=ListGetNext(Source);
    while (Curr)
    {
        SetTypedVar(Dest,Curr->Tag,Curr->Item,Curr->ItemType);
        Curr=ListGetNext(Curr);
    }

}



char *ParseVar(char *Buff, const char **Line, ListNode *LocalVars, int Flags)
{
    char *VarName=NULL, *OutStr=NULL, *Tempstr=NULL;
    const char *ptr, *vptr;

    OutStr=Buff;
    ptr=*Line;

    switch (*ptr)
    {
    //the var name is itself a var, dereference that first
    case '$':
        ptr++;
        Tempstr=ParseVar(Tempstr,&ptr,LocalVars,Flags);
        break;

    //Vars in brackets
    case '(':
        ptr++;
        while ((*ptr !=')') && (*ptr !='\0'))
        {
            if (*ptr=='$')
            {
                ptr++;
                Tempstr=ParseVar(Tempstr,&ptr,LocalVars,Flags);
                VarName=CatStr(VarName,Tempstr);
            }
            else VarName=AddCharToStr(VarName,*ptr);
            ptr++;
        }
        break;

    //vars seperated by spaces
    default:
        ptr++;
        while ((! isspace(*ptr)) && (*ptr !='\0'))
        {
            if (*ptr=='$')
            {
                ptr++;
                Tempstr=ParseVar(Tempstr,&ptr,LocalVars,Flags);
                VarName=CatStr(VarName,Tempstr);
            }
            else VarName=AddCharToStr(VarName,*ptr);
            ptr++;
        }
        break;
    }


    *Line=ptr; //very important! Otherwise the calling process will not
    //know we have consumed some of the text!

    //Now lookup var/format/append to output
    if (! (Flags & SUBS_CASE_VARNAMES)) strlwr(VarName);

    vptr=GetVar(LocalVars,VarName);
		if (vptr)
		{

		if (Flags & SUBS_SHELL_SAFE) 
		{
			//naughty reuse of VarName to hold altered string
			VarName=MakeShellSafeString(VarName, vptr, 0);
			vptr=VarName;
		}

    if (Flags & SUBS_STRIP_VARS_WHITESPACE)
    {
        Tempstr=CopyStr(Tempstr,vptr);
        StripTrailingWhitespace(Tempstr);
        StripLeadingWhitespace(Tempstr);
				vptr=Tempstr;
    }

    if (Flags & SUBS_QUOTE_VARS) OutStr=MCatStr(OutStr,"'",vptr,"'",NULL);
    else OutStr=CatStr(OutStr, vptr);
		}

    DestroyString(VarName);
    DestroyString(Tempstr);

    return(OutStr);
}


char SubstituteQuotedChar(char InChar)
{
    switch (InChar)
    {
    case '0':
        return(0);
        break;
    case 'a':
        return(7);
        break;
    case 'b':
        return(8);
        break;
    case 'd':
        return(127);
        break;
    case 'e':
        return(27);
        break;
    case 'f':
        return(12);
        break;
    case 't':
        return('	');
        break;
    case 'r':
        return('\r');
        break;
    case 'n':
        return('\n');
        break;
    }

    return(InChar);
}


char *SubstituteVarsInString(char *Buffer, const char *Fmt, ListNode *Vars, int Flags)
{
    char *ReturnStr=NULL, *VarName=NULL, *Tempstr=NULL;
    const char *FmtPtr;
    int len=0;

    ReturnStr=CopyStr(Buffer,"");

    if (! Fmt) return(ReturnStr);


    FmtPtr=Fmt;
    while (*FmtPtr !=0)
    {
        switch (*FmtPtr)
        {
        case '\\':
            if (Flags & SUBS_INTERPRET_BACKSLASH)
            {
                FmtPtr++;
                ReturnStr=AddCharToBuffer(ReturnStr, len++, SubstituteQuotedChar(*FmtPtr));
            }
            else
            {
                ReturnStr=AddCharToBuffer(ReturnStr,len++,*FmtPtr);
            }
            break;

        case '$':
            FmtPtr++;
						StrLenCacheAdd(ReturnStr, len);
            ReturnStr=ParseVar(ReturnStr, &FmtPtr, Vars, Flags);
            len=StrLen(ReturnStr);
            break;

        case '"':
            if (Flags & SUBS_QUOTES)
            {
                FmtPtr++;
                while (*FmtPtr && (*FmtPtr !='"'))
                {
                    ReturnStr=AddCharToBuffer(ReturnStr,len++,*FmtPtr);
                    FmtPtr++;
                }
            }
            else ReturnStr=AddCharToBuffer(ReturnStr,len++,*FmtPtr);
            break;

        default:
            ReturnStr=AddCharToBuffer(ReturnStr,len++,*FmtPtr);
            break;
        }

        FmtPtr++;
    }


		StrLenCacheAdd(ReturnStr, len);
    DestroyString(Tempstr);
    DestroyString(VarName);
    return(ReturnStr);
}






void ExtractVarsReadVar(const char **Fmt, const char **Msg, ListNode *Vars)
{
    const char *FmtPtr, *MsgPtr;
    char *VarName=NULL;
    int len=0;
    ListNode *Node;

    FmtPtr=*Fmt;

    if (*FmtPtr=='(') FmtPtr++;
    while (*FmtPtr != ')')
    {
        VarName=AddCharToBuffer(VarName,len,*FmtPtr);
        len++;
        FmtPtr++;
    }
    if (*FmtPtr==')') FmtPtr++;

    MsgPtr=*Msg;
    while ((*MsgPtr !=0) && (*MsgPtr != *FmtPtr))
    {
        if ((*MsgPtr=='"') || (*MsgPtr=='\'')) MsgPtr=traverse_quoted(MsgPtr);
        MsgPtr++;
    }

    Node=ListFindNamedItem(Vars,VarName);

    if (Node) Node->Item=(void *) CopyStrLen((char *) Node->Item, *Msg, MsgPtr-*Msg);
    else Node=ListAddNamedItem(Vars,VarName,CopyStrLen(NULL, *Msg, MsgPtr-*Msg));

    *Fmt=FmtPtr;
    *Msg=MsgPtr;

    DestroyString(VarName);
}

char *ExtractVarsGetLiteralString(char *Buffer, const char *InStr)
{
    char *RetStr;
    const char *ptr;

    RetStr=Buffer;

    ptr=InStr;
    while ((*ptr !=0) && (*ptr !='$') && (*ptr !='?') && (*ptr !='*')) ptr++;

    RetStr=CopyStrLen(Buffer,InStr,ptr-InStr);
    return(RetStr);
}

int ExtractVarsFromString(const char *Data, const char *FormatStr, ListNode *Vars)
{
    const char *FmtPtr, *MsgPtr;
    char *Token=NULL;
    int Match=TRUE, len;

    FmtPtr=FormatStr;
    MsgPtr=Data;
    while ( (*FmtPtr != '\0') && (Match))
    {
        switch (*FmtPtr)
        {
        case '?':
            FmtPtr++;
            MsgPtr++;
            break;

        case '*':
            FmtPtr++;
            Token=ExtractVarsGetLiteralString(Token,FmtPtr);
            len=StrLen(Token);
            while (
                (*MsgPtr != '\0') &&
                (strncmp(MsgPtr,Token,len) !=0)
            ) MsgPtr++;
            break;

        case '$':
            FmtPtr++;
            ExtractVarsReadVar(&FmtPtr, &MsgPtr, Vars);
            break;

        default:
            if (*FmtPtr != *MsgPtr)
            {
                Match=FALSE;
            }
            FmtPtr++;
            MsgPtr++;
            break;
        }

    }
    DestroyString(Token);

    return(Match);
}




int FindVarNamesInString(const char *Data, ListNode *Vars)
{
const char *ptr;
char *Name=NULL;

for (ptr=Data; *ptr !='\0'; ptr++)
{
  if ((*ptr=='$') && (*(ptr+1)=='('))
  {
  ptr=GetToken(ptr+2, ")", &Name, 0);
  SetVar(Vars, Name, "");
  }
}

Destroy(Name);

return(ListSize(Vars));
}

