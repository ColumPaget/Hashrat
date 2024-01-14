#include "../libUseful.h"

//this program demostrates some aspects of the libUseful error system


//this function should generate two errors
int TestFunction()
{
STREAM *S;

// by default failed network connections raise errors. However, if you uncomment 'LibUsefulSetValue("Error:IgnoreIP","true");' 
// in main, then network errors will be ignored, and we will have to pass 'E' in the second argument to STREAMOpen here
S=STREAMOpen("tcp://nonexistenthost.fdafelfhgexy.com","");
STREAMClose(S);

// note use of 'E' flag to cause an error to be raised if this file files to open
S=STREAMOpen("/tmp/nonexistentfile","rE");
STREAMClose(S);

return(FALSE);
}


void HandleErrors()
{
ListNode *Errors, *Curr;
TError *Err;

Errors=ErrorsGet();
Curr=ListGetNext(Errors);
while (Curr)
{
Err=(TError *) Curr->Item;
printf("Error during %s at line %d in file '%s':  '%s'. errno was %d '%s' \n",Err->where, Err->line, Err->file, Err->msg, Err->errval, strerror(Err->errval));
Curr=ListGetNext(Curr);
}

fflush(NULL);
exit(1);
}

main()
{

//Suppress libUseful's own error print out, we are going to handle and print ouf the errors ourselves
LibUsefulSetValue("Error:Silent","true");

//Setting this causes network errors to be ignored
//LibUsefulSetValue("Error:IgnoreIP","true");

if (! TestFunction()) HandleErrors();


}
