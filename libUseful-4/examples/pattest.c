#include "../libUseful.h"

#define Str "made glorious summer by this son of york"

void Check(const char *Pattern)
{
ListNode *Matches;
int result;

Matches=ListCreate();

result=pmatch(Pattern, Str, StrLen(Str), Matches, 0);
printf("[%s] %d %d matches\n",Pattern, result, ListSize(Matches));

}

main()
{

//Pattern=CopyStr(Pattern, "* new high-speed USB device * using xhci_hcd");
//Str=CopyStr(Str,"usb 4-12: new high-speed USB device number 4 using xhci_hcd");


Check("*glorious*");
Check("glorious");
Check("*glorious");
Check("*xlorious*");
Check("*[Ggx]lorious*");
Check("*?lorious*");
Check(Str);
}
