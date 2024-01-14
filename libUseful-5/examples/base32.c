#include "../libUseful.h"

#define HELLO "JBSWY3DPEE======"

main(int argc, char *argv[])
{
char *Chars=NULL;


/*
if (argc < 2) printf("ERROR: no test string");
else 
DecodeBytes(&Chars, argv[1], ENCODE_BASE32);
printf("%s\n", Chars);
*/

Chars=EncodeBytes(Chars, argv[1], StrLen(argv[1]), ENCODE_BASE32);
printf("%s\n", Chars);
Chars=EncodeBytes(Chars, argv[1], StrLen(argv[1]), ENCODE_CBASE32);
printf("%s\n", Chars);
Chars=EncodeBytes(Chars, argv[1], StrLen(argv[1]), ENCODE_WBASE32);
printf("%s\n", Chars);
Chars=EncodeBytes(Chars, argv[1], StrLen(argv[1]), ENCODE_HBASE32);
printf("%s\n", Chars);
Chars=EncodeBytes(Chars, argv[1], StrLen(argv[1]), ENCODE_ZBASE32);
printf("%s\n", Chars);


}
