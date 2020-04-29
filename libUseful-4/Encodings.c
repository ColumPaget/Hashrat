#include "Encodings.h"
#include "base64.h"

int EncodingParse(const char *Str)
{
    if (strcmp(Str,"8")==0) return(ENCODE_OCTAL);
    if (strncmp(Str,"oct",3)==0) return(ENCODE_OCTAL);
    if (strcmp(Str,"10")==0) return(ENCODE_DECIMAL);
    if (strncmp(Str,"dec",3)==0) return(ENCODE_DECIMAL);
    if (strcmp(Str,"16")==0) return(ENCODE_HEX);
    if (strncmp(Str,"hex",3)==0) return(ENCODE_HEX);
    if (strcmp(Str,"64")==0) return(ENCODE_BASE64);
    if (strcmp(Str,"b64")==0) return(ENCODE_BASE64);
    if (strcmp(Str,"base64")==0) return(ENCODE_BASE64);
    if (strcmp(Str,"i64")==0) return(ENCODE_IBASE64);
    if (strcmp(Str,"p64")==0) return(ENCODE_PBASE64);
    if (strcmp(Str,"r64")==0) return(ENCODE_RBASE64);
    if (strcmp(Str,"rfc4648")==0) return(ENCODE_RBASE64);
    if (strncmp(Str,"xx",2)==0) return(ENCODE_XXENC);
    if (strncmp(Str,"uu",2)==0) return(ENCODE_UUENC);
    if (strcmp(Str,"crypt")==0) return(ENCODE_CRYPT);
    if (strcmp(Str,"z85")==0) return(ENCODE_Z85);

    return(ENCODE_NONE);
}


char *Ascii85(char *RetStr, const char *Bytes, int ilen, const char *CharMap)
{
    const char *ptr, *block, *end;
    uint32_t val, mod;
    int olen=0, i;
    char Buff[6];

    end=Bytes+ilen;
    for (ptr=Bytes; ptr < end; )
    {
        block=ptr;
        val = ((*ptr & 0xFF) << 24);
        ptr++;
        if (ptr < end)
        {
            val |= ((*ptr & 0xFF) << 16);
            ptr++;
        }

        if (ptr < end)
        {
            val |= ((*ptr & 0xFF) << 8);
            ptr++;
        }

        if (ptr < end)
        {
            val |= (*ptr & 0xFF);
            ptr++;
        }

        if (val==0) strcpy(Buff,"z");
        else for (i=4; i >-1; i--)
            {
                mod=val % 85;
                val /= 85;
                Buff[i]=CharMap[mod & 0xFF];
            }

        //we only add as many characters as we encoded
        //so for the last chracter
        RetStr=CatStrLen(RetStr,Buff,ptr-block);
    }

    return(RetStr);
}


char *EncodeBytes(char *Buffer, const char *Bytes, int len, int Encoding)
{
    char *Tempstr=NULL, *RetStr=NULL;
    int i;

    RetStr=CopyStr(Buffer,"");
    switch (Encoding)
    {
    case ENCODE_BASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        to64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len);
        break;

    case ENCODE_IBASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,IBASE64_CHARS,'\0');
        break;

    case ENCODE_PBASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,PBASE64_CHARS,'\0');
        break;

    case ENCODE_RBASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,RBASE64_CHARS,'=');
        break;

    case ENCODE_CRYPT:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,CRYPT_CHARS,'\0');
        break;

    case ENCODE_XXENC:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,XXENC_CHARS,'+');
        break;

    case ENCODE_UUENC:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,UUENC_CHARS,'\'');
        break;

    case ENCODE_ASCII85:
        RetStr=Ascii85(RetStr,Bytes,len,ASCII85_CHARS);
        break;

    case ENCODE_Z85:
        RetStr=Ascii85(RetStr,Bytes,len,Z85_CHARS);
        break;

    case ENCODE_OCTAL:
        for (i=0; i < len; i++)
        {
            Tempstr=FormatStr(Tempstr,"%03o",Bytes[i] & 255);
            RetStr=CatStr(RetStr,Tempstr);
        }
        break;

    case ENCODE_DECIMAL:
        for (i=0; i < len; i++)
        {
            Tempstr=FormatStr(Tempstr,"%03d",Bytes[i] & 255);
            RetStr=CatStr(RetStr,Tempstr);
        }
        break;

    case ENCODE_HEX:
        for (i=0; i < len; i++)
        {
            Tempstr=FormatStr(Tempstr,"%02x",Bytes[i] & 255);
            RetStr=CatStr(RetStr,Tempstr);
        }
        break;

    case ENCODE_HEXUPPER:
        for (i=0; i < len; i++)
        {
            Tempstr=FormatStr(Tempstr,"%02X",Bytes[i] & 255);
            RetStr=CatStr(RetStr,Tempstr);
        }
        break;


    default:
        RetStr=SetStrLen(RetStr, len);
        memcpy(RetStr,Bytes,len);
        RetStr[len]='\0';
        break;
    }

    DestroyString(Tempstr);
    return(RetStr);
}


int DecodeBytes(char **Return, const char *Text, int Encoding)
{
    long len=0, val, i=0;
    const char *ptr, *end;

    len=StrLen(Text);
    *Return=SetStrLen(*Return,len);
    memset(*Return,0,len);
    switch (Encoding)
    {
    case ENCODE_BASE64:
        len=Radix64tobits(*Return,Text,BASE64_CHARS,'=');
        break;
        break;

    case ENCODE_IBASE64:
        len=Radix64tobits(*Return,Text,IBASE64_CHARS,'\0');
        break;
        break;

    case ENCODE_PBASE64:
        len=Radix64tobits(*Return,Text,PBASE64_CHARS,'\0');
        break;
        break;

    case ENCODE_CRYPT:
        len=Radix64tobits(*Return,Text,CRYPT_CHARS,'\0');
        break;
        break;

    case ENCODE_XXENC:
        len=Radix64tobits(*Return,Text,XXENC_CHARS,'+');
        break;
        break;

    case ENCODE_UUENC:
        len=Radix64tobits(*Return,Text,UUENC_CHARS,'\'');
        break;
        break;

    case ENCODE_ASCII85:
        //RetStr=Ascii85(RetStr,Bytes,len,ASCII85_CHARS); break;
        break;

    case ENCODE_Z85:
        //RetStr=Ascii85(RetStr,Bytes,len,Z85_CHARS); break;
        break;

    case ENCODE_OCTAL:
        ptr=Text;
        end=ptr+len;
        while (ptr < end)
        {
            strntol(&ptr, 3, 8, &val);
            (*Return)[i]=val & 0xFF;
            i++;
        }
        len=i;
        break;

    case ENCODE_DECIMAL:
        ptr=Text;
        end=ptr+len;
        while (ptr < end)
        {
            strntol(&ptr, 3, 10, &val);
            (*Return)[i]=val & 0xFF;
            i++;
        }
        len=i;
        break;

    case ENCODE_HEX:
    case ENCODE_HEXUPPER:
        ptr=Text;
        end=ptr+len;
        while (ptr < end)
        {
            strntol(&ptr, 2, 16, &val);
            (*Return)[i]=val & 0xFF;
            i++;
        }
        len=i;
        break;

    default:
        break;
    }

    return(len);
}



char *DecodeToText(char *RetStr, const char *Text, int Encoding)
{
    DecodeBytes(&RetStr, Text, Encoding);
    return(RetStr);
}
