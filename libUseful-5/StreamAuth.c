#include "StreamAuth.h"
#include "OpenSSL.h"
#include "PasswordFile.h"
#include "HttpUtil.h"
#include "Encodings.h"
#include "StringList.h"


static int STREAMAuthValueFile(const char *Path, const char *Value)
{

    return(FALSE);
}

//did the client provide an SSL certificate as authentication?
static int STREAMAuthProcessCertificate(STREAM *S, const char *CertName, const char *CommonName)
{
    char *Require=NULL;
    int AuthResult=FALSE;

#ifdef HAVE_LIBSSL
//does the certificate name/subject match out expectation?
    Require=OpenSSLCertDetailsGetCommonName(Require, STREAMGetValue(S, CommonName));
    if (CompareStr(CertName, Require)==0)
    {
        //is certificate valid
        if (CompareStr(STREAMGetValue(S, "SSL:CertificateVerify"), "OK")==0) AuthResult=TRUE;
    }
#endif

    Destroy(Require);
    return(AuthResult);
}




static int STREAMBasicAuthPasswordFile(const char *Path, STREAM *S)
{
    char *User=NULL, *Password=NULL;
    const char *ptr;
    int AuthResult=FALSE;

		if (StrValid(Path))
		{
    User=CopyStr(User, STREAMGetValue(S, "AUTH:User"));
    Password=CopyStr(Password, STREAMGetValue(S, "AUTH:Password"));
    AuthResult=PasswordFileCheck(Path, User, Password, NULL);
		}

    Destroy(User);
    Destroy(Password);

    return(AuthResult);
}



static int STREAMAuthProcess(STREAM *S, const char *AuthTypes)
{
    char *Key=NULL, *Value=NULL, *Tempstr=NULL;
    const char *ptr;
    int AuthResult=FALSE;

    //we are passed a list of authentication methods as name-value pairs,
    //where the name is the authentication method type, and the value is
    //data related to that type (sometimes a password or ip, sometimes a path
    //to a file containing the actual passwords etc)
    ptr=GetNameValuePair(AuthTypes, ";", ":",&Key, &Value);
    while (ptr)
    {
        if (CompareStrNoCase(Key, "basic")==0)
        {
            Tempstr=EncodeBytes(Tempstr, Value, StrLen(Value), ENCODE_BASE64);
            if (CompareStr(Tempstr, STREAMGetValue(S, "Auth:Basic"))==0) AuthResult=TRUE;
        }
        else if (
            (CompareStrNoCase(Key, "certificate")==0) ||
            (CompareStrNoCase(Key, "cert")==0)
        )  AuthResult=STREAMAuthProcessCertificate(S, Value, "SSL:CertificateSubject");
        else if (CompareStrNoCase(Key, "issuer")==0) AuthResult=STREAMAuthProcessCertificate(S, Value, "SSL:CertificateIssuer");
        else if (CompareStrNoCase(Key, "cookie")==0)
        {
            if (InStringList(STREAMGetValue(S, Key), Value, ",")) AuthResult=TRUE;
        }
        else if (CompareStrNoCase(Key, "ip")==0)
        {
            if (InStringList(GetRemoteIP(S->in_fd), Value, ",")) AuthResult=TRUE;
        }
        else if (CompareStrNoCase(Key, "password-file")==0) AuthResult=STREAMBasicAuthPasswordFile(Value, S);

        ptr=GetNameValuePair(ptr, ";", ":",&Key, &Value);
    }

    if (AuthResult==TRUE) STREAMSetValue(S, "STREAM:Authenticated", "Y");


    Destroy(Key);
    Destroy(Value);
    Destroy(Tempstr);

    return(AuthResult);
}



int STREAMAuth(STREAM *S)
{
    const char *ptr;

    ptr=STREAMGetValue(S, "AUTH:Types");
    if (! StrValid(ptr)) return(TRUE);

    if (STREAMAuthProcess(S, ptr))
		{
      S->Flags |= LU_SS_AUTH;
			return(TRUE);
		}

    return(FALSE);
}
