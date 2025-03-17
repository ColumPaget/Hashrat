#include "../libUseful.h"


//crude demo of tty functions

void ReadReply(STREAM *S)
{
char *Tempstr=NULL;
Tempstr=STREAMReadLine(Tempstr, S);
while (StrLen(Tempstr))
{
	StripTrailingWhitespace(Tempstr);
	printf("%s\n",Tempstr);
	if (strcmp(Tempstr,"OK")==0) break;
	Tempstr=STREAMReadLine(Tempstr, S);
}
DestroyString(Tempstr);
}

main(int argc, char *argv[])
{
int fd;
STREAM *S;
char *Tempstr=NULL;


//this was tested with a GSM modem. These do not implement a full serial device, so options like
//'echo/TTYFLAG_ECHO' don't work properly. So all we do here is set the baudrate

//This is one method using TTYOpen
/*
Tempstr=CopyStr(Tempstr,"/dev/ttyS0");
if (argc > 0) Tempstr=CopyStr(Tempstr,argv[1]);

fd=TTYOpen(Tempstr, 38400, 0);
S=STREAMFromFD(fd);
*/


//this is another method using STREAMOpen
Tempstr=CopyStr(Tempstr,"tty:/dev/ttyS0");
if (argc > 0) Tempstr=MCopyStr(Tempstr,"tty:",argv[1],NULL);

S=STREAMOpen(Tempstr, "38400");
// end

STREAMWriteLine("ATI\r\n",S);
ReadReply(S);

STREAMWriteLine("AT+CSQ\r\n",S);
ReadReply(S);

STREAMWriteLine("AT^SYSINFO\r\n",S);
ReadReply(S);

STREAMWriteLine("AT+COPS=?\r\n",S);
ReadReply(S);

STREAMWriteLine("AT+COPS=23415\r\n",S);
ReadReply(S);



STREAMWriteLine("AT+CMGF=1\r\n",S);
ReadReply(S);
STREAMWriteLine("AT+CMGS=+447749722575\r\nThis is the text message.\x1a",S);
ReadReply(S);


STREAMClose(S);
DestroyString(Tempstr);
}
