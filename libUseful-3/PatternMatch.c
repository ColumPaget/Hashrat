#include "PatternMatch.h"

typedef enum {MATCH_FAIL, MATCH_FOUND, MATCH_ONE, MATCH_MANY, MATCH_REPEAT, MATCH_NEXT, MATCH_QUOT, MATCH_START, MATCH_CHARLIST, MATCH_HEX, MATCH_OCTAL, MATCH_SWITCH_ON, MATCH_SWITCH_OFF} TPMatchElements;

//Gets Called recursively
int pmatch_char(const char **P_PtrPtr, const char **S_PtrPtr, int *Flags);
int pmatch_search(const char **P_PtrPtr, const char **S_PtrPtr, const char *S_End, const char **Start, const char **End, int *Flags);


int pmatch_ascii(const char *P_Ptr, char S_Char,int Type)
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
    case 'C':
        NewFlag=PMATCH_NOCASE;
        OnOrOff= !OnOrOff;
        break;
    case 'W':
        NewFlag=PMATCH_NOWILDCARDS;
        OnOrOff= !OnOrOff;
        break;
    case 'X':
        NewFlag=PMATCH_NOEXTRACT;
        OnOrOff= !OnOrOff;
        break;

    case 'N':
        NewFlag=PMATCH_NEWLINEEND;
        break;
    }

    if (OnOrOff) *Flags |= NewFlag;
    else *Flags &= ~NewFlag;

}


int pmatch_quot_analyze(char P_Char, char S_Char)
{
    int result=MATCH_FAIL;

    switch (P_Char)
    {
    case 'a':
        if (S_Char==7) result=MATCH_ONE;
        break;
    case 'b':
        if (S_Char=='\b') result=MATCH_ONE;
        break;
    case 'd':
        if (S_Char==127) result=MATCH_ONE;
        break;
    case 'e':
        if (S_Char==27) result=MATCH_ONE;
        break; //escape
    case 'n':
        if (S_Char=='\n') result=MATCH_ONE;
        break;
    case 'r':
        if (S_Char=='\r') result=MATCH_ONE;
        break;
    case 't':
        if (S_Char=='	') result=MATCH_ONE;
        break;
    case 'l':
        if (islower(S_Char)) result=MATCH_ONE;
        break;
    case 'x':
        result=MATCH_HEX;
        break;
    case 'A':
        if (isalpha(S_Char)) result=MATCH_ONE;
        break;
    case 'B':
        if (isalnum(S_Char)) result=MATCH_ONE;
        break;
    case 'C':
        if (isprint(S_Char)) result=MATCH_ONE;
        break;
    case 'D':
        if (isdigit(S_Char)) result=MATCH_ONE;
        break;
    case 'S':
        if (isspace(S_Char)) result=MATCH_ONE;
        break;
    case 'P':
        if (ispunct(S_Char)) result=MATCH_ONE;
        break;
    case 'X':
        if (isxdigit(S_Char)) result=MATCH_ONE;
        break;
    case 'U':
        if (isupper(S_Char)) result=MATCH_ONE;
        break;
    case '+':
        result=MATCH_SWITCH_ON;
        break;
    case '-':
        result=MATCH_SWITCH_OFF;
        break;

    default:
        if (S_Char==P_Char) result=MATCH_ONE;
        break;
    }

    return(result);
}



int pmatch_quot(const char **P_PtrPtr, const char **S_PtrPtr, int *Flags)
{
    int result=MATCH_FAIL, OldFlags;
    char P_Char, S_Char;
    const char *OldPos, *ptr;

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

        if (result==MATCH_FAIL)
        {
            *P_PtrPtr=OldPos;
            *Flags=OldFlags;
        }
        return(result);
        break;

    case MATCH_FAIL:
        return(MATCH_FAIL);
        break;
    }

    return(MATCH_ONE);
}


#define CHARLIST_NOT 1

int pmatch_charlist(const char **P_PtrPtr, char S_Char, int Flags)
{
    char P_Char, Prev_Char=0;
    int result=MATCH_FAIL;
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
        if (result==MATCH_ONE) result=MATCH_FAIL;
        else result=MATCH_ONE;
    }

    return(result);
}



int pmatch_repeat(const char **P_PtrPtr, const char **S_PtrPtr, const char *S_End, int *Flags)
{
    char *SubPattern=NULL;
    const char *ptr;
    char *sp_ptr;
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
            result=pmatch_search((const char **) &sp_ptr, S_PtrPtr, S_End, NULL, NULL, &val);
            if (result==MATCH_FAIL) break;
        }
    }

    (*S_PtrPtr)--;
    (*P_PtrPtr)++;
    DestroyString(SubPattern);
    return(result);
}




int pmatch_char(const char **P_PtrPtr, const char **S_PtrPtr, int *Flags)
{
    char P_Char, S_Char;
    const char *P_Start;
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
    case '\0':
        result=MATCH_FOUND;
        break;
    case '|':
        result=MATCH_FOUND;
        break;
    case '*':
        result=MATCH_MANY;
        break;
    case '?':
        (*P_PtrPtr)++;
        result=MATCH_ONE;
        break;
    case '^':
        (*P_PtrPtr)++;
        result=MATCH_START;
        break;
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

    case '[':
        (*P_PtrPtr)++;
        result=pmatch_charlist(P_PtrPtr,S_Char,*Flags);
        break;

    case '{':
        (*P_PtrPtr)++;
        result=MATCH_REPEAT;
        break;

    case '\\':
        (*P_PtrPtr)++;
        result=pmatch_quot(P_PtrPtr, S_PtrPtr, Flags);
        break;

    default:
        (*P_PtrPtr)++;
        if (P_Char==S_Char) result=MATCH_ONE;
        else result=MATCH_FAIL;
        break;
    }

    if (result==MATCH_FAIL) *(P_PtrPtr)=P_Start;

    return(result);
}


int pmatch_many(const char **P_PtrPtr, const char **S_PtrPtr, const char *S_End, const char **MatchStart, const char **MatchEnd, int *Flags)
{
    const char *P_Ptr, *S_Ptr;
    int result;

    if (MatchStart) *MatchStart=NULL;
    if (MatchEnd) *MatchEnd=NULL;

    P_Ptr=(*P_PtrPtr)+1;
    if (*P_Ptr=='\0')
    {
        //we have a terminating '*' so consume all string
        while (**S_PtrPtr != '\0') (*S_PtrPtr)++;
        *P_PtrPtr=P_Ptr;
        return(MATCH_ONE);
    }

    for (S_Ptr=*S_PtrPtr; *S_Ptr != '\0'; S_Ptr++)
    {
        P_Ptr=(*P_PtrPtr)+1;
        //out of pattern, must be a match!
        result=pmatch_search(&P_Ptr, &S_Ptr, S_End, MatchStart, MatchEnd, Flags);


        if (result == MATCH_ONE)
        {
            *P_PtrPtr=P_Ptr;
            *S_PtrPtr=S_Ptr;
            return(MATCH_ONE);
        }
    }
    return(MATCH_FAIL);
}


//Somewhat ugly, as we need to iterate through the string, so we need it passed as a **
int pmatch_search(const char **P_PtrPtr, const char **S_PtrPtr, const char *S_End, const char **MatchStart, const char **MatchEnd, int *Flags)
{
    const char *ptr, *S_Start, *P_tmp, *S_tmp;
    int result;

    if (MatchStart) *MatchStart=NULL;
    if (MatchEnd) *MatchEnd=NULL;

    if (*Flags & PMATCH_SUBSTR)
    {
        (*Flags) &= ~PMATCH_SUBSTR;
        (*Flags) |= PMATCH_SHORT;
    }

    S_Start=*S_PtrPtr;
    result=pmatch_char(P_PtrPtr, S_PtrPtr, Flags);
    while (*S_PtrPtr < S_End)
    {
        switch (result)
        {
        case MATCH_FAIL:
            return(MATCH_FAIL);
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
            result=pmatch_many(P_PtrPtr, S_PtrPtr, S_End, MatchStart, MatchEnd, Flags);
            if (result==MATCH_FAIL) return(MATCH_FAIL);
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
            result=pmatch_repeat(P_PtrPtr, S_PtrPtr, S_End, Flags);
            if (result==MATCH_FAIL) return(MATCH_FAIL);
            break;
        }


        if (result==MATCH_NEXT) /*do nothing */ ;
        else if (result==MATCH_FOUND) continue;
        else if (! (*Flags & PMATCH_NOEXTRACT))
        {
            if (MatchStart && (*S_PtrPtr >= S_Start) && (! *MatchStart)) *MatchStart=*S_PtrPtr;
            if (MatchEnd && ((*(S_PtrPtr)+1) < S_End)) *MatchEnd=(*S_PtrPtr)+1;
        }

        //Handle 'MATCH_FOUND' in the switch statement, don't iterate further through Pattern or String

        if (**S_PtrPtr != '\0') (*S_PtrPtr)++;

        result=pmatch_char(P_PtrPtr, S_PtrPtr, Flags);
    }

//any number of '*' at the end of the pattern don't count once we've run out of string
//? however does, because '?' is saying 'one character' whereas '*' is zero or more
//while (*P_Ptr=='*') P_Ptr++;

//if pattern not exhausted then we didn't get a match
    if (**P_PtrPtr !='\0') return(MATCH_FAIL);

    return(MATCH_ONE);
}




//Wrapper around pmatch_search to make it more user friendly
int pmatch_one(const char *Pattern, const char *String, int len, const char **Start, const char **End, int Flags)
{
    const char *P_Ptr, *S_Ptr, *S_End;
    int result;

    if (! String) return(FALSE);
    if (! Pattern) return(FALSE);

    P_Ptr=Pattern;
    S_Ptr=String;
    S_End=String+len;
    if (Start) *Start=NULL;
    if (End) *End=NULL;

    result=pmatch_search(&P_Ptr, &S_Ptr, S_End, Start, End, &Flags);
    if ((result == MATCH_ONE) || (result == MATCH_FOUND))
    {
        if ((Flags & PMATCH_SHORT) || (*S_Ptr=='\0'))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}




int pmatch_process(const char **Compiled, const char *String, int len, ListNode *Matches, int Flags)
{
//p_ptr points to the pattern from 'Compiled' that's currently being
//tested. s_ptr holds our progress through the string
    const char **p_ptr;
    const char *Start=NULL, *End=NULL;
    int result;
    TPMatch *Match;
    int NoOfItems=0;

    for (p_ptr=Compiled; *p_ptr != NULL; p_ptr++)
    {
        if (pmatch_one(*p_ptr, String, len, &Start, &End, Flags))
        {
            NoOfItems++;
            if (Matches)
            {
                Match=(TPMatch *) calloc(1, sizeof(TPMatch));
                Match->Start=Start;
                Match->End=End;
                ListAddItem(Matches,Match);
            }
        }
    }

    return(NoOfItems);
}



void pmatch_compile(const char *Pattern, const char ***Compiled)
{
    int NoOfRecords=0, NoOfItems=0;
    const char *ptr;

    ptr=Pattern;

    while (ptr && (*ptr != '\0'))
    {
        //while ((*ptr=='?') || (*ptr=='*')) ptr++;
        NoOfItems++;
        if (NoOfItems >= NoOfRecords)
        {
            NoOfRecords+=10;
            *Compiled=(const char **) realloc(*Compiled,NoOfRecords*sizeof(char *));
        }
        (*Compiled)[NoOfItems-1]=ptr;
        while ((*ptr !='\0') && (*ptr != '|')) ptr++;
        if (*ptr=='|') ptr++;
    }

    *Compiled=(const char **) realloc(*Compiled,(NoOfRecords+1)*sizeof(char *));
    (*Compiled)[NoOfItems]=NULL;
}



int pmatch(const char *Pattern, const char *String, int Len, ListNode *Matches, int Flags)
{
    const char **Compiled=NULL;
    const char *s_ptr, *s_end;
    int result=0;

    pmatch_compile(Pattern,&Compiled);

    //We handle PMATCH_SUBSTR in this function
    s_end=String+Len;

    if (Flags & PMATCH_SUBSTR)
    {
        for (s_ptr=String; s_ptr < s_end; s_ptr++)
        {
            result+=pmatch_process((const char **) Compiled, s_ptr, s_end-s_ptr, Matches, Flags);
            Flags |= PMATCH_NOTSTART;
        }
    }
    else result=pmatch_process((const char **) Compiled, String, Len, Matches, Flags);

    /*
    				//if we allow matches to overlap, then we'll check for a match
    				//at every position, otherwise we jump to end of this match
    				if (! (Flags & PMATCH_OVERLAP))
    				{
    					s_ptr=End;
    					s_ptr--;
    				}
    */

    if (Compiled) free(Compiled);
    return(result);
}
