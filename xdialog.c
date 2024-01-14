#include "xdialog.h"
#include "frontend.h"
#include "encodings.h"
#include "files.h"

char *XDialogFindXDialogCommand(char *Cmd, const char *XDialogCommandList)
{
    char *Token=NULL, *Found=NULL;
    const char *ptr;

    ptr=GetToken(XDialogCommandList, ",", &Token, 0);
    while (ptr)
    {
        Found=FindFileInPath(Found, Token, getenv("PATH"));
        if (StrValid(Found))
        {
            Cmd=CopyStr(Cmd, Found);
            break;
        }
        ptr=GetToken(ptr, ",", &Token, 0);
    }

    Destroy(Token);

    return(Cmd);
}




char *XDialogFormInit(char *Cmd, const char *DialogCmd, const char *Title)
{
    Cmd=MCopyStr(Cmd, "cmd:", DialogCmd, " --title='", Title, "'", NULL);
    return(Cmd);
}


char *XDialogFormAddList(char *Cmd, const char *DialogCmd, const char *Name, const char *Items, const char *Selected)
{
    char *Tempstr=NULL, *Token=NULL, *Divisor=NULL;
    const char *ptr;


    if (strcmp(GetBasename(DialogCmd), "yad")==0) Divisor=CopyStr(Divisor, "!");
    else Divisor=CopyStr(Divisor, "|");

    if (StrValid(Selected)) Tempstr=MCopyStr(Tempstr, Selected, Divisor, NULL);
    ptr=GetToken(Items, ",",&Token,0);
    while (ptr)
    {
        if ( (! StrValid(Selected)) || (strcmp(Selected, Token) !=0) ) Tempstr=MCatStr(Tempstr, Token, Divisor, NULL);
        ptr=GetToken(ptr, ",",&Token,0);
    }

    if (strcmp(GetBasename(DialogCmd), "yad")==0) Cmd=MCatStr(Cmd, " --form --field='", Name, "':CB '", Tempstr, "'", NULL);
    else Cmd=MCatStr(Cmd, " --forms --add-combo='", Name, "' --combo-values='", Tempstr, "'", NULL);

    Destroy(Tempstr);
    Destroy(Divisor);
    Destroy(Token);
    return(Cmd);
}


char *XDialogFormAddTextEntry(char *Cmd, const char *DialogCmd, const char *Name)
{
    if (strcmp(GetBasename(DialogCmd), "yad")==0) Cmd=MCatStr(Cmd, " --form --field='", Name, "':TXT", NULL);
    else Cmd=MCatStr(Cmd, " --forms --add-entry='", Name, "'", NULL);

    return(Cmd);
}


STREAM *XDialogDisplayPage(const char *Dialog, HashratCtx *Config)
{
    STREAM *S;
    char *Tempstr=NULL, *Token=NULL, *Cmd=NULL;
    const char *ptr;
    int i;

    Tempstr=MCopyStr(Tempstr, "Hashrat ", VERSION, NULL);
    Cmd=XDialogFormInit(Cmd, Dialog, Tempstr);

    Tempstr=HashAvailableTypes(Tempstr);
    Cmd=XDialogFormAddList(Cmd, Dialog, "Hash Type", Tempstr, Config->HashType);

    Tempstr=CopyStr(Tempstr, "");
    if (Config->Encoding > -1) Tempstr=MCopyStr(Tempstr, EncodingDescriptionFromID(Config->Encoding), ",", NULL);
    for (i=0; EncodingDescriptions[i] != NULL; i++)
    {
        if (strcmp(Tempstr, EncodingDescriptions[i]) !=0) Tempstr=MCatStr(Tempstr, EncodingDescriptions[i], ",", NULL);
    }
    Cmd=XDialogFormAddList(Cmd, Dialog, "Encoding", Tempstr, "");

    Tempstr=CopyStr(Tempstr, "");
    for (i=0; LineEndingDescriptions[i] != NULL; i++)
    {
        Token=QuoteCharsInStr(Token, LineEndingDescriptions[i], "\\");
        Tempstr=MCatStr(Tempstr, Token, ",", NULL);
    }
    Cmd=XDialogFormAddList(Cmd, Dialog, "Line Ending", Tempstr, "");
    Cmd=XDialogFormAddTextEntry(Cmd, Dialog, "Input");

    printf("CMD: %s\n", Cmd);
    S=STREAMOpen(Cmd, "rw");

    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Cmd);
    return(S);
}

void XDialogDisplayHash(const char *DialogCmd, const char *Hash)
{
    char *Cmd=NULL, *Tempstr=NULL;
    STREAM *S;


    if (strcmp(GetBasename(DialogCmd), "yad")==0) Cmd=MCopyStr(Cmd, "cmd:", DialogCmd, " --info --selectable-labels --title='Your Hash Value' --text='", Hash, "'", NULL);
    else Cmd=MCopyStr(Cmd, "cmd:", DialogCmd, " --info --title='Your Hash Value' --text='", Hash, "'", NULL);
    S=STREAMOpen(Cmd, "rw");
    Tempstr=STREAMReadLine(Tempstr, S);
    STREAMClose(S);

    Destroy(Tempstr);
    Destroy(Cmd);
}


int XDialogProcess(const char *Cmd, HashratCtx *Config)
{
    STREAM *S;
    char *Tempstr=NULL, *LineEnding=NULL, *Text=NULL;
    const char *ptr;
    HashratCtx *Ctx;
    int i, RetVal=FALSE;

    S=XDialogDisplayPage(Cmd, Config);
    Tempstr=STREAMReadLine(Tempstr, S);
    STREAMClose(S);

    if (StrValid(Tempstr))
    {
        RetVal=TRUE;

        Ctx=(HashratCtx *) calloc(1,sizeof(HashratCtx));
        Ctx->Encoding |=ENCODE_HEX;
//    Ctx->OutputLength=OutputLength;
//    Ctx->SegmentLength=SegmentLength;
//    Ctx->SegmentChar=SegmentChar[0];

        ptr=GetToken(Tempstr, "|", &Ctx->HashType, 0);
        ptr=GetToken(ptr, "|", &Text, 0);
        i=MatchTokenFromList(Text, EncodingNames, 0);
        if (i > -1) Ctx->Encoding=Encodings[i];
        ptr=GetToken(ptr, "|", &LineEnding, 0);

        if (StrLen(LineEnding))
        {
            if (strcmp(LineEnding, "crlf")==0) Text=CatStr(Text,"\r\n");
            if (strcmp(LineEnding, "lf")==0) Text=CatStr(Text,"\n");
            if (strcmp(LineEnding, "cr")==0) Text=CatStr(Text,"\r");
        }

        Text=CopyStr(Text, ptr);

        ProcessData(&Tempstr, Ctx, Text, StrLen(Text));
        XDialogDisplayHash(Cmd, Tempstr);
    }

    Destroy(LineEnding);
    Destroy(Tempstr);
    Destroy(Text);

    return(RetVal);
}


void XDialogFrontend(HashratCtx *Config)
{
    char *Cmd=NULL;

    Cmd=XDialogFindXDialogCommand(Cmd, GetVar(Config->Vars, "DialogTypes"));
    while (XDialogProcess(Cmd, Config))
    {

    }
    Destroy(Cmd);
}
