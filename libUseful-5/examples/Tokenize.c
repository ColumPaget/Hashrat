#include "../libUseful.h"

//demonstrates libUseful functions to break a string up into tokens 

#define STR "one two three four \"five six seven\" eight nine"

void OutputTokens(const char *Title, const char *In, int Flags)
{
char *Token=NULL;
const char *ptr;

printf("\n%s\n",Title);
ptr=GetToken(In, " ", &Token, Flags);
while (ptr)
{
printf("%s\n",Token);
ptr=GetToken(ptr, " ", &Token, Flags);
}

DestroyString(Token);
}

main()
{
printf("Results of GetToken called with different flags on the string : %s\n",STR);
OutputTokens("GetToken - no flags", STR, 0);
OutputTokens("GetToken - GETTOKEN_HONOR_QUOTES", STR, GETTOKEN_HONOR_QUOTES);
OutputTokens("GetToken - GETTOKEN_QUOTES", STR, GETTOKEN_QUOTES);
}
