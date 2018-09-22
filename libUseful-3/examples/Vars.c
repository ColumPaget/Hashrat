#include "../libUseful.h"

//simple program to demostrate how Vars work in libUseful

main()
{
ListNode *Vars;
char *Tempstr=NULL;

Vars=ListCreate();
SetVar(Vars, "arg1", "abcdefg");
SetVar(Vars, "arg2", "12345");

Tempstr=SubstituteVarsInString(Tempstr,"arg1=$(arg1) arg2=$(arg2)", Vars,0);
printf("%s\n",Tempstr);

DestroyString(Tempstr);
}
