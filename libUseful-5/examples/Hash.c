#include "../Hash.h"


main()
{
char *Tempstr=NULL, *Types=NULL, *Type=NULL;
const char *ptr;

Types=HashAvailableTypes(Types);
printf("HashTypes: %s\n", Types);

ptr=GetToken(Types, ",", &Type, 0);
while (ptr)
{
HashBytes(&Tempstr, Type, "testing123", 10, ENCODE_HEX);
printf("%s: %s\n", Type,  Tempstr); fflush(NULL);
ptr=GetToken(ptr, ",", &Type, 0);
}

Destroy(Tempstr);
}
