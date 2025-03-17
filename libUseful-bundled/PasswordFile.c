#include "PasswordFile.h"
#include "Entropy.h"
#include "Hash.h"



char *PasswordFileGenerateEntry(char *RetStr, const char *User, const char *PassType, const char *Password, const char *Extra)
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

    Tempstr=QuoteCharsInStr(Tempstr, Extra, ":");
    RetStr=MCopyStr(RetStr, User, ":", PassType, ":", Salt, ":", Hash, ":", Tempstr, "\n", NULL);

    Destroy(Tempstr);
    Destroy(Hash);
    Destroy(Salt);

    return(RetStr);
}


int PasswordFileAdd(const char *Path, const char *PassType, const char *User, const char *Password, const char *Extra)
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


        Tempstr=PasswordFileGenerateEntry(Tempstr, User, PassType, Password, Extra);
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


int PasswordFileAppend(const char *Path, const char *PassType, const char *User, const char *Password, const char *Extra)
{
    STREAM *F;
    char *Tempstr=NULL, *Token=NULL;
    int RetVal=FALSE;

    F=STREAMOpen(Path, "a");
    if (! F) return(FALSE);

    STREAMSeek(F, 0, SEEK_END);
    Tempstr=PasswordFileGenerateEntry(Tempstr, User, PassType, Password, Extra);
    STREAMWriteLine(Tempstr, F);
    STREAMClose(F);

    Destroy(Tempstr);
    Destroy(Token);

    return(RetVal);
}





static int PasswordFileMatchItem(const char *Data, const char *User, const char *Password)
{
    char *Tempstr=NULL, *Token=NULL, *Salt=NULL, *ProvidedCred=NULL,  *StoredCred=NULL;
    const char *ptr;
    int result=FALSE;

    ptr=GetToken(Data, ":", &Token, 0);
    if (strcmp(Token, User)==0)
    {
        ptr=GetToken(ptr, ":", &Token, 0);
        ptr=GetToken(ptr, ":", &Salt, 0);
        ptr=GetToken(ptr, ":", &StoredCred, 0);

        if ( (! StrValid(Token)) || (strcmp(Token, "plain")==0) ) ProvidedCred=CopyStr(ProvidedCred, Password);
        else
        {
            Tempstr=MCopyStr(Tempstr, Salt, Password, NULL);
            HashBytes(&ProvidedCred, Token, Tempstr, StrLen(Tempstr), ENCODE_BASE64);
        }

        if (strcmp(ProvidedCred, StoredCred) == 0) result=TRUE;
    }

    Destroy(Tempstr);
    Destroy(StoredCred);
    Destroy(ProvidedCred);
    Destroy(Token);
    Destroy(Salt);

    return(result);
}


int PasswordFileCheck(const char *Path, const char *User, const char *Password, char **Extra)
{
    STREAM *F;
    char *Tempstr=NULL;
    const char *ptr;
    int result=FALSE;

    if (! StrValid(Path)) return(FALSE);
    if (! StrValid(User)) return(FALSE);

    F=STREAMOpen(Path, "r");
    if (F)
    {
        Tempstr=STREAMReadLine(Tempstr, F);
        while (Tempstr)
        {
            StripTrailingWhitespace(Tempstr);
            result=PasswordFileMatchItem(Tempstr, User, Password);
            if (result)
            {
                if (Extra)
                {
                    ptr=strrchr(Tempstr, ':');
                    *Extra=UnQuoteStr(*Extra, ptr+1);
                }
                break;
            }

            Tempstr=STREAMReadLine(Tempstr, F);
        }

        STREAMClose(F);
    }
    else RaiseError(ERRFLAG_DEBUG, "PasswordFileCheck", "can't open %s", Path);
    Destroy(Tempstr);

    return(result);
}
