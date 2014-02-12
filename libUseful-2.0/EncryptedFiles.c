#include "EncryptedFiles.h"

char *FormatEncryptArgs(char *RetBuff, int Flags, const char *Cipher, const char *Key, const char *InitVector, const char *Salt )
{
char *EncryptArgs=NULL, *Tempstr=NULL;

EncryptArgs=CopyStr(RetBuff,"");

EncryptArgs=CopyStr(EncryptArgs,"Cipher=");
EncryptArgs=CatStr(EncryptArgs,Cipher);


if (StrLen(Key))
{
if (Flags & FLAG_HEXKEY) Tempstr=FormatStr(Tempstr," hexkey='%s'",Key);
else Tempstr=FormatStr(Tempstr," key='%s'",Key);
EncryptArgs=CatStr(EncryptArgs,Tempstr);
}

if (StrLen(InitVector))
{
if (Flags & FLAG_HEXIV) Tempstr=FormatStr(Tempstr," hexiv='%s'",InitVector);
else Tempstr=FormatStr(Tempstr," iv='%s'",InitVector);
EncryptArgs=CatStr(EncryptArgs,Tempstr);
}

if (StrLen(Salt))
{
if (Flags & FLAG_HEXSALT) Tempstr=FormatStr(Tempstr," hexsalt='%s'",Salt);
else Tempstr=FormatStr(Tempstr," salt='%s'",Salt);
EncryptArgs=CatStr(EncryptArgs,Tempstr);
}


if (Flags & FLAG_NOPAD_DATA) EncryptArgs=CatStr(EncryptArgs," PadBlock=N");

return(EncryptArgs);
}



int AddEncryptionHeader(STREAM *S, int Flags, const char *Cipher, const char *Key, const char *InitVector, const char *Salt)
{
char *EncryptArgs=NULL;
char *Tempstr=NULL;



EncryptArgs=FormatEncryptArgs(EncryptArgs,Flags, Cipher, "", InitVector,Salt);
Tempstr=FormatStr(Tempstr,"ENCR %s\n",EncryptArgs);
STREAMWriteLine(Tempstr,S);

EncryptArgs=FormatEncryptArgs(EncryptArgs,Flags, Cipher, Key, InitVector,Salt);
if (! STREAMAddStandardDataProcessor(S,"Crypto",Cipher,EncryptArgs)) printf("Failed to initialise encryption!\n");

DestroyString(Tempstr);
DestroyString(EncryptArgs);
}



void HandleDecryptionHeader(STREAM *S, const char *Header, const char *Key)
{
const char *ptr;
char *Tempstr=NULL;

ptr=Header;
if (strncmp(ptr,"ENCR ",5)==0) ptr+=5;
Tempstr=MCopyStr(Tempstr,ptr," key='",Key,"'",NULL);

fprintf(stderr,"SASDP: [%s]\n",Tempstr);
STREAMAddStandardDataProcessor(S,"crypto","",Tempstr);
DestroyString(Tempstr);
}


