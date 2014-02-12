#include "MathExpr.h"
	
typedef struct
{
int operator;
double value;
} ExprToken;

char *OpStrings[]={"0","+","-","*","/","(",")","%","^",NULL};
typedef enum Ops {OP_VAL,OP_PLUS,OP_MINUS,OP_TIMES,OP_DIVIDE,OP_OPEN,OP_CLOSE,OP_MOD,OP_POW} LIBUSEFUL_TMATHOPS;

char *GetMathExprToken(char *String, char **Token)
{
char *ptr, *start;
int count, found=FALSE;

ptr=String;
if (! ptr) return(NULL);
if (*ptr=='\0') return(NULL);

while (isspace(*ptr)) ptr++;
start=ptr;

while (ptr)
{
	if (*ptr=='\0')
	{

		*Token=CopyStr(*Token,start);
		break;
	}

	for (count=1; OpStrings[count] !=NULL; count++)
	{
		if (strncmp(ptr,OpStrings[count],StrLen(OpStrings[count]))==0)
		{
			// we have come to an Operator at the start, so 
			// that is our token
			if (start==ptr)
			{
				*Token=CopyStr(*Token,OpStrings[count]);					ptr+=StrLen(OpStrings[count]);
			}
			else
			{
				*Token=CopyStrLen(*Token,start,ptr-start);
			}
			found=TRUE;
			break;
		}
	}
	if (found) break;
	ptr++;
}

return(ptr);
}


double ProcessMathExpression(ListNode *Tokens);

double ProcessSumExpression(ListNode *Tokens)
{
ListNode *Curr;
ExprToken *Tok, *NextTok;
double val=0;

Curr=ListGetNext(Tokens);
Tok=(ExprToken *) Curr->Item;
val=Tok->value;
Curr=ListGetNext(Curr);
while (Curr)
{
  Tok=(ExprToken *) Curr->Item;
  Curr=ListGetNext(Curr);
  NextTok=(ExprToken *) Curr->Item;

  
  if (Tok->operator==OP_PLUS) val+=NextTok->value;
  else if (Tok->operator==OP_MINUS) val-=NextTok->value;
  else if (Tok->operator==OP_VAL) val=Tok->value;
  Curr=ListGetNext(Curr);
}
ListClear(Tokens,free);
Tokens->Next=NULL;
Tokens->Head=NULL;
Tok=(ExprToken *) calloc(1,sizeof(ExprToken));
Tok->operator=OP_VAL;
Tok->value=val;
ListAddItem(Tokens,Tok);
return(val);
}


double ProcessMultDiv(ListNode *Tokens)
{
ListNode *Curr;
ExprToken *Tok, *PrevTok, *NextTok;
double val=0;
int count;

Curr=ListGetNext(Tokens);
while (Curr)
{
  Tok=Curr->Item;
  if (
	  (Tok->operator==OP_TIMES)  || 
	  (Tok->operator==OP_DIVIDE) 
     )
  {
	PrevTok=(ExprToken *) Curr->Prev->Item;
	NextTok=(ExprToken *) Curr->Next->Item;
	if (Tok->operator==OP_TIMES) val=PrevTok->value * NextTok->value;
	if (Tok->operator==OP_DIVIDE) val=PrevTok->value / NextTok->value;
	ListDeleteNode(Curr->Prev);
	ListDeleteNode(Curr->Next);
	free(PrevTok);
	free(NextTok);
	Tok->operator=OP_VAL;
	Tok->value=val;
  }
  
  Curr=ListGetNext(Curr);
}

return(val);
}

double ProcessExpn(ListNode *Tokens)
{
ListNode *Curr;
ExprToken *Tok, *PrevTok, *NextTok;
double val=0;
int count;

Curr=ListGetNext(Tokens);
while (Curr)
{
  Tok=Curr->Item;
  if (
	  (Tok->operator==OP_MOD) ||
	  (Tok->operator==OP_POW)
     )
  {
	PrevTok=(ExprToken *) Curr->Prev->Item;
	NextTok=(ExprToken *) Curr->Next->Item;
	if (Tok->operator==OP_MOD) val=(int) PrevTok->value % (int) NextTok->value;
	if (Tok->operator==OP_POW)
	{
		val=1.0;
		for (count=0; count < NextTok->value; count++)
		{
		   val=val * PrevTok->value;
		}
	}
	ListDeleteNode(Curr->Prev);
	ListDeleteNode(Curr->Next);
	free(PrevTok);
	free(NextTok);
	Tok->operator=OP_VAL;
	Tok->value=val;
  }
  
  Curr=ListGetNext(Curr);
}

return(val);
}


void ProcessBrackets(ListNode *Tokens)
{
ListNode *Start=NULL, *Curr, *SubExpr=NULL, *SubCurr=NULL;
ExprToken *Tok;

Curr=ListGetNext(Tokens);
while (Curr)
{
  Tok=(ExprToken *) Curr->Item;
  if (Start)
  {
	if (Tok->operator==OP_CLOSE)
	{
		SubCurr->Next=NULL;
		//replace end brace with val
		Tok->operator=OP_VAL;
		Tok->value=ProcessMathExpression(SubExpr);
		Start->Next=Curr;
		Curr->Prev=Start;
		Start=NULL;
	}
	else 
	{
		SubCurr->Next=Curr;
		Curr->Prev=SubCurr;
		SubCurr=SubCurr->Next;
	}
  }
  else if (Tok->operator==OP_OPEN)
  {
	Start=Curr->Prev;
	//get rid of pesky leading brace
	ListDeleteNode(Curr);
	free(Tok);
	Curr=Start;
	SubExpr=ListCreate();
	SubCurr=SubExpr;
  } 
  Curr=ListGetNext(Curr);
}

if (SubExpr) ListDestroy(SubExpr,free);
}

double ProcessMathExpression(ListNode *Tokens)
{
double val;

ProcessBrackets(Tokens);
ProcessExpn(Tokens);
ProcessMultDiv(Tokens);
val=ProcessSumExpression(Tokens);
return(val);
}


double EvaluateMathStr(char *String)
{
double val;
char *ptr, *Token=NULL;
int operator;
ListNode *Tokens, *Curr;
ExprToken *Tok;

Tokens=ListCreate();
ptr=GetMathExprToken(String,&Token);
while (ptr)
{
   operator=MatchTokenFromList(Token,OpStrings,0);
   if (operator==-1) operator=OP_VAL;
   Tok=(ExprToken *) calloc(1,sizeof(ExprToken));
   Tok->operator=operator;
   if (operator==OP_VAL) 
   {
	   Tok->value=atof(Token);
   }
   ListAddItem(Tokens,Tok);
   ptr=GetMathExprToken(ptr,&Token);
}

val=ProcessMathExpression(Tokens);

ListDestroy(Tokens,free);
DestroyString(Token);

return(val);
}

