#include "cgi.h"

//This file provides an HTTP Common Gateway Interface for Hashrat

#define CGI_DOHASH 1
#define CGI_LF 2
#define CGI_CRLF 4
#define CGI_OCTAL 8
#define CGI_HEX 16
#define CGI_HEXUPPER 32
#define CGI_BASE64 64
#define CGI_DECIMAL 128
#define CGI_HIDETEXT 256
#define CGI_SHOWTEXT 512

void CGIPrintHashTypeSelect(char *CurrType)
{
ListNode *Vars, *Curr;

Vars=ListCreate();
HashAvailableTypes(Vars);
printf("<select name=HashType>\r\n");

Curr=ListGetNext(Vars);
while(Curr)
{
	if (strcmp(Curr->Tag,CurrType)==0) printf("<option selected> %s\r\n",Curr->Tag);
	else printf("<option> %s\r\n",Curr->Tag);
	Curr=ListGetNext(Curr);
}
printf("</select>\r\n");

ListDestroy(Vars, DestroyString);
}


void CGIPrintRadioButton(char *Name, char *Value, char *Caption, char *Color, char *Checked, char *Title, char *ColSpan)
{
printf("<td bgcolor=\"%s\" title=\"%s\" colspan=\"%s\"> %s <input type=radio title=\"%s\" name=\"%s\" value=\"%s\" %s></td>\n",Color,Title,ColSpan,Caption,Title,Name,Value,Checked); 
}

void CGIPrintHashEncodingSelect(int Flags)
{
printf("<td>Encoding:</td>\r\n");

if (Flags & CGI_OCTAL) 
{
	CGIPrintRadioButton("Encoding", "oct", "Octal", "#BFBB99", "checked", "Encode the hash in octal","1");
	CGIPrintRadioButton("Encoding", "dec", "Decimal", "#99BBFF", "", "Encode the hash in decimal","1");
	CGIPrintRadioButton("Encoding", "hex", "Hex", "#BBBBFF", "", "Encode the hash in hexadecimal","1");
	CGIPrintRadioButton("Encoding", "hexupper", "Uppercase Hex", "#BB88FF", "", "Encode the hash in UPPERCASE hexadecimal","1");
	CGIPrintRadioButton("Encoding", "base64", "Base64", "#BBFFFF", "", "Encode the hash with base64","1");
}
else if (Flags & CGI_BASE64) 
{
	CGIPrintRadioButton("Encoding", "oct", "Octal", "#BFBB99", "", "Encode the hash in octal","1");
	CGIPrintRadioButton("Encoding", "dec", "Decimal", "#99BBFF", "", "Encode the hash in decimal","1");
	CGIPrintRadioButton("Encoding", "hex", "Hex", "#BBBBFF", "", "Encode the hash in hexadecimal","1");
	CGIPrintRadioButton("Encoding", "hexupper", "Uppercase Hex", "#BB88FF", "", "Encode the hash in UPPERCASE hexadecimal","1");
	CGIPrintRadioButton("Encoding", "base64", "Base64", "#BBFFFF", "checked", "Encode the hash with base64","1");
}
else if (Flags & CGI_DECIMAL) 
{
	CGIPrintRadioButton("Encoding", "oct", "Octal", "#BFBB99", "", "Encode the hash in octal","1");
	CGIPrintRadioButton("Encoding", "dec", "Decimal", "#99BBFF", "checked", "Encode the hash in decimal","1");
	CGIPrintRadioButton("Encoding", "hex", "Hex", "#BBBBFF", "", "Encode the hash in hexadecimal","1");
	CGIPrintRadioButton("Encoding", "hexupper", "Uppercase Hex", "#BB88FF", "", "Encode the hash in UPPERCASE hexadecimal","1");
	CGIPrintRadioButton("Encoding", "base64", "Base64", "#BBFFFF", "", "Encode the hash with base64","1");
}
else if (Flags & CGI_HEXUPPER) 
{
	CGIPrintRadioButton("Encoding", "oct", "Octal", "#BFBB99", "", "Encode the hash in octal","1");
	CGIPrintRadioButton("Encoding", "dec", "Decimal", "#99BBFF", "", "Encode the hash in decimal","1");
	CGIPrintRadioButton("Encoding", "hex", "Hex", "#BBBBFF", "", "Encode the hash in hexadecimal","1");
	CGIPrintRadioButton("Encoding", "hexupper", "Uppercase Hex", "#BB88FF", "checked", "Encode the hash in UPPERCASE hexadecimal","1");
	CGIPrintRadioButton("Encoding", "base64", "Base64", "#BBFFFF", "", "Encode the hash with base64","1");
}
else 
{
	CGIPrintRadioButton("Encoding", "oct", "Octal", "#BFBB99", "", "Encode the hash in octal","1");
	CGIPrintRadioButton("Encoding", "dec", "Decimal", "#99BBFF", "", "Encode the hash in decimal","1");
	CGIPrintRadioButton("Encoding", "hex", "Hex", "#BBBBFF", "checked", "Encode the hash in hexadecimal","1");
	CGIPrintRadioButton("Encoding", "hexupper", "Uppercase Hex", "#BB88FF", "", "Encode the hash in UPPERCASE hexadecimal","1");
	CGIPrintRadioButton("Encoding", "base64", "Base64", "#BBFFFF", "", "Encode the hash with base64","1");
}


}


void CGIPrintHashLineEndSelect(int Flags)
{

printf("<td>Line Ending:</td>\r\n");
if (Flags & CGI_CRLF) 
{
	CGIPrintRadioButton("LineEnding", "none", "None", "#FFAAFF", "","","1");
	CGIPrintRadioButton("LineEnding", "lf","Unix (newline)", "#AAFFFF", "","add newline before hashing (compatible with things like 'echo text | md5sum')","1");
	CGIPrintRadioButton("LineEnding", "crlf", "DOS (carriage-return,newline)", "#AAFFAA", "checked","add carriage-return/newline before hashing","3");
}
else if (Flags & CGI_LF) 
{
	CGIPrintRadioButton("LineEnding", "none", "None", "#FFAAFF", "","","1");
	CGIPrintRadioButton("LineEnding", "lf","Unix (newline)", "#AAFFFF", "checked","add newline before hashing (compatible with things like 'echo text | md5sum')","1");
	CGIPrintRadioButton("LineEnding", "crlf", "DOS (carriage-return,newline)", "#AAFFAA", "","add carriage-return/newline before hashing","3");
}
else
{
	CGIPrintRadioButton("LineEnding", "none", "None", "#FFAAFF", "checked","","1");
	CGIPrintRadioButton("LineEnding", "lf","Unix (newline)", "#AAFFFF", "","add newline before hashing (compatible with things like 'echo text | md5sum')","1");
	CGIPrintRadioButton("LineEnding", "crlf", "DOS (carriage-return,newline)", "#AAFFAA", "","add carriage-return/newline before hashing","3");
}

}



void CGIDrawTextInput(int Flags)
{

printf("<td>Text:</td><td colspan=3>\r\n");
if (Flags & CGI_HIDETEXT) 
{
	printf("<input type=password width=90%% name=\"PlainText\">\r\n");
	printf(" &nbsp; <input type=submit name=\"ShowText\" value=\"Show Text\" title=\"Show text while typing\">\r\n");
	printf("<input type=hidden name=\"HideText\" value=\"Hide Text\">\r\n");
}
else
{
	printf("<input type=text width=90%% name=\"PlainText\">\r\n");
	printf(" &nbsp; <input type=submit name=\"HideText\" value=\"Hide Text\" title=\"Hide text while typing\">\r\n");
}

printf("</td>\r\n");
}


int CGIParseArgs(char **HashType, char **Text)
{
char *QName=NULL, *QValue=NULL, *Name=NULL, *Value=NULL, *ptr;
int Flags=0;

ptr=getenv("QUERY_STRING");

ptr=GetNameValuePair(ptr, "&","=",&QName,&QValue);
while (ptr)
{
	Name=HTTPUnQuote(Name,QName);
	Value=HTTPUnQuote(Value,QValue);

	if (strcmp(Name,"HashType")==0) *HashType=CopyStr(*HashType, Value);
	if (strcmp(Name,"PlainText")==0) 
	{
		*Text=CopyStr(*Text, Value);
		Flags |= CGI_DOHASH;
	}
	if (strcmp(Name,"Encoding")==0)
	{
		if (strcmp(Value,"base64")==0) Flags |= CGI_BASE64;
		if (strcmp(Value,"hexupper")==0) Flags |= CGI_HEXUPPER;
		if (strcmp(Value,"oct")==0) Flags |= CGI_OCTAL;
		if (strcmp(Value,"dec")==0) Flags |= CGI_DECIMAL;
	}
	if (strcmp(Name,"LineEnding")==0)
	{
		if (strcmp(Value,"lf")==0) Flags |= CGI_LF;
		if (strcmp(Value,"crlf")==0) Flags |= CGI_CRLF;
	}
	if (strcmp(Name,"HideText")==0) Flags |= CGI_HIDETEXT;
	if (strcmp(Name,"ShowText")==0) Flags |= CGI_SHOWTEXT;

ptr=GetNameValuePair(ptr, "&","=",&QName,&QValue);
}

//if we *were* in HIDETEXT mode, but the user has clicked on 'Show Text', then
//override the 'HIDETEXT'
if (Flags & CGI_SHOWTEXT) Flags &= ~CGI_HIDETEXT;

DestroyString(QName);
DestroyString(QValue);
DestroyString(Name);
DestroyString(Value);

return(Flags);
}


void CGIDrawHashResult(char *Hash)
{
	printf("<p/>\r\n");
	printf("<table align=center>\r\n");
	printf("<tr><th>Your Hash is</th></tr>\r\n");
	printf("<tr><td><textarea rows=4 cols=64 readonly>%s</textarea></td></tr>\n",Hash);
	printf("</table>\r\n");
	printf("<p/>\r\n");
}

void CGIDisplayPage()
{
char *HashType=NULL, *Text=NULL, *Hash=NULL;
HashratCtx *Ctx;
int Flags;


//We don't need to read anything from disk, so in case we're running
//as root, or something like that, let's try to chroot, so no weird attacks are possible

//shouldn't work, because we shouldn't be running as root
chdir("/var/empty");
chroot(".");


//Send HTTP Headers
printf("Content-type: text/html\r\n");
printf("Connection: close\r\n");
printf("Cache-control: private, max-age=0, no-cache\r\n");
printf("\r\n");


Flags=CGIParseArgs(&HashType, &Text);

printf("<body><html><form>\r\n");

printf("<h2 align=center>Hashrat: Online hash calculator</h2>\r\n");
printf("<div align=center>Version: %s, Licence: GPLv3, Author: Colum Paget, BugReports: colums projects at gmail dot com</div>\r\n",VERSION);

if (Flags & CGI_DOHASH)
{
	Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
	Ctx->Encoding=ENCODE_HEX;
	Ctx->HashType=CopyStr(Ctx->HashType,HashType);
	if (Flags & CGI_BASE64) Ctx->Encoding=ENCODE_BASE64;
	if (Flags & CGI_HEXUPPER) Ctx->Encoding=ENCODE_HEXUPPER;
	if (Flags & CGI_DECIMAL) Ctx->Encoding=ENCODE_DECIMAL;
	if (Flags & CGI_OCTAL) Ctx->Encoding=ENCODE_OCTAL;
	if (Flags & CGI_CRLF) Text=CatStr(Text,"\r\n");
	if (Flags & CGI_LF) Text=CatStr(Text,"\n");
	Hash=ProcessData(Hash, Ctx, Text, StrLen(Text));

	CGIDrawHashResult(Hash);
}

printf("<table align=center>\r\n");
printf("<tr>\r\n");
printf("<td>Hash Type:</td><td>");
CGIPrintHashTypeSelect(HashType);
printf("</td></tr>\r\n");
printf("<tr>\r\n");
CGIDrawTextInput(Flags);
printf("<tr>\r\n");
CGIPrintHashEncodingSelect(Flags);
printf("</tr>\r\n");
printf("<tr>\r\n");
CGIPrintHashLineEndSelect(Flags);
printf("</tr>\r\n");
printf("<tr><td colspan=4><input type=submit value=\"Hash it!\"></td></tr>\r\n");
printf("</table>\r\n");

printf("</form></html></body>\r\n");

fflush(NULL);

DestroyString(HashType);
DestroyString(Hash);
DestroyString(Text);
}
