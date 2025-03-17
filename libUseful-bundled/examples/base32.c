#include "../libUseful.h"

#define HELLO "JBSWY3DPEE======"

main(int argc, char *argv[])
{
char *Chars=NULL, *Bytes=NULL;
int len;

if (argc < 2) 
{
printf("ERROR: no test string, please supply one on the command-line\n");
exit(1);
}

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


len=DecodeBytes(&Bytes, "7e7e9c42a91bfef19fa929e5fda1b72e0ebc1a4c1141673e2794234d86addf4e", ENCODE_HEX);
Chars=EncodeBytes(Chars, Bytes, len, ENCODE_BECH32);
printf("%s\n", Chars);



}
