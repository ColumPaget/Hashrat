#include "libUseful.h"

void CompressTermcap()
{
STREAM *In, *Out;
char *Buffer=NULL;
int result;

Out=STREAMOpenFile("/tmp/termcap.gz",O_CREAT | O_TRUNC | O_WRONLY);
STREAMAddStandardDataProcessor(Out, "compression", "gzip", "");

Buffer=SetStrLen(Buffer,4096);
In=STREAMOpenFile("/etc/termcap",O_RDONLY);
result=STREAMReadBytes(In,Buffer,4096);
while (result > 0)
{
	STREAMWriteBytes(Out,Buffer,result);
	result=STREAMReadBytes(In,Buffer,4096);
}
STREAMClose(Out);
STREAMClose(In);
}

void ReadTermcap()
{
STREAM *In, *Out;
char *Buffer=NULL;
int result;

In=STREAMOpenFile("/tmp/termcap.gz",O_RDONLY);
STREAMAddStandardDataProcessor(In, "compression", "gzip", "");

Buffer=SetStrLen(Buffer,4096);
Out=STREAMOpenFile("/tmp/termcap.out",O_CREAT|O_TRUNC|O_WRONLY);
result=STREAMReadBytes(In,Buffer,4096);
while (result > 0)
{
	STREAMWriteBytes(Out,Buffer,result);
	result=STREAMReadBytes(In,Buffer,4096);
}
STREAMClose(In);
STREAMClose(Out);
}

main()
{
	printf("WRITE\n");
	CompressTermcap();
	printf("READ\n");
	ReadTermcap();
}

//TarFiles(Out,"*.c");
