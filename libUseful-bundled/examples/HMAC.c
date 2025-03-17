#include "../Hash.h"


void HMACTest(const char *Type, const char *Text)
{
char *Tempstr=NULL, *Output=NULL;
int len;

len=HMACBytes(&Output, Type, "key", 3, Text, StrLen(Text), ENCODE_HEX);
printf("%s: %s\n", Type, Output);

Destroy(Tempstr);
Destroy(Output);
}

main()
{
HMACTest("md5", "The quick brown fox jumps over the lazy dog");
HMACTest("sha1", "The quick brown fox jumps over the lazy dog");
HMACTest("sha256", "The quick brown fox jumps over the lazy dog");
HMACTest("sha512", "The quick brown fox jumps over the lazy dog");
}
