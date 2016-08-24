#include "libUseful-2.0/libUseful.h"
#include <mmintrin.h>

#define FNV_INIT_VAL 2166136261

unsigned int fnv_data2bucket(unsigned char *key, int size, int NoOfItems)
{
  unsigned char *p, *end;
  unsigned int h = FNV_INIT_VAL;
  int i;

	end=key+size;
  for (p=key; p < end ; p++ ) h = ( h * 16777619 ) ^ *p;

  return(h % NoOfItems);
}

unsigned int fnv_string2bucket(unsigned char *key, int size, int NoOfItems)
{
  unsigned char *p;
  unsigned int h = FNV_INIT_VAL;
  int i;

  for (p=key; *p !='\0' ; p++ ) h = ( h * 16777619 ) ^ *p;

  return(h % NoOfItems);
}


unsigned int mmx_hash_bucket_data(unsigned char *key, int size, int NoOfItems)
{
		char *p, *end;
    __m64 v1, v2, s; 
		int val;

		if (size < 8) return(fnv_data2bucket(key, size, NoOfItems));

		p=key;
		end=key+size;
    _mm_empty();                            // emms
		v1=_mm_set1_pi32(FNV_INIT_VAL);

		while ((end-p) > 7)
		{
		v2=_mm_setr_pi32(*p,*(p+4));
		v1=_mm_add_pi16(v1, v2);
		v1=_mm_slli_pi32(v1, 3);
		p+=8;
		}

		val=_mm_cvtsi64_si32(v1);
    _mm_empty();                            // emms

		if (val < 0) val=1-val;
 		val =val % NoOfItems;
		return(val);
}





main(int argc, char *argv[])
{
int val, len;
STREAM *S;
char *Tempstr=NULL;
clock_t start, mid, end;

S=STREAMOpenFile(argv[1],SF_RDONLY);
start=clock();
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
	len=StrLen(Tempstr);
	val=mmx_hash_bucket_data(Tempstr, len, 9999);
	Tempstr=STREAMReadLine(Tempstr,S);
}
end=clock();

printf("MMX: %d\n",end-start,val,Tempstr);
STREAMSeek(S,0,SEEK_SET);

start=clock();
Tempstr=STREAMReadLine(Tempstr,S);
while (Tempstr)
{
	len=StrLen(Tempstr);
	val=fnv_string2bucket(Tempstr,len, 9999);
	Tempstr=STREAMReadLine(Tempstr,S);
}
end=clock();
printf("FNV: %d\n",end-start,val,Tempstr);




STREAMClose(S);
}

