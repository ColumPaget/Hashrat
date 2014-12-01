#include "libUseful.h"

main()
{
char *Token=NULL, *ptr;


ptr=GetToken("this is 'a test of' gettoken"," ",&Token,GETTOKEN_QUOTES);
while (ptr)
{
printf("TOK: %s\n",Token);

ptr=GetToken(ptr," ",&Token,GETTOKEN_QUOTES);
}

ptr=GetToken("this is,a test;of gettoken"," |,|;",&Token,GETTOKEN_MULTI_SEPARATORS);
while (ptr)
{
printf("TOK: %s [%s]\n",Token,ptr);

ptr=GetToken(ptr," |,|;",&Token,GETTOKEN_MULTI_SEPARATORS);
}
}
