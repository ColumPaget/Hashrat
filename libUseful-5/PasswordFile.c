#include "PasswordFile.h"
#include "Entropy.h"
#include "Hash.h"



char *PasswordFileGenerateEntry(char *RetStr, const char *User, const char *PassType, const char *Password)
{
    char *Salt=NULL, *Hash=NULL, *Tempstr=NULL;

    if (StrEnd(PassType) || (strcasecmp(PassType, "plain")==0) )
    {
        Salt=CopyStr(Salt, "");
        Hash=CopyStr(Hash, Password);
    }
    else
    {
        Salt=GetRandomAlphabetStr(Salt, 20);
        Tempstr=MCatStr(Tempstr, Salt, Password, NULL);
        HashBytes(&Hash, PassType, Tempstr, StrLen(Tempstr), ENCODE_BASE64);
    }

    RetStr=MCopyStr(RetStr, User, ":", PassType, ":", Salt, ":", Hash, "\n", NULL);

    Destroy(Tempstr);
    Destroy(Hash);
    Destroy(Salt);

    return(RetStr);
}


int PasswordFileAdd(const char *Path, const char *PassType, const char *User, const char *Password)
{
    STREAM *Old, *New;
    char *Tempstr=NULL, *Token=NULL, *Salt=NULL;
    int RetVal=FALSE;

    Old=STREAMOpen(Path, "r");

    Tempstr=MCopyStr(Tempstr, Path, "+", NULL);
    New=STREAMOpen(Tempstr, "w");
    if (New)
    {
        if (Old)
        {
            Tempstr=STREAMReadLine(Tempstr, Old);
            while (Tempstr)
            {
                GetToken(Tempstr, ":", &Token, 0);
                if (strcmp(User, Token) !=0) STREAMWriteLine(Tempstr, New);
                Tempstr=STREAMReadLine(Tempstr, Old);
            }
            STREAMClose(Old);
        }


        Tempstr=PasswordFileGenerateEntry(Tempstr, User, PassType, Password);
        STREAMWriteLine(Tempstr, New);

        rename(New->Path, Path);
        STREAMClose(New);

        RetVal=TRUE;
    }


    Destroy(Tempstr);
    Destroy(Token);
    Destroy(Salt);

    return(RetVal);
}


int PasswordFileAppend(const char *Path, const char *PassType, const char *User, const char *Password)
{
    STREAM *F;
    char *Tempstr=NULL, *Token=NULL;
    int RetVal=FALSE;

    F=STREAMOpen(Path, "a");
    if (! F) return(FALSE);

    STREAMSeek(F, 0, SEEK_END);
    Tempstr=PasswordFileGenerateEntry(Tempstr, User, PassType, Password);
    STREAMWriteLine(Tempstr, F);
    STREAMClose(F);

    Destroy(Tempstr);
    Destroy(Token);

    return(RetVal);
}





static int PasswordFileMatchItem(const char *Data, const char *User, const char *Password)
{
    char *Token=NULL, *Salt=NULL, *Compare=NULL, *Tempstr=NULL;
    const char *ptr;
    int result=FALSE;

    ptr=GetToken(Data, ":", &Token, 0);
    if (strcmp(Token, User)==0)
    {
        ptr=GetToken(ptr, ":", &Token, 0);
        ptr=GetToken(ptr, ":", &Salt, 0);

        if ( (! StrValid(Token)) || (strcmp(Token, "plain")==0) ) Compare=CopyStr(Compare, Password);
        else
        {
            Tempstr=MCopyStr(Tempstr, Salt, Password, NULL);
            HashBytes(&Compare, Token, Tempstr, StrLen(Tempstr), ENCODE_BASE64);
        }

        if (strcmp(Compare, ptr) == 0) result=TRUE;
    }

    Destroy(Tempstr);
    Destroy(Compare);
    Destroy(Token);
    Destroy(Salt);

    return(result);
}


int PasswordFileCheck(const char *Path, const char *User, const char *Password)
{
    STREAM *F;
    char *Tempstr=NULL;
    int result=FALSE;

    F=STREAMOpen(Path, "r");
    if (F)
    {
        Tempstr=STREAMReadLine(Tempstr, F);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            result=PasswordFileMatchItem(Tempstr, User, Password);
            if (result) break;
            Tempstr=STREAMReadLine(Tempstr, F);
        }

        STREAMClose(F);
    }

    Destroy(Tempstr);

    return(result);
}
