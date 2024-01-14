/*
 * base64.c -- base-64 conversion routines.
 *
 * For license terms, see the file COPYING in this directory.
 *
 * This base 64 encoding is defined in RFC2045 section 6.8,
 * "Base64 Content-Transfer-Encoding", but lines must not be broken in the
 * scheme used here.
 */
#include <ctype.h>
#include "Encodings.h"


#define BAD	-1
#define DECODE64(base64val, c)  (isascii(c) ? base64val[c] : BAD)



void Radix64frombits(unsigned char *out, const unsigned char *in, int inlen, const char *base64digits, char pad)
/* raw bytes in quasi-big-endian order to base 64 string (NUL-terminated) */
{
    for (; inlen >= 3; inlen -= 3)
    {
        *out++ = base64digits[in[0] >> 2];
        *out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = base64digits[in[2] & 0x3f];
        in += 3;
    }

    if (inlen > 0)
    {
        unsigned char fragment;

        *out++ = base64digits[in[0] >> 2];
        fragment = (in[0] << 4) & 0x30;
        if (inlen > 1)
            fragment |= in[1] >> 4;
        *out++ = base64digits[fragment];
        *out++ = (inlen < 2) ? pad : base64digits[(in[1] << 2) & 0x3c];
        *out++ = pad;
    }
    *out = '\0';
}



void to64frombits(char *out, const char *in, int inlen)
{
    Radix64frombits(out, in, inlen, BASE64_CHARS,'=');
}




int Radix64tobits(char *out, const char *in, const char *base64digits, char pad)
/* base 64 to raw bytes in quasi-big-endian order, returning count of bytes */
{
    int len = 0, i=0;
    char base64vals[255];
    register unsigned char digit1, digit2, digit3, digit4;
    const unsigned char *ptr, *end;

    for (ptr=(const unsigned char *) base64digits; *ptr !='\0'; ptr++)
    {
        base64vals[*ptr]=i;
        i++;
    }

    ptr=in;
    end=in+StrLen(in);
    if (ptr[0] == '+' && ptr[1] == ' ') ptr += 2;
    if (*ptr == '\r') return(0);



    do
    {
        digit1 = ptr[0];
        if (DECODE64(base64vals, digit1) == BAD) return(-1);
        digit2 = ptr[1];
        if (DECODE64(base64vals, digit2) == BAD) return(-1);
        digit3 = ptr[2];
        if ((digit3 != pad) && (DECODE64(base64vals, digit3) == BAD)) return(-1);
        digit4 = ptr[3];
        if ((digit4 != pad) && (DECODE64(base64vals, digit4) == BAD)) return(-1);
        ptr += 4;
        *out++ = (DECODE64(base64vals, digit1) << 2) | (DECODE64(base64vals, digit2) >> 4);
        ++len;
        if (digit3 != pad)
        {
            *out++ = ((DECODE64(base64vals, digit2) << 4) & 0xf0) | (DECODE64(base64vals, digit3) >> 2);
            ++len;
            if (digit4 != pad)
            {
                *out++ = ((DECODE64(base64vals, digit3) << 6) & 0xc0) | DECODE64(base64vals, digit4);
                ++len;
            }
        }
    }
    while (*ptr && (*ptr != '\r') && (ptr < end) && (digit4 != pad));

    return (len);
}



int from64tobits(char *out, const char *in)
{
    return(Radix64tobits(out, in, BASE64_CHARS,'='));
}

/* base64.c ends here */
