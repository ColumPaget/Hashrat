#ifndef BASE_64_H
#define BASE_64_H

#ifdef __cplusplus
extern "C" {
#endif

void to64frombits(unsigned char *out, const unsigned char *in, int inlen);
int from64tobits(char *out, const char *in);

#ifdef __cplusplus
}
#endif


#endif
