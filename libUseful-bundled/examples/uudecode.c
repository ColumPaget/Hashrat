#include "../libUseful.h"

#define HELLO "JBSWY3DPEE======"


main(int argc, char *argv[])
{
char *Chars=NULL;
//char *Input="K:'1T<',Z+R]W=W<N>6]U='5B92YC;VTO=V%T8V@_=CUH26XQ3F8S8F1D40``";
char *Input="K:'1T<";
int len;

len=DecodeBytes(&Chars, Input, ENCODE_UUENC);
printf("%s\n", Chars);

Chars=EncodeBytes(Chars, "Cat", 3, ENCODE_UUENC);
printf("%s\n", Chars);
}
