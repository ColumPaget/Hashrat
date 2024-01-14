#include "includes.h"
#include "Hash.h"
#include "HMAC.h"
#include "Encodings.h"
#include "String.h"


char *TOTP(char *RetStr, const char *HashType, const char *EncodedSecret, int SecretEncoding, int Digits, int Interval)
{
uint64_t totp=0;
uint32_t code;
char *Tempstr=NULL, *Secret=NULL;
const char *ptr;
int len, pos;

len=DecodeBytes(&Secret, EncodedSecret, SecretEncoding);
totp=(uint64_t) htonll((uint64_t) (time(NULL) / Interval));

len=HMACBytes(&Tempstr, HashType, Secret, len, (unsigned char *) &totp, sizeof(uint64_t), ENCODE_NONE);

ptr=Tempstr + len -1;
pos=*ptr & 0xf;
memcpy(&code, Tempstr + pos, 4);

code=htonl(code);
code &= 0x7fffffff;

Tempstr=FormatStr(Tempstr, "%u", code);
RetStr=CopyStrLen(RetStr, Tempstr + StrLen(Tempstr) - Digits, Digits);

Destroy(Tempstr);
Destroy(Secret);

return(RetStr);
}


char *GoogleTOTP(char *RetStr, const char *EncodedSecret)
{
return(TOTP(RetStr, "sha1", EncodedSecret, ENCODE_BASE32, 6, 30));
}
