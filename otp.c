#include "otp.h"
#include "output.h"


void OTPProcess(HashratCtx *Ctx)
{
    char *Hash=NULL, *Tempstr=NULL;
    int validity, period, digits;

    digits=atoi(GetVar(Ctx->Vars, "OTP:Digits"));
    period=atoi(GetVar(Ctx->Vars, "OTP:Period"));

    validity=period - (time(NULL) % period);
    Hash=TOTP(Hash, GetVar(Ctx->Vars, "HashType"), GetVar(Ctx->Vars, "EncryptionKey"), ENCODE_BASE32, digits, period);
    Tempstr=FormatStr(Tempstr, "valid for: %d seconds", validity);
    OutputHash(Ctx, Hash, Tempstr);

    Destroy(Tempstr);
    Destroy(Hash);
}


void OTPGoogle(HashratCtx *Ctx, const char *Secret)
{
    Ctx->Action = ACT_OTP;
    SetVar(Ctx->Vars, "HashType", "sha1");
    SetVar(Ctx->Vars, "EncryptionKey", Secret);
    SetVar(Ctx->Vars, "OTP:Digits", "6");
    SetVar(Ctx->Vars, "OTP:Period", "30");
}


//otpauth://totp/ACME%20Co:john.doe@email.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ&issuer=ACME%20Co&algorithm=SHA1&digits=6&period=30
void OTPParse(HashratCtx *Ctx, const char *Input)
{
    char *Proto=NULL, *Host=NULL, *Path=NULL, *Args=NULL;
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    Ctx->Action=ACT_OTP;

    if (strncmp(Input, "otpauth:", 8)==0)
    {
        ParseURL(Input, &Proto, &Host, NULL, NULL, NULL, &Path, &Args);

        ptr=GetNameValuePair(Args, "&", "=", &Name, &Value);
        while (ptr)
        {
            if (strcasecmp(Name, "algorithm")==0) SetVar(Ctx->Vars, "HashType", Value);
            if (strcasecmp(Name, "secret")==0) SetVar(Ctx->Vars, "EncryptionKey", Value);
            if (strcasecmp(Name, "digits")==0) SetVar(Ctx->Vars, "OTP:Digits", Value);
            if (strcasecmp(Name, "period")==0) SetVar(Ctx->Vars, "OTP:Period", Value);
            ptr=GetNameValuePair(ptr, "&", "=", &Name, &Value);
        }
    }
    else SetVar(Ctx->Vars, "EncryptionKey", Input);

    Destroy(Proto);
    Destroy(Host);
    Destroy(Path);
    Destroy(Args);
    Destroy(Name);
    Destroy(Value);
}
