#include "includes.h"
#include "defines.h"
#include "list.h"

void SetVar(ListNode *Vars, const char *Name, const char *Data)
{
ListNode *Node;
char *Tempstr=NULL;

Tempstr=CopyStr(Tempstr,Name);
//strlwr(Tempstr);
Node=ListFindNamedItem(Vars,Tempstr);
if (Node) Node->Item=(void *) CopyStr((char *) Node->Item,Data);
else ListAddNamedItem(Vars,Tempstr,CopyStr(NULL,Data));

DestroyString(Tempstr);
}

char *GetVar(ListNode *Vars, const char *Name)
{
ListNode *Node;
char *Tempstr=NULL;

Tempstr=CopyStr(Tempstr,Name);
//strlwr(Tempstr);
Node=ListFindNamedItem(Vars,Tempstr);
DestroyString(Tempstr);
if (Node) return((char *) Node->Item);
return(NULL);
}


void UnsetVar(ListNode *Vars,const char *Name)
{
ListNode *Curr;
char *Str=NULL;
char *Tempstr=NULL;

if (Vars) return;
Tempstr=CopyStr(Tempstr,Name);
strlwr(Tempstr);
Curr=ListFindNamedItem(Vars,Tempstr);
if (Curr)
{
    Str=ListDeleteNode(Curr);
    DestroyString(Str);
}
DestroyString(Tempstr);
}


void ClearVars(ListNode *Vars)
{
ListNode *Curr;
char *Str;

if (! Vars) return;
Curr=ListGetNext(Vars);
while (Curr)
{
    Str=ListDeleteNode(Curr);
    DestroyString(Str);
Curr=ListGetNext(Curr);
}


}



void CopyVars(ListNode *Dest, ListNode *Source)
{
ListNode *Curr;
char *Str;

if (! Dest) return;
if (! Source) return;
Curr=ListGetNext(Source);
while (Curr)
{
SetVar(Dest,Curr->Tag,Curr->Item);
Curr=ListGetNext(Curr);
}

}



char *ParseVar(char *Buff, const char **Line, ListNode *LocalVars, int Flags)
{
char *VarName=NULL, *OutStr=NULL, *Tempstr=NULL;
const char *ptr, *vptr;

OutStr=Buff;
ptr=*Line;

if (*ptr=='(') ptr++;

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

*Line=ptr; //very important! Otherwise the calling process will not 
	   //know we have consumed some of the text!

	//Now lookup var/format/append to output
	if (! (Flags & SUBS_CASE_VARNAMES)) strlwr(VarName);

	vptr=GetVar(LocalVars,VarName);

	if (Flags & SUBS_QUOTE_VARS) OutStr=CatStr(OutStr,"'");

	if (Flags & SUBS_STRIP_VARS_WHITESPACE) 
	{
		Tempstr=CopyStr(Tempstr,vptr);
		StripTrailingWhitespace(Tempstr);
		StripLeadingWhitespace(Tempstr);
   	OutStr=CatStr(OutStr,Tempstr);
	}
  else OutStr=CatStr(OutStr, vptr);

	if (Flags & SUBS_QUOTE_VARS) OutStr=CatStr(OutStr,"'");

DestroyString(VarName);
DestroyString(Tempstr);

return(OutStr);
}



char *SubstituteVarsInString(char *Buffer, const char *Fmt, ListNode *Vars, int Flags)
{
char *ReturnStr=NULL, *VarName=NULL, *Tempstr=NULL;
const char *FmtPtr;
int count, VarIsPointer=FALSE;
ListNode *Curr;
int len=0, i;

ReturnStr=CopyStr(Buffer,"");

if (! Fmt) return(ReturnStr);


FmtPtr=Fmt;
while (*FmtPtr !=0)
{
	switch (*FmtPtr)
	{

		case '\\':
			FmtPtr++;
			switch (*FmtPtr)
			{
			   case 't':
				ReturnStr=AddCharToStr(ReturnStr,' ');
				len=StrLen(ReturnStr);
				while ((len % 4) !=0)
				{
				ReturnStr=AddCharToStr(ReturnStr,' ');
				len++;
				}
			   break;

			   case 'r':
				ReturnStr=AddCharToStr(ReturnStr,'\r');
				len++;
			   break;

			   case 'n':
				ReturnStr=AddCharToStr(ReturnStr,'\n');
				len++;
			   break;

			   default:
				ReturnStr=AddCharToStr(ReturnStr,*FmtPtr);
				len++;
			}
		break;

		case '$':
		FmtPtr++;
		/*
			if (*FmtPtr=='$')
			{
				 VarIsPointer=TRUE;
				 FmtPtr++;
			}
			if (*FmtPtr=='(') FmtPtr++;
			*/

		ReturnStr=ParseVar(ReturnStr, &FmtPtr, Vars, Flags);
 		break;

		case '"':
			FmtPtr++;
			while (*FmtPtr && (*FmtPtr !='"')) 
			{
			 	ReturnStr=AddCharToStr(ReturnStr,*FmtPtr);
				len++;
				FmtPtr++;
			}
			break;
 
		default:
			 ReturnStr=AddCharToStr(ReturnStr,*FmtPtr);
			 len++;
	}

FmtPtr++;
}


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
 if (*MsgPtr=='"')
 {
		do 
		{
			MsgPtr++;
		} while ((*MsgPtr != '"') && (*MsgPtr != 0));
 }
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

int ExtractVarsFromString(char *Data, const char *FormatStr, ListNode *Vars)
{
const char *FmtPtr, *MsgPtr;
char *Token=NULL;
int Match=TRUE, len;

FmtPtr=FormatStr;
MsgPtr=Data;

while ( (*FmtPtr !=0) && (Match))
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
                (*MsgPtr !=0) && 
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


