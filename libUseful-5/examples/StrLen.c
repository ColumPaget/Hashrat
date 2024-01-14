#include "../libUseful.h"


int StrLenTest(const char *Str, unsigned int iter)
{
uint64_t start, end;
int i, len;

start=GetTime(0);
//printf("DOC: %s\n", Tempstr);
for (i=0; i < iter; i++)
{
len=StrLenFromCache(Str);
}
end=GetTime(0);

printf("CACHE: iter=%u len=%d time=%d\n", iter, len, (int) (end-start));

start=GetTime(0);
//printf("DOC: %s\n", Tempstr);
for (i=0; i < iter; i++)
{
len=strlen(Str);
}
end=GetTime(0);

printf("NO CACHE: iter=%u len=%d time=%d\n", iter, len, (int) (end-start));
}


main()
{
STREAM *S;
char *Tempstr=NULL;

StrLenCacheInit(10, 100);

/*
S=STREAMOpen("http://freshcode.club/", "r");
Tempstr=STREAMReadDocument(Tempstr, S);
STREAMClose(S);
*/
Tempstr=CopyStr(Tempstr, "this is a test. Twas brillig and the slivey tooves dist gyre and gimble in the wable. All mimsy were the borogroves and all the meme wraths outgrabe.");

StrLenTest(Tempstr, UINT_MAX);
}
