#include "cgi.h"
#include "files.h"

//This file provides an HTTP Common Gateway Interface for Hashrat

#define CGI_DOHASH 1
#define CGI_HIDETEXT 256
#define CGI_SHOWTEXT 512

char *EncodingNames[]={"octal","dec","hex","uhex","base64","ibase64","pbase64","xxencode","uuencode","crypt","ascii85","z85",NULL};
char *EncodingDescriptions[]={"Octal","Decimal","Hexadecimal","Uppercase Hexadecimal","Base64","IBase64","PBase64","XXEncode","UUEncode","Crypt","ASCII85","Z85",NULL};
int Encodings[]={ENCODE_OCTAL, ENCODE_DECIMAL, ENCODE_HEX, ENCODE_HEXUPPER, ENCODE_BASE64, ENCODE_IBASE64, ENCODE_PBASE64, ENCODE_XXENC, ENCODE_UUENC, ENCODE_CRYPT, ENCODE_ASCII85, ENCODE_Z85};

char *LineEndingNames[]={"none","lf","crlf","cr",NULL};
char *LineEndingDescriptions[]={"None","Unix (newline)","DOS (carriage-return+newline)","carriage-return",NULL};

void CGIPrintSelect(const char *Name, const char *CurrType, ListNode *Items)
{
ListNode *Curr;

printf("<td>\n");
printf("<select name=%s>\r\n", Name);

Curr=ListGetNext(Items);
while(Curr)
{
	if (StrLen(CurrType) && (strcmp(Curr->Tag,CurrType)==0)) printf("<option selected> %s\r\n",Curr->Tag);
	else printf("<option value=%s> %s\r\n",Curr->Tag, Curr->Item);
	Curr=ListGetNext(Curr);
}
printf("</select>\r\n");
printf("</td>\n");

}







void CGIDrawTextInput(int Flags)
{

printf("<td>Text:</td><td colspan=3>\r\n");
if (Flags & CGI_HIDETEXT) 
{
	printf("<input type=password width=90%% name=\"PlainText\" style=\"font-weight: bold;  font-size:16px\">\r\n");
	printf(" &nbsp; <input type=submit name=\"ShowText\" value=\"Show Text\" title=\"Show text while typing\">\r\n");
	printf("<input type=hidden name=\"HideText\" value=\"Hide Text\">\r\n");
}
else
{
	printf("<input type=text width=90%% name=\"PlainText\" style=\"font-weight: bold;  font-size:16px\">\r\n");
	printf(" &nbsp; <input type=submit name=\"HideText\" value=\"Hide Text\" title=\"Hide text while typing\">\r\n");
}

printf("</td>\r\n");
}


int CGIParseArgs(char **HashType, char **Encoding, char **LineEnding, char **Text)
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
	if (strcmp(Name,"Encoding")==0) *Encoding=CopyStr(*Encoding, Value);
	if (strcmp(Name,"LineEnding")==0) *LineEnding=CopyStr(*LineEnding, Value);

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

/*
void CGIDrawHashResult(char *Hash)
{
	printf("<p/>\r\n");
	printf("<table align=center>\r\n");
	printf("<tr><th>Your Hash is:</th></tr>\r\n");
	printf("<tr><td style=\"font-size: 16px\">%s</td></tr>\n",Hash);
	printf("<tr><td><textarea rows=2 cols=64 readonly style=\"font-weight: bold;  font-size:16px\">%s</textarea></td></tr>\n",Hash);
	printf("</table>\r\n");
	printf("<p/>\r\n");
}
*/


void CGIDrawHashResult(char *Hash)
{
	printf("<p /><p />\r\n");
	printf("<div align=center>\r\n");
	printf("<h4>Your Hash is:</h4>\r\n");
	printf("<h2>%s<h2>\r\n",Hash);
	printf("<h4>Here it is in a text box so you can 'select all'</h4>\r\n");
	printf("<textarea rows=2 cols=64 readonly style=\"font-weight: bold;  font-size:16px\">%s</textarea>\n",Hash);
	printf("<p />\r\n");
	printf("</div>\r\n");
}


void CGIDisplayPage()
{
char *HashType=NULL, *Encoding=NULL, *LineEnding=NULL,  *Text=NULL, *Hash=NULL;
HashratCtx *Ctx;
ListNode *Items;
int Flags, val, i;

Items=ListCreate();

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


Flags=CGIParseArgs(&HashType, &Encoding, &LineEnding, &Text);

printf("<body><html><form>\r\n");

printf("<h2 align=center>Hashrat: Online hash calculator</h2>\r\n");
printf("<div align=center>Version: %s, Licence: GPLv3, Author: Colum Paget, BugReports: colums projects at gmail dot com</div>\r\n",VERSION);

if (Flags & CGI_DOHASH)
{
	Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
	Ctx->HashType=CopyStr(Ctx->HashType,HashType);
	Ctx->Encoding |=ENCODE_HEX;

	val=MatchTokenFromList(Encoding, EncodingNames, 0);
	if (val > -1) Ctx->Encoding=Encodings[i];
	
	if (StrLen(LineEnding))
	{
	if (strcmp(LineEnding, "crlf")==0) Text=CatStr(Text,"\r\n");
	if (strcmp(LineEnding, "lf")==0) Text=CatStr(Text,"\n");
	if (strcmp(LineEnding, "cr")==0) Text=CatStr(Text,"\r");
	}

	ProcessData(&Hash, Ctx, Text, StrLen(Text));

	CGIDrawHashResult(Hash);
}

printf("<table align=center>\r\n");
printf("<tr>\r\n");
printf("<td>Hash Type:</td><td>");
HashAvailableTypes(Items);
CGIPrintSelect("HashType", HashType, Items);
ListClear(Items, DestroyString);
printf("</td></tr>\r\n");


printf("<tr>\r\n");
CGIDrawTextInput(Flags);
printf("</tr>\r\n");

printf("<tr>\r\n");
printf("<td>Encoding:</td>\r\n");
for (i=0; EncodingNames[i] !=NULL; i++) SetVar(Items, EncodingNames[i], EncodingDescriptions[i]);
CGIPrintSelect("Encoding", HashType, Items);
ListClear(Items, DestroyString);
printf("</tr>\r\n");

printf("<tr>\r\n");
printf("<td>Line Ending:</td>\r\n");
for (i=0; LineEndingNames[i] !=NULL; i++) SetVar(Items, LineEndingNames[i], LineEndingDescriptions[i]);
CGIPrintSelect("LineEnding", HashType, Items);
ListClear(Items, DestroyString);
printf("</tr>\r\n");

printf("<tr><td colspan=4><input type=submit value=\"Hash it!\"></td></tr>\r\n");
printf("</table>\r\n");

printf("</form></html></body>\r\n");

fflush(NULL);

ListDestroy(Items, DestroyString);
DestroyString(HashType);
DestroyString(Hash);
DestroyString(Text);
}
