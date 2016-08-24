#include "ansi.h"


const char *ANSIColorStrings[]={"none","black","red","green","yellow","blue","magenta","cyan","white",NULL};



int ANSIParseColor(const char *Str)
{
int val;

val=MatchTokenFromList(Str, ANSIColorStrings, 0);
if (val==-1) val=ANSI_NONE;

return(val);
}

char *ANSICode(int Color, int BgColor, int Flags)
{
static char *ANSI=NULL;
int FgVal, BgVal;

if ((! Color) && (! Flags)) return("");

if ( (Color > 0) && (BgColor > 0) )
{	
	//Bg colors are set into the higher byte of 'attribs', so that we can hold both fg and bg in the
	//same int, so we must shift them down
	BgColor=BgColor >> 8;

	if (Color >= ANSI_DARKGREY) FgVal=90+Color - ANSI_DARKGREY;
	else FgVal=30+Color-1;
	
		if (BgColor >= ANSI_DARKGREY) BgVal=100+BgColor - ANSI_DARKGREY;
	else BgVal=40+BgColor-1;
	
	ANSI=FormatStr(ANSI,"\x1b[%d;%d",FgVal,BgVal);
	if (Flags) ANSI=CatStr(ANSI,";");
}
else if (Color > 0) 
{
	if (Color >= ANSI_DARKGREY) FgVal=90+Color - ANSI_DARKGREY;
	else FgVal=30+Color-1;
	
	ANSI=FormatStr(ANSI,"\x1b[%d",FgVal);
	if (Flags) ANSI=CatStr(ANSI,";");
}
else ANSI=CopyStr(ANSI,"\x1b[");

if (Flags)
{
	if (Flags & ANSI_BOLD) ANSI=CatStr(ANSI,"01");
	if (Flags & ANSI_FAINT) ANSI=CatStr(ANSI,"02");
	if (Flags & ANSI_UNDER) ANSI=CatStr(ANSI,"04");
	if (Flags & ANSI_BLINK) ANSI=CatStr(ANSI,"05");
	if (Flags & ANSI_INVERSE) ANSI=CatStr(ANSI,"07");
}
ANSI=CatStr(ANSI,"m");

return(ANSI);
}


char *TerminalReadText(char *RetStr, int Flags, STREAM *S)
{
int inchar, len=0;
char outchar;

inchar=STREAMReadChar(S);
while (inchar != EOF)
{
	if (Flags & TERM_SHOWTEXT) outchar=inchar & 0xFF;
	if (Flags & TERM_SHOWSTARS) 
	{
		if ((outchar & 0xFF) =='\n') outchar=inchar & 0xFF;
		else if ((outchar & 0xFF) =='\b') outchar=inchar & 0xFF;
		else outchar='*';
	}


	if (Flags & TERM_SHOWTEXTSTARS) 
	{
		switch (inchar)
		{

		case '\n':
		case '\r':
		case 0x08:
		break;

		default:
		if (len > 0) STREAMWriteString("\x08*",S);
		break;
		}

		outchar=inchar & 0xFF;
	}


	STREAMWriteBytes(S, &outchar,1);
	STREAMFlush(S);
	if (inchar == '\n') break;
	if (inchar == '\r') break;

	RetStr=AddCharToBuffer(RetStr,len++, inchar & 0xFF);
	inchar=STREAMReadChar(S);
}

return(RetStr);
}
