#include "PatternMatch.h"

typedef enum {MATCH_FAIL, MATCH_FOUND, MATCH_ONE, MATCH_MANY, MATCH_REPEAT, MATCH_NEXT, MATCH_CONT, MATCH_QUOT, MATCH_START, MATCH_CHARLIST, MATCH_HEX, MATCH_OCTAL, MATCH_SWITCH_ON, MATCH_SWITCH_OFF} TPMatchElements;

//Gets Called recursively
int pmatch_char(char **P_PtrPtr, char **S_PtrPtr, int *Flags);
int pmatch_search(char *Pattern, char **S_PtrPtr, char *S_End, char **Start, char **End, int *Flags);


int pmatch_ascii(char *P_Ptr,char S_Char,int Type)
{
char ValStr[4];
int P_Char=-1;

if (Type==MATCH_HEX) 
{
	strncpy(ValStr,P_Ptr,2);
	P_Char=strtol(P_Ptr,NULL,16);
}

if (Type==MATCH_OCTAL) 
{
	strncpy(ValStr,P_Ptr,3);
	P_Char=strtol(P_Ptr,NULL,8);
}

if (P_Char==-1) return(FALSE);

if (P_Char==S_Char) return(TRUE);

return(FALSE);
}


void pmatch_switch(char SwitchType, char SwitchOnOrOff, int *Flags)
{
int NewFlag=0, OnOrOff=FALSE;

if (SwitchOnOrOff==MATCH_SWITCH_ON) OnOrOff=TRUE;
else if (SwitchOnOrOff==MATCH_SWITCH_OFF) OnOrOff=FALSE; 
else return;

switch (SwitchType)
{
//These switches have the opposite meaning to the flags they control.
//+C turns case sensitivity on, but the flag is 'PMATCH_NOCASE' that
//turns it off, so we need to invert the sense of 'OnOrOff'
case 'C': NewFlag=PMATCH_NOCASE; OnOrOff= !OnOrOff; break;
case 'W': NewFlag=PMATCH_NOWILDCARDS; OnOrOff= !OnOrOff; break;
case 'X': NewFlag=PMATCH_NOEXTRACT; OnOrOff= !OnOrOff; break;

case 'N': NewFlag=PMATCH_NEWLINEEND; break;
}

if (OnOrOff) *Flags |= NewFlag;
else *Flags &= ~NewFlag;

}


int pmatch_quot_analyze(char P_Char, char S_Char)
{
int result=MATCH_FAIL;

switch (P_Char)
{
	case 'a': if (S_Char==7) result=MATCH_ONE; break;
	case 'b': if (S_Char=='\b') result=MATCH_ONE; break;
	case 'd': if (S_Char==127) result=MATCH_ONE; break;
	case 'e': if (S_Char==27) result=MATCH_ONE; break; //escape
	case 'n': if (S_Char=='\n') result=MATCH_ONE; break;
	case 'r': if (S_Char=='\r') result=MATCH_ONE; break;
	case 't': if (S_Char=='	') result=MATCH_ONE; break;
	case 'l': if (islower(S_Char)) result=MATCH_ONE; break;
	case 'x': result=MATCH_HEX; break;
	case 'A': if (isalpha(S_Char)) result=MATCH_ONE; break;
	case 'B': if (isalnum(S_Char)) result=MATCH_ONE; break;
	case 'C': if (isprint(S_Char)) result=MATCH_ONE; break;
	case 'D': if (isdigit(S_Char)) result=MATCH_ONE; break;
	case 'S': if (isspace(S_Char)) result=MATCH_ONE; break;
	case 'P': if (ispunct(S_Char)) result=MATCH_ONE; break;
	case 'X': if (isxdigit(S_Char)) result=MATCH_ONE; break;
	case 'U': if (isupper(S_Char)) result=MATCH_ONE; break;
	case '+': result=MATCH_SWITCH_ON; break;
	case '-': result=MATCH_SWITCH_OFF; break;
	
	default: if (S_Char==P_Char) result=MATCH_ONE; break;
}

return(result);
}



int pmatch_quot(char **P_PtrPtr, char **S_PtrPtr, int *Flags)
{
int result=MATCH_FAIL, OldFlags;
char P_Char, S_Char, *OldPos, *ptr;

P_Char=**P_PtrPtr;
S_Char=**S_PtrPtr;


result=pmatch_quot_analyze(P_Char, S_Char);

switch (result)
{
	case MATCH_ONE:
	(*P_PtrPtr)++;
	break;

	case MATCH_HEX:
		if (! pmatch_ascii((*P_PtrPtr)+1,S_Char,MATCH_HEX)) return(MATCH_FAIL);
		(*P_PtrPtr)+=2;
	break;

	case MATCH_OCTAL:
		if (! pmatch_ascii((*P_PtrPtr)+1,S_Char,MATCH_OCTAL)) return(MATCH_FAIL);
		(*P_PtrPtr)+=3;
	break;

	case MATCH_SWITCH_ON:
	case MATCH_SWITCH_OFF:
		//some switches need to be applied in order for a pattern to match 
		//(like the case-insensitive switch) others should only be applied if
		//it matches. So we apply the switch, but if the subsequent pmatch_char fails
		//we unapply it
		OldFlags=*Flags;
		OldPos=*P_PtrPtr;
		(*P_PtrPtr)++; //go past the + or - to the actual type
		pmatch_switch(**P_PtrPtr, result, Flags);
		(*P_PtrPtr)++;
		result=pmatch_char(P_PtrPtr, S_PtrPtr, Flags);

		if ((result==MATCH_FAIL) || (result==MATCH_CONT))
		{
			*P_PtrPtr=OldPos;
			*Flags=OldFlags;
		}
		return(result);
	break;

	case MATCH_FAIL:
		if (*Flags & PMATCH_SUBSTR) return(MATCH_CONT);
		return(MATCH_FAIL);
	break;
}

return(MATCH_ONE);
}


#define CHARLIST_NOT 1

int pmatch_charlist(char **P_PtrPtr,char S_Char, int Flags)
{
char P_Char, Prev_Char=0;
int result=MATCH_CONT;
int mode=0;


while (**P_PtrPtr != '\0')
{
	if (Flags & PMATCH_NOCASE) P_Char=tolower(**P_PtrPtr);
	else P_Char=**P_PtrPtr;

	if (P_Char==']') break;

	switch (P_Char)
	{
	case '\\': 
		(*P_PtrPtr)++;
		if (Flags & PMATCH_NOCASE) P_Char=tolower(**P_PtrPtr);
		else P_Char=**P_PtrPtr;
	break;

	case '-':
		(*P_PtrPtr)++;
		if (Flags & PMATCH_NOCASE) P_Char=tolower(**P_PtrPtr);
		else P_Char=**P_PtrPtr;
	
		if ((S_Char >= Prev_Char) && (S_Char <= P_Char)) result=MATCH_ONE;
	break;

	case '!':
		mode |= CHARLIST_NOT;
	break;

	default:
		if (P_Char == S_Char) result=MATCH_ONE;
	break;
	}
	
	Prev_Char=P_Char;
	(*P_PtrPtr)++;
}

//go beyond ']'
(*P_PtrPtr)++;

if (mode & CHARLIST_NOT) 
{
	if (result==MATCH_ONE) result=MATCH_CONT;
	else result=MATCH_ONE;
}

return(result);
}



int pmatch_repeat(char **P_PtrPtr, char **S_PtrPtr, char *S_End, int *Flags)
{
char *SubPattern=NULL, *sp_ptr, *ptr;
int count=0, i, val=0, result=MATCH_FAIL;

ptr=*P_PtrPtr;
while ( ((**P_PtrPtr) != '}') && ((**P_PtrPtr) != '\0') ) (*P_PtrPtr)++;
if ((**P_PtrPtr)=='\0') return(MATCH_FAIL);

SubPattern=CopyStrLen(SubPattern,ptr,*P_PtrPtr - ptr);

sp_ptr=strchr(SubPattern,'|');
if (sp_ptr) 
{
	*sp_ptr='\0';
	sp_ptr++;
	count=atoi(SubPattern);
	for (i=0; i < count; i++) 
	{
		val=0;
		result=pmatch_search(sp_ptr, S_PtrPtr, S_End, NULL, NULL, &val);
		if (result==MATCH_CONT) result=MATCH_FAIL;
		if (result==MATCH_FAIL) break;
	}
}

(*S_PtrPtr)--;
(*P_PtrPtr)++;
DestroyString(SubPattern);
return(result);
}




int pmatch_char(char **P_PtrPtr, char **S_PtrPtr, int *Flags)
{
char P_Char, S_Char, *P_Start;
int result=MATCH_FAIL;

P_Start=*P_PtrPtr;
if (*Flags & PMATCH_NOCASE)
{
	P_Char=tolower(**P_PtrPtr);
	S_Char=tolower(**S_PtrPtr);
}
else
{
	P_Char=**P_PtrPtr;
	S_Char=**S_PtrPtr;
}


//we must still honor switches even if 'nowildcards' is set, as we may want to turn
//'nowildcards' off, or turn case or extraction features on or off
if (*Flags & PMATCH_NOWILDCARDS) 
{
	if (
				(P_Char=='\\') && 
				((*(*P_PtrPtr+1)=='+') || (*(*P_PtrPtr+1)=='-'))
		) /*This is a switch, fall through and process it */ ;
	else if (P_Char==S_Char) return(MATCH_ONE);
	else return(MATCH_FAIL);
}

switch (P_Char)
{
	case '\0': result=MATCH_FOUND; break;
	case '|': result=MATCH_FOUND; break;
	case '*': result=MATCH_MANY; break;
	case '?': (*P_PtrPtr)++; result=MATCH_ONE; break;
	case '^': (*P_PtrPtr)++; result=MATCH_START; break;
	case '$':
		if (S_Char=='\0') result=MATCH_FOUND; 
		else if ((*Flags & PMATCH_NEWLINEEND) && (S_Char=='\n')) result=MATCH_FOUND;
	break;

  //This is akin to $ (match end) but matches any 'breaking' character
  case '%':
    switch (S_Char)
    {
    case '\0':
    case '\r':
    case '\n':
    case ' ':
    case '	':
    case '-':
    case '_':
    case '.':
    case ',':
    case ':':
    case ';':
        result=MATCH_FOUND;
    break;
		}
	break;

	case '[': (*P_PtrPtr)++; result=pmatch_charlist(P_PtrPtr,S_Char,*Flags); break;

	case '{': (*P_PtrPtr)++; result=MATCH_REPEAT; break;

	case '\\': 
			//results here can either be MATCH_FAIL, MATCH_CONT, MATCH_ONE 
			(*P_PtrPtr)++;
			result=pmatch_quot(P_PtrPtr, S_PtrPtr, Flags);
	break;

	default:
		(*P_PtrPtr)++;
		if (P_Char==S_Char) result=MATCH_ONE;
		else if (*Flags & PMATCH_SUBSTR) result=MATCH_CONT;
		else result=MATCH_FAIL;
	break;
}

if ((result==MATCH_CONT) || (result==MATCH_FAIL)) *(P_PtrPtr)=P_Start;

return(result);
}


//pmatch_many looks ahead to see if the next character can be included in the match
int pmatch_many(char *P_Ptr, char *S_Ptr, char *S_End, int *Flags)
{
int result=MATCH_FAIL;
char *p_ptr, *s_ptr;

//This is a LOOKAHEAD function, so we don't want to change any pointer positions
//thus we use local pointers to do all the searching
p_ptr=P_Ptr;
s_ptr=S_Ptr;

switch (*p_ptr)
{
	//we are at end of pattern. If we are also at end of string, then it's a match
	case '\0':
		if (s_ptr == S_End) return(MATCH_FOUND);
	break;

	//we have a quoted char. Could be a switch, in which case we skip over it
	case '\\':
		p_ptr++;
		result=pmatch_quot_analyze(*p_ptr, *s_ptr);
		p_ptr++;
		//just ignore switches
		if ((result==MATCH_SWITCH_ON) || (result==MATCH_SWITCH_OFF)) 
		{		
			p_ptr++;
			return(pmatch_many(p_ptr, s_ptr, S_End, Flags));
		}
	break;

	default:
		//if we've consumed all string, BUT ARE NOT AT END OF PATTERN, then we've failed to find a match
		if (s_ptr == S_End) return(MATCH_FAIL);
		else result=pmatch_char(&p_ptr, &s_ptr, Flags);

	break;
}

//if the next character is a match, then we're at the end
//of match many
if (result==MATCH_FAIL) return(MATCH_MANY);

return(result);
}



//Somewhat ugly, as we need to iterate through the string, so we need it passed as a **
int pmatch_search(char *Pattern, char **S_PtrPtr, char *S_End, char **MatchStart, char **MatchEnd, int *Flags)
{
int result;
char *P_Ptr, *ptr, *S_Start;

	if (MatchStart) *MatchStart=NULL;
	if (MatchEnd) *MatchEnd=NULL;

	P_Ptr=Pattern;
	S_Start=*S_PtrPtr;
	result=pmatch_char(&P_Ptr, S_PtrPtr, Flags);
	while (*S_PtrPtr <= S_End) 
	{
		switch (result)
		{
		case MATCH_FAIL: 
			if (*Flags & PMATCH_SUBSTR) return(MATCH_CONT); 
			return(MATCH_FAIL); 
		break;

		//Match failed, but we're looking for a substring of 'String' so continue searching for match
		case MATCH_CONT: 
			if (! (*Flags & PMATCH_SUBSTR)) return(MATCH_FAIL); 
			//if we were some ways through a pattern before hitting the character
			//that failed the match, then we must rewind to reconsider that character
			//as a fresh match
			//if ((*P_Ptr != *Pattern) && (*Pattern != '^')) (*S_PtrPtr)--;
			if ((*P_Ptr != *Pattern) ) (*S_PtrPtr)--;
			P_Ptr=Pattern; 
		break;

		case MATCH_START:
			if (*Flags & PMATCH_NOTSTART) return(MATCH_FAIL);
			(*S_PtrPtr)--; //naughty, were are now pointing before String, but the
							 //S_Ptr++ below will correct this
		break;

		
		//Match many is a special case. We have too look ahead to see if the next char
		//Matches, and thus takes us out of 'match many' howerver, If the call to pmatch_char 
		//fails then we have to rewind the pattern pointer to match many's '*' and go round again
		case MATCH_MANY:
			result=pmatch_many(P_Ptr+1, (*S_PtrPtr)+1, S_End, Flags);

			if (result == MATCH_FAIL) return(MATCH_FAIL);
			if (result == MATCH_ONE) 
			{
				result=MATCH_NEXT;
				P_Ptr++;
			}
		break;

		case MATCH_FOUND:
				if (! (*Flags & PMATCH_NOEXTRACT) )
				{
					//This is to prevent returning NULL strings in 'MatchStart' and 'MatchEnd'
					if (MatchStart && (! *MatchStart)) *MatchStart=S_Start;
					if (MatchEnd && (! *MatchEnd)) *MatchEnd=*MatchStart;
				}
			return(MATCH_ONE); 
		break;

		case MATCH_REPEAT:
			result=pmatch_repeat(&P_Ptr, S_PtrPtr, S_End, Flags);
			if (result==MATCH_FAIL)
			{
			if (*Flags & PMATCH_SUBSTR) return(MATCH_CONT); 
			return(MATCH_FAIL); 
			}
		break;
		}


		if (result==MATCH_NEXT) /*do nothing */ ;
		else if (result==MATCH_FOUND) continue;	
		else if ((result==MATCH_CONT) )
		{
			if (MatchStart) *MatchStart=NULL;
			if (MatchEnd) *MatchEnd=NULL;
		}
		else if (! (*Flags & PMATCH_NOEXTRACT))
		{
				if (MatchStart && (*S_PtrPtr >= S_Start) && (! *MatchStart)) *MatchStart=*S_PtrPtr;
				if (MatchEnd && ((*(S_PtrPtr)+1) < S_End)) *MatchEnd=(*S_PtrPtr)+1;
		}

		//Handle 'MATCH_FOUND' in the switch statement, don't iterate further through Pattern or String

		(*S_PtrPtr)++;	
		
		result=pmatch_char(&P_Ptr, S_PtrPtr, Flags);
	}

//if pattern not exhausted then we didn't get a match
if (*P_Ptr !='\0') return(MATCH_CONT);

return(MATCH_ONE);
}




//Wrapper around pmatch_search to make it more user friendly
int pmatch_one(char *Pattern, char *String, int len, char **Start, char **End, int Flags)
{
char *S_Ptr, *S_End;
int result;

if (! String) return(FALSE);
if (! Pattern) return(FALSE);

	S_Ptr=String;
	S_End=String+len;
	if (Start) *Start=NULL;
	if (End) *End=NULL;

result=pmatch_search(Pattern, &S_Ptr, S_End, Start, End, &Flags);

if (result == MATCH_ONE) return(TRUE);
if (result == MATCH_FOUND) return(TRUE);

return(FALSE);
}




int pmatch_process(char **Compiled, char *String, int len, ListNode *Matches, int iFlags)
{
//p_ptr points to the pattern from 'Compiled' that's currently being
//tested. s_ptr holds our progress through the string
char **p_ptr;
char *s_ptr, *s_end;
char *Start=NULL, *End=NULL;
int result, Flags;
TPMatch *Match;
int NoOfItems=0;

Flags=iFlags &= ~PMATCH_SUBSTR;
s_end=String+len;
//We handle PMATCH substr in this function
for (s_ptr=String; s_ptr < s_end; s_ptr++)
{
	for (p_ptr=Compiled; *p_ptr != NULL; p_ptr++)
	{
		if (pmatch_one(*p_ptr, s_ptr, s_end-s_ptr, &Start, &End, Flags))
		{
			NoOfItems++;
			if (Matches)
			{
				Match=(TPMatch *) calloc(1, sizeof(TPMatch));
				Match->Start=Start;
				Match->End=End;
				ListAddItem(Matches,Match);

				//if we allow matches to overlap, then we'll check for a match
				//at every position, otherwise we jump to end of this match
				if (! (Flags & PMATCH_OVERLAP)) 
				{
					s_ptr=End;
					s_ptr--;
				}
			}
		}
	}
	Flags |= PMATCH_NOTSTART;
}

return(NoOfItems);
}



void pmatch_compile(char *Pattern, char ***Compiled)
{
int NoOfRecords=0, NoOfItems=0;
char *ptr;

ptr=Pattern;

while (ptr && (*ptr != '\0'))
{
	//while ((*ptr=='?') || (*ptr=='*')) ptr++;
	NoOfItems++;
	if (NoOfItems >= NoOfRecords)
	{
		NoOfRecords+=10;
		*Compiled=(char **) realloc(*Compiled,NoOfRecords*sizeof(char *));

	}
	(*Compiled)[NoOfItems-1]=ptr;
	while ((*ptr !='\0') && (*ptr != '|')) ptr++;
	if (*ptr=='|') ptr++;
}

*Compiled=(char **) realloc(*Compiled,(NoOfRecords+1)*sizeof(char *));
(*Compiled)[NoOfItems]=NULL;
}



int pmatch(char *Pattern, char *String, int Len, ListNode *Matches, int Flags)
{
char *ptr, **Compiled=NULL;
int result, len;

	pmatch_compile(Pattern,&Compiled);

	ptr=String;	
	len=Len;

	result=pmatch_process(Compiled, ptr, len, Matches, Flags);

	if (Compiled) free(Compiled);
	return(result);
}
