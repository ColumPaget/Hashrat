#ifndef BASE_64_H
#define BASE_64_H

#ifdef __cplusplus
extern "C" {
#endif

void Radix64frombits(unsigned char *out, const unsigned char *in, int inlen, const char *base64digits, char pad);
int Radix64tobits(char *out, const char *in, const char *base64digits, char pad);

void to64frombits(unsigned char *out, const unsigned char *in, int inlen);
int from64tobits(char *out, const char *in);

#ifdef __cplusplus
}
#endif


#endif
