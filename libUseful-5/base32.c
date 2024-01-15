#include "base32.h"






/* This is surely not the fastest base32 decoder, it is written for understanding/legiblity, not speed.
 * first we convert base32 into a binary, as a string of '1' and '0' characters (binary coded decimal). 
 * Then we work through this string converting it into eight-bit values.
 * The reason it's done this way is that it's tough to handle 'overlapping bits' when dealing with characters
 * that represent 5-bit chunks of a string of 8-bit values, and the resulting code of trying to do it all
 * with bit-shifting is rather unreadable
 */

char *base32tobinary(char *RetStr, const char *In, const char *Encoder)
{
const char *ptr, *found;
uint32_t val=0, count=0, bit;

for (ptr=In; *ptr != '\0'; ptr++)
{
//find the position in the base32 characterset. That position is the 'value' of the character
found=strchr(Encoder, *ptr); 
if (found) val=(found - Encoder);
else break;;

//the highest value a base 32 character can express is 16 (0x10)
bit=16;

//unpack the value into a string of 1's and 0's. As we add 1's and 0's from later characters
//we will get a contiguous stream of 1's and 0's that we can then break up into 8-bit chunks
while (bit > 0)
{
if (val & bit) RetStr=CatStr(RetStr, "1");
else RetStr=CatStr(RetStr, "0");
bit=bit >> 1;
}
}

return(RetStr);
}


int base32decode(unsigned char *Out, const char *In, const char *Encoder)
{
char *Tempstr=NULL;
unsigned char *p_Out;
const char *ptr;
uint32_t val;

Tempstr=base32tobinary(Tempstr, In, Encoder);

p_Out=Out;
for (ptr=Tempstr; ptr < Tempstr+StrLen(Tempstr); ptr+=8)
{
  val=parse_bcd_byte(ptr);
	*p_Out=val & 0xFF;
  p_Out++;
}

val=p_Out - Out;

Destroy(Tempstr);

return(val);
}




char *base32encode(char *RetStr, const char *Input, int Len, const char *Encoder, char Pad)
{
char *Tempstr=NULL, *BCD=NULL;
const char *ptr, *end;
int val;

RetStr=CopyStr(RetStr, "");
BCD=encode_bcd_bytes(BCD, Input, Len);

end=BCD+StrLen(BCD);
for (ptr=BCD; ptr < end; ptr+=5)
{
Tempstr=CopyStrLen(Tempstr, ptr, 5); 
Tempstr=PadStrTo(Tempstr, '0', 5);
val=(int) parse_bcd_byte(Tempstr);
RetStr=AddCharToStr(RetStr, Encoder[val]);
}

val=StrLen(RetStr) % 8;
for (; val < 8; val++) RetStr=AddCharToStr(RetStr, Pad);

Destroy(Tempstr);
Destroy(BCD);
return(RetStr);
}
