#include "includes.h"
#include "Hash.h"
#include "HMAC.h"
#include "Encodings.h"
#include "String.h"

int TOTPAtTime(char **RetStr, const char *HashType, const char *EncodedSecret, int SecretEncoding, time_t When, int Digits, int Interval)
{
    uint64_t totp=0;
    uint32_t code;
    char *Tempstr=NULL, *Secret=NULL;
    const char *ptr;
    int len, pos;

    len=DecodeBytes(&Secret, EncodedSecret, SecretEncoding);
    totp=(uint64_t) htonll((uint64_t) (When / Interval));

    len=HMACBytes(&Tempstr, HashType, Secret, len, (char *) &totp, sizeof(uint64_t), ENCODE_NONE);

    ptr=Tempstr + len -1;
    pos=*ptr & 0xf;
    memcpy(&code, Tempstr + pos, 4);

    code=htonl(code);
    code &= 0x7fffffff;

    Tempstr=FormatStr(Tempstr, "%u", code);
    *RetStr=CopyStrLen(*RetStr, Tempstr + StrLen(Tempstr) - Digits, Digits);

    Destroy(Tempstr);
    Destroy(Secret);

    return(When % Interval);
}



int TOTPPrevCurrNext(char **Prev, char **Curr, char **Next, const char *HashType, const char *EncodedSecret, int SecretEncoding, int Digits, int Interval)
{
    time_t When;
    When=time(NULL);

    TOTPAtTime(Prev, HashType, EncodedSecret, SecretEncoding, When - (Interval / 2 + 1), Digits, Interval);
    TOTPAtTime(Curr, HashType, EncodedSecret, SecretEncoding, When, Digits, Interval);
    TOTPAtTime(Next, HashType, EncodedSecret, SecretEncoding, When + (Interval / 2 + 1), Digits, Interval);
    return(When % Interval);
}


char *TOTP(char *RetStr, const char *HashType, const char *EncodedSecret, int SecretEncoding, int Digits, int Interval)
{
    TOTPAtTime(&RetStr, HashType, EncodedSecret, SecretEncoding, time(NULL), Digits, Interval);
    return(RetStr);
}


char *GoogleTOTP(char *RetStr, const char *EncodedSecret)
{
    return(TOTP(RetStr, "sha1", EncodedSecret, ENCODE_BASE32, 6, 30));
}
