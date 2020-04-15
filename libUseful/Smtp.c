#include "Smtp.h"
#include "Encodings.h"
#include "URL.h"
#include "inet.h"

#define CAP_HELO 1
#define CAP_EHLO 2
#define CAP_AUTH_LOGIN 4
#define CAP_AUTH_PLAIN 8

char *SMTPRead(char *RetStr, STREAM *S)
{
    char *Tempstr=NULL;

    RetStr=CopyStr(RetStr, "");
    Tempstr=STREAMReadLine(Tempstr, S);
    while (StrLen(Tempstr) > 3)
    {
        RetStr=CatStr(RetStr,Tempstr);
        if (Tempstr[3] == ' ') break;
        Tempstr=STREAMReadLine(Tempstr, S);
    }

    DestroyString(Tempstr);
    return(RetStr);
}


int SMTPInteract(const char *Line, STREAM *S)
{
    char *Tempstr=NULL;
    int result=FALSE;

    if (StrValid(Line)) STREAMWriteLine(Line, S);
    STREAMFlush(S);
    Tempstr=SMTPRead(Tempstr, S);

    /*
    syslog(LOG_DEBUG,"mail >> %s",Line);
    syslog(LOG_DEBUG,"mail << %s",Tempstr);
    */

    if (
        StrValid(Tempstr) &&
        ( (*Tempstr=='2') || (*Tempstr=='3') )
    ) result=TRUE;

    DestroyString(Tempstr);
    return(result);
}


int SMTPParseCapabilities(const char *String)
{
    char *Token=NULL;
    const char *ptr;
    int Caps=0;

    ptr=GetToken(String+4," ",&Token,0);
    while (ptr)
    {
        if (strcasecmp(Token, "LOGIN")==0) Caps |= CAP_AUTH_LOGIN;
        if (strcasecmp(Token, "PLAIN")==0) Caps |= CAP_AUTH_PLAIN;
        ptr=GetToken(ptr," ",&Token,0);
    }

    DestroyString(Token);

    return(Caps);
}


int SMTPHelo(STREAM *S)
{
    int RetVal=0;
    char *Tempstr=NULL, *Token=NULL;
    const char *ptr;

    ptr=LibUsefulGetValue("SMTP:HELO");
    if (! StrValid(ptr)) ptr=STREAMGetValue(S,"SMTP:HELO");
    if (! StrValid(ptr))
    {
        Token=GetExternalIP(Token);
        ptr=Token;
    }

    Tempstr=MCopyStr(Tempstr, "EHLO ", ptr, "\r\n", NULL);
    STREAMWriteLine(Tempstr,S);
    Tempstr=SMTPRead(Tempstr, S);

    if (*Tempstr == '2')
    {
        RetVal |= CAP_EHLO;
        ptr=GetToken(Tempstr,"\n",&Token,0);
        while (ptr)
        {
            StripTrailingWhitespace(Token);
            RetVal |= SMTPParseCapabilities(Token);
            ptr=GetToken(ptr,"\n",&Token,0);
        }
    }
//Some old server that doesn't support EHLO, switch to HELO
    else
    {
        Tempstr=MCopyStr(Tempstr, "HELO ", ptr, "\r\n", NULL);
        STREAMWriteLine(Tempstr,S);
        if (SMTPInteract(Tempstr, S)) RetVal |= CAP_HELO;
    }




    DestroyString(Tempstr);
    DestroyString(Token);
    return(RetVal);
}


int SMTPLogin(STREAM *S, int Caps, const char *User, const char *Pass)
{
    char *Tempstr=NULL, *Base64=NULL, *ptr;
    int len, result=FALSE;


    if (Caps & CAP_AUTH_LOGIN)
    {
        Tempstr=CopyStr(Tempstr, "AUTH LOGIN\r\n");
        if (SMTPInteract(Tempstr, S))
        {
            Base64=EncodeBytes(Base64, User, StrLen(User), ENCODE_BASE64);
            Tempstr=MCopyStr(Tempstr, Base64, "\r\n", NULL);
            if (SMTPInteract(Tempstr, S))
            {
                Base64=EncodeBytes(Base64, Pass, StrLen(Pass), ENCODE_BASE64);
                Tempstr=MCopyStr(Tempstr, Base64, "\r\n", NULL);
                if (SMTPInteract(Tempstr, S)) result=TRUE;
            }
        }
    }
    else if (Caps & CAP_AUTH_PLAIN)
    {
        Tempstr=SetStrLen(Tempstr, StrLen(User) + StrLen(Pass) +10);

				//this isn't what it looks like. The '\0' here do not terminate the string
				//as this authentication system uses a string with '\0' as separators
        len=StrLen(User);
        ptr=Tempstr;
        memcpy(ptr, User, len);
        ptr+=len;
        *ptr='\0';
        ptr++;

        len=StrLen(Pass);
        memcpy(ptr, Pass, len);
        ptr+=len;
        *ptr='\0';
        ptr++;

        Base64=EncodeBytes(Base64, Tempstr, ptr-Tempstr, ENCODE_BASE64);
        Tempstr=MCopyStr(Tempstr, "AUTH PLAIN ", Base64, "\r\n",NULL);
        if (SMTPInteract(Tempstr, S)) result=TRUE;
    }

    DestroyString(Tempstr);
    DestroyString(Base64);

    return(result);
}


int SmtpSendRecipients(const char *Recipients, STREAM *S)
{
    char *Recip=NULL, *Tempstr=NULL;
    const char *ptr;
    int result=FALSE;

    ptr=GetToken(Recipients, ",", &Recip, GETTOKEN_HONOR_QUOTES);
    while (ptr)
    {
        Tempstr=MCopyStr(Tempstr, "RCPT TO: ", Recip, "\r\n", NULL);
        if (SMTPInteract(Tempstr, S)) result=TRUE;
        ptr=GetToken(ptr, ",", &Recip, GETTOKEN_HONOR_QUOTES);
    }

    DestroyString(Tempstr);
    DestroyString(Recip);
    return(result);
}


STREAM *SMTPConnect(const char *Sender, const char *Recipients, int Flags)
{
    char *MailFrom=NULL, *Recip=NULL, *Tempstr=NULL;
    char *Proto=NULL, *User=NULL, *Pass=NULL, *Host=NULL, *PortStr=NULL;
    const char *p_MailServer, *ptr;
    int result=FALSE, Caps=0, RecipientAccepted=FALSE;
    STREAM *S;

    p_MailServer=LibUsefulGetValue("SMTP:Server");
    if (! StrValid(p_MailServer))
    {
        RaiseError(0, "SendMail", "No Mailserver set");
        return(NULL);
    }

    if (strncmp(p_MailServer,"smtp:",5) !=0) Tempstr=MCopyStr(Tempstr,"smtp:",p_MailServer,NULL);
    else Tempstr=CopyStr(Tempstr, p_MailServer);

    ParseURL(Tempstr, &Proto, &Host, &PortStr, &User, &Pass, NULL, NULL);
    if (! StrValid(PortStr)) PortStr=CopyStr(PortStr, "25");
    Tempstr=MCopyStr(Tempstr,"tcp:",Host,":",PortStr,NULL);
//syslog(LOG_DEBUG, "mailto: %s [%s] [%s] [%s]",Tempstr,Proto,Host,PortStr);

    S=STREAMOpen(Tempstr, "");
    if (S)
    {
        if (SMTPInteract("", S))
        {
            Caps=SMTPHelo(S);

            if (Caps > 0)
            {
                //try STARTTLS, the worst that will happen is the server will say no
                if ((! (Flags & SMTP_NOSSL)) && SSLAvailable() && SMTPInteract("STARTTLS\r\n", S)) DoSSLClientNegotiation(S, 0);

                if (
                    (Caps & (CAP_AUTH_LOGIN | CAP_AUTH_PLAIN)) &&
                    (StrValid(User) && StrValid(Pass))
                ) SMTPLogin(S, Caps, User, Pass);

                //Whether login was needed or not,  worked or not, let's try to send a mail
                Tempstr=MCopyStr(Tempstr, "MAIL FROM: ", Sender, "\r\n", NULL);
                if (! SMTPInteract(Tempstr, S)) RaiseError(0,"SendMail","mailserver refused sender");
                else if (! SmtpSendRecipients(Recipients, S)) RaiseError(0,"SendMail","No recipients accepted by mailserver");
                else if (! SMTPInteract("DATA\r\n", S)) RaiseError(0,"SendMail","mailserver refused mail");
                else
                {
                    //we got this far, rest of the process is handled by the calling function
                    result=TRUE;
                }
            }
            else RaiseError(0,"SendMail","Initial mailserver handshake failed");
        }
        else RaiseError(0,"SendMail","Initial mailserver handshake failed");
    }
    else RaiseError(0,"SendMail","mailserver connection failed");


    DestroyString(Tempstr);
    DestroyString(Recip);
    DestroyString(Proto);
    DestroyString(User);
    DestroyString(Pass);
    DestroyString(Host);
    DestroyString(PortStr);

    if (! result)
    {
        STREAMClose(S);
        return(NULL);
    }

    return(S);
}


int SMTPSendMail(const char *Sender, const char *Recipient, const char *Subject, const char *Body, int Flags)
{
    char *Tempstr=NULL;
    STREAM *S;
    int result=FALSE;

    S=SMTPConnect(Sender, Recipient, Flags);
    if (S)
    {
        if (! (Flags & SMTP_NOHEADER))
        {
            Tempstr=MCopyStr(Tempstr,"Date: ", GetDateStr("%a, %d %b %Y %H:%M:%S", NULL), "\r\n", NULL);
            Tempstr=MCatStr(Tempstr,"From: ", Sender, "\r\n", NULL);
            Tempstr=MCatStr(Tempstr,"To: ", Recipient, "\r\n", NULL);
            Tempstr=MCatStr(Tempstr,"Subject: ", Subject, "\r\n\r\n", NULL);
            STREAMWriteLine(Tempstr, S);
        }
        STREAMWriteLine(Body, S);
        STREAMWriteLine("\r\n.\r\n", S);
        SMTPInteract("", S);
        SMTPInteract("QUIT\r\n", S);
        result=TRUE;

        STREAMClose(S);
    }

    DestroyString(Tempstr);
    return(result);
}



int SMTPSendMailFile(const char *Sender, const char *Recipient, const char *Path, int Flags)
{
    char *Tempstr=NULL;
    STREAM *S, *F;
    int result=FALSE;

    F=STREAMOpen(Path, "r");
    if (F)
    {
        S=SMTPConnect(Sender, Recipient, Flags);
        if (S)
        {
            STREAMSendFile(F, S, 0, SENDFILE_LOOP);
            STREAMWriteLine("\r\n.\r\n", S);
            SMTPInteract("", S);
            SMTPInteract("QUIT\r\n", S);
            result=TRUE;

            STREAMClose(S);
        }
        STREAMClose(F);
    }
    else RaiseError(0,"SMTPSendMailFile","Failed to open file for sending");

    DestroyString(Tempstr);
    return(result);
}



