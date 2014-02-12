#include "ansi.h"


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
