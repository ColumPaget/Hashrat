#include "cgi.h"
#include "files.h"
#include "encodings.h"

//This file provides an HTTP Common Gateway Interface for Hashrat


//Gets recursively called
static int CGIParseArgs(const char *Str, const char *Sep1, const char *Sep2, char **HashType, char **Encoding, char **LineEnding, char **Text, int *OutputLength, int *SegmentLength, char **SegmentChar, char **OptionsFile);



static void CGIPrintSelect(const char *Name, const char *CurrType, ListNode *Items)
{
    ListNode *Curr;

    printf("<select name=%s>\r\n", Name);

    Curr=ListGetNext(Items);
    while(Curr)
    {
        if (StrValid(CurrType) && (strcmp(Curr->Tag,CurrType)==0)) printf("<option selected value=%s> %s\r\n",Curr->Tag, Curr->Item);
        else printf("<option value=%s> %s\r\n",Curr->Tag, Curr->Item);
        Curr=ListGetNext(Curr);
    }
    printf("</select>\r\n");
}





static void CGIDrawTextInput(int Flags)
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



static int CGILoadOptionsFile(const char *Path, char **HashType, char **Encoding, char **LineEnding, char **Text, int *OutputLength, int *SegmentLength, char **SegmentChar)
{
    STREAM *S;
    char *Tempstr=NULL, *OptionsFile=NULL;
    int Flags=0;

    S=STREAMOpen(Path,"r");
    if (S)
    {
        Tempstr=STREAMReadDocument(Tempstr, S);
        Flags=CGIParseArgs(Tempstr, "\n", "=", HashType, Encoding, LineEnding, Text, OutputLength, SegmentLength, SegmentChar, &OptionsFile);
        STREAMClose(S);
    }

    Destroy(OptionsFile);
    Destroy(Tempstr);

    return(Flags);
}



static int CGIParseArgs(const char *Str, const char *Sep1, const char *Sep2, char **HashType, char **Encoding, char **LineEnding, char **Text, int *OutputLength, int *SegmentLength, char **SegmentChar, char **OptionsFile)
{
    char *QName=NULL, *QValue=NULL, *Name=NULL, *Value=NULL;
    const char *ptr;
    int Flags=0;

    ptr=GetNameValuePair(Str, Sep1, Sep2, &QName, &QValue);
    while (ptr)
    {
        Name=HTTPUnQuote(Name,QName);
        Value=HTTPUnQuote(Value,QValue);

        if (strcasecmp(Name,"OptionsFile")==0)
        {
            Flags |= CGILoadOptionsFile(Value, HashType, Encoding, LineEnding, Text, OutputLength, SegmentLength, SegmentChar);
            *OptionsFile=CopyStr(*OptionsFile, Value);
        }

        if (strcasecmp(Name,"HashType")==0) *HashType=CopyStr(*HashType, Value);
        if (strcasecmp(Name,"PlainText")==0)
        {
            *Text=CopyStr(*Text, Value);
            Flags |= CGI_DOHASH;
        }
        if (strcasecmp(Name,"Encoding")==0) *Encoding=CopyStr(*Encoding, Value);
        if (strcasecmp(Name,"LineEnding")==0) *LineEnding=CopyStr(*LineEnding, Value);
        if (strcasecmp(Name,"SegmentChar")==0) *SegmentChar=CopyStr(*SegmentChar, Value);

        if (strcasecmp(Name,"HideText")==0) Flags |= CGI_HIDETEXT;
        if (strcasecmp(Name,"ShowText")==0) Flags |= CGI_SHOWTEXT;

        if (strcasecmp(Name,"OutputLength")==0) *OutputLength=atoi(Value);
        if (strcasecmp(Name,"SegmentLength")==0) *SegmentLength=atoi(Value);

        if (strcasecmp(Name,"NoOptions")==0) Flags |= CGI_NOOPTIONS;

        ptr=GetNameValuePair(ptr, Sep1, Sep2, &QName, &QValue);
    }

    if (Flags & CGI_SHOWTEXT) Flags &= ~CGI_HIDETEXT;

    Destroy(QName);
    Destroy(QValue);
    Destroy(Name);
    Destroy(Value);

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


static void CGIDrawHashResult(const char *Hash)
{
    printf("<p /><p />\r\n");
    printf("<table align=center bgcolor=#FFBBBB>\r\n");
    printf("<tr><th bgcolor=red><font color=white>Your Hash is:</font></th></tr>\r\n");
//	printf("<tr><td><b>%s</b></td>\r\n",Hash);
//	printf("<tr><td>Here it is in a text box so you can 'select all'</td>\r\n");
    printf("<tr><td align=center><textarea align=center rows=1 cols=%d readonly style=\"font-weight: bold;  font-size:16px\">%s</textarea></td>\n",StrLen(Hash)+2, Hash);
    printf("</table>\r\n");
    printf("<p /><p />\r\n");
}


static void CGIDisplayOptions(const char *HashType, const char *Encoding, const char *LineEnding, int OutputLength)
{
    char *Token=NULL, *Tempstr=NULL;
    const char *ptr;
    ListNode *Items=NULL;
    int i;

    Items=ListCreate();
    printf("<tr>\r\n");

    Tempstr=HashAvailableTypes(Tempstr);
    ptr=GetToken(Tempstr, ",",&Token,0);
    while (ptr)
    {
        ListAddNamedItem(Items, Token, CopyStr(NULL, Token));
        ptr=GetToken(ptr, ",",&Token,0);
    }

    printf("<td align=left>Type: ");
    CGIPrintSelect("HashType", HashType, Items);
    ListClear(Items, Destroy);
    printf("</td>\r\n");

    printf("<td align=right>Encoding: ");
    for (i=0; EncodingNames[i] !=NULL; i++) SetVar(Items, EncodingNames[i], EncodingDescriptions[i]);
    CGIPrintSelect("Encoding", Encoding, Items);
    ListClear(Items, Destroy);
    printf("</td>\r\n");

    printf("<tr>\r\n");
    printf("<td align=left>Line Ending: </td>");
    printf("<td align=right>");
    for (i=0; LineEndingNames[i] !=NULL; i++) SetVar(Items, LineEndingNames[i], LineEndingDescriptions[i]);
    CGIPrintSelect("LineEnding", LineEnding, Items);
    ListClear(Items, Destroy);
    printf("</td>\r\n");
    printf("</tr>\r\n");

    printf("<tr>\r\n");
    printf("<td align=left>Hash Length: </td>");
    printf("<td align=right>");
    if (OutputLength > 0) printf("<input type=text width=90%% name=\"OutputLength\" style=\"font-weight: bold;  font-size:16px\" value=\"%d\">\r\n",OutputLength);
    else printf("<input type=text width=90%% name=\"OutputLength\" style=\"font-weight: bold;  font-size:16px\">\r\n");
    printf("</td>\r\n");
    printf("</tr>\r\n");

    ListDestroy(Items, Destroy);
    Destroy(Tempstr);
    Destroy(Token);
}


void CGIDisplayPage()
{
    char *HashType=NULL, *Encoding=NULL, *LineEnding=NULL, *SegmentChar=NULL,  *Text=NULL, *Hash=NULL, *Token=NULL, *OptionsFile=NULL;
    const char *ptr;
    HashratCtx *Ctx;
    ListNode *Items;
    int Flags, i, OutputLength=0, SegmentLength=0;

    SegmentChar=CopyStr(SegmentChar, " ");
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


    Flags=CGIParseArgs(getenv("QUERY_STRING"),"&","=",&HashType, &Encoding, &LineEnding, &Text, &OutputLength, &SegmentLength, &SegmentChar, &OptionsFile);

    printf("<body><html><form>\r\n");

    printf("<h2 align=center>Hashrat: Online hash calculator</h2>\r\n");
    printf("<div align=center>Version: %s, Licence: GPLv3, Author: Colum Paget, BugReports: colums projects at gmail dot com</div><p/>\r\n",VERSION);

    if (Flags & CGI_DOHASH)
    {
        Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
        Ctx->HashType=CopyStr(Ctx->HashType,HashType);
        Ctx->OutputLength=OutputLength;
        Ctx->SegmentLength=SegmentLength;
        Ctx->SegmentChar=SegmentChar[0];
        Ctx->Encoding |=ENCODE_HEX;
        if ((OutputLength > 0) || (SegmentLength > 0)) Ctx->Flags |= CTX_REFORMAT;

        i=MatchTokenFromList(Encoding, EncodingNames, 0);
        if (i > -1) Ctx->Encoding=Encodings[i];

        if (StrLen(LineEnding))
        {
      		Text=PreProcessInput(Text, ptr, GetVar(Ctx->Vars,"InputPrefix"), LineEnding);
        }

        ProcessData(&Hash, Ctx, Text, StrLen(Text));

        Token=ReformatHash(Token, Hash, Ctx);
        CGIDrawHashResult(Token);
    }

    printf("<table align=center bgcolor=#BBBBBB>\r\n");
    printf("<tr>\r\n");
    printf("<th colspan=2 bgcolor=blue><font color=white>New Hash</font></th>\r\n");

    if (Flags & CGI_NOOPTIONS)
    {
        printf("<input type=hidden name=\"HashType\" value=\"%s\">\n",HashType);
        printf("<input type=hidden name=\"Encoding\" value=\"%s\">\n",Encoding);
        printf("<input type=hidden name=\"LineEnding\" value=\"%s\">\n",LineEnding);
        printf("<input type=hidden name=\"OutputLength\" value=\"%d\">\n",OutputLength);
        printf("<input type=hidden name=\"SegmentLength\" value=\"%d\">\n",SegmentLength);
        if (StrLen(SegmentChar)) printf("<input type=hidden name=\"SegmentChar\" value=\"%s\">\n",SegmentChar);
        printf("<input type=hidden name=\"NoOptions\" value=\"Y\">\n");
        //if (StrLen(OptionsFile)) printf("<input type=hidden name=\"OptionsFile\" value=\"%d\">\n",OptionsFile);
    }
    else CGIDisplayOptions(HashType, Encoding, LineEnding, OutputLength);

    printf("<tr>\r\n");
    CGIDrawTextInput(Flags);
    printf("</tr>\r\n");


    printf("<tr><td colspan=4><input type=submit value=\"Hash it!\"></td></tr>\r\n");
    printf("</table>\r\n");


    printf("</form></html></body>\r\n");

    fflush(NULL);

    ListDestroy(Items, Destroy);

    Destroy(LineEnding);
    Destroy(OptionsFile);
    Destroy(SegmentChar);
    Destroy(Encoding);
    Destroy(HashType);
    Destroy(Token);
    Destroy(Hash);
    Destroy(Text);
}
