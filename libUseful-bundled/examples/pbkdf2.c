#include "../libUseful.h"

main()
{
char *Hash=NULL;

PBK2DF2(&Hash, "hmac-sha1", "password", 8, "salt", 4, 1, ENCODE_HEX);

printf("result: %s\n", Hash);

Destroy(Hash);
}
