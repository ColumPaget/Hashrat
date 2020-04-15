/* This program gives the reference implementation of JH.
   It implements the standard description of JH (not bitslice)
   The description given in this program is suitable for hardware implementation

   --------------------------------
   Comparing to the original reference implementation,
   two functions are added to make the porgram more readable.
   One function is E8_initialgroup() at the beginning of E8;
   another function is E8_finaldegroup() at the end of E8.

   --------------------------------

   Last Modified: January 16, 2011
*/


#ifndef JH_HASH_H
#define JH_HASH_H


typedef unsigned long long DataLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;

typedef struct
{
    int hashbitlen;                     /*the message digest size*/
    unsigned long long databitlen;      /*the message size in bits*/
    unsigned long long datasize_in_buffer;  /*the size of the message remained in buffer; assumed to be multiple of 8bits except for the last partial block at the end of the message*/
    unsigned char H[128];               /*the hash value H; 128 bytes;*/
    unsigned char A[256];               /*the temporary round value; 256 4-bit elements*/
    unsigned char roundconstant[64];    /*round constant for one round; 64 4-bit elements*/
    unsigned char buffer[64];           /*the message block to be hashed; 64 bytes*/
} hashState;

/*The API functions*/
HashReturn JHInit(hashState *state, int hashbitlen);
HashReturn JHUpdate(hashState *state, const unsigned char *data, DataLength databitlen);
unsigned int JHFinal(hashState *state, unsigned char *hashval);


#endif
