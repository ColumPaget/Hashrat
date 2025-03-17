#include "Encodings.h"
#include "base64.h"
#include "base32.h"
#include "Http.h"



char *EncodeQuoted(char *Return, const char *Input, int len, char QuoteChar)
{
    const char *ptr;
    char Hex[4];

    for (ptr=Input; ptr < (Input + len); ptr++)
    {
        if ( (*ptr < 32) || (*ptr > 127) || (*ptr == '=') )
        {
            snprintf(Hex, 3, "%02x", (*ptr) & 0xFF);
            Return=MCatStr(Return, "=", Hex, NULL);
        }
        else Return=AddCharToStr(Return, *ptr);
    }

    return(Return);
}


int DecodeQuoted(char **Return, const char *Text, char QuoteChar)
{
    const char *ptr;
    char Hex[3];
    int len=0;

    for (ptr=Text; *ptr != '\0'; ptr++)
    {
        if (*ptr==QuoteChar)
        {
            ptr++;
            if (*ptr=='\0') break;
            else if (*ptr =='\r') ptr++;

            if (*ptr=='\0') break;
            else if (*ptr !='\n')
            {
                strncpy(Hex, ptr, 2);
                *Return=AddCharToBuffer(*Return, len, strtol(Hex, NULL, 16));
                len++;
                ptr++;
                if (*ptr=='\0') break;
            }
        }
        else
        {
            *Return=AddCharToBuffer(*Return, len, *ptr);
            len++;
        }
    }

    return(len);
}



char *EncodeYenc(char *Return, const char *Input, int len, char QuoteChar)
{
    const char *ptr;
    char echar;

    for (ptr=Input; ptr < (Input + len); ptr++)
    {
        //shift the character up some bytes, because the null byte is a common occurance
        //in binary files, and we don't want to escape it, increasing the size of the
        //file. We'll escape some less common byte in it's place
        echar = (*ptr + 42) % 256; //of course it's 42
        if ( (echar==0) || (echar=='\r') || (echar=='\n') || (echar==QuoteChar))
        {
            Return=AddCharToStr(Return, QuoteChar);
            Return=AddCharToStr(Return, (echar + 62) % 256);
        }
        else Return=AddCharToStr(Return, echar);
    }

    return(Return);
}


int DecodeYenc(char **Return, const char *Text, char QuoteChar)
{
    const char *ptr;
    int len=0, echar;

    for (ptr=Text; *ptr != '\0'; ptr++)
    {
        if (*ptr==QuoteChar)
        {
            ptr++;
            if (*ptr=='\0') break;
            echar=*ptr - 62;
        }
        else echar=*ptr;

        *Return=AddCharToBuffer(*Return, len, echar - 42);
        len++;
    }

    return(len);
}




//mostly a helper function for environments where integer constants are not convinient
int EncodingParse(const char *Str)
{
    int Encode=ENCODE_HEX;

    if (StrValid(Str))
    {
        switch (*Str)
        {
        case '1':
            if (strcasecmp(Str,"16")==0) Encode=ENCODE_HEX;
            else if (strcasecmp(Str,"10")==0) Encode=ENCODE_DECIMAL;
            break;

        case '2':
            if (strcasecmp(Str,"2")==0) Encode=ENCODE_BINARY;
            break;


        case '3':
            if (strcasecmp(Str,"32")==0) Encode=ENCODE_BASE32;
            break;

        case '6':
            if (strcasecmp(Str,"64")==0) Encode=ENCODE_BASE64;
            break;

        case '8':
            if (strcasecmp(Str,"8")==0) Encode=ENCODE_OCTAL;
            break;

        case 'a':
        case 'A':
            if (strcasecmp(Str,"a85")==0) Encode=ENCODE_ASCII85;
            else if (strcasecmp(Str,"ascii85")==0) Encode=ENCODE_ASCII85;
            else if (strcasecmp(Str,"asci85")==0) Encode=ENCODE_ASCII85;
            break;

        case 'b':
        case 'B':
            if (strcasecmp(Str, "base2")==0) Encode=ENCODE_BINARY;
            else if (strcasecmp(Str, "bin")==0) Encode=ENCODE_BINARY;
            else if (strcasecmp(Str, "binary")==0) Encode=ENCODE_BINARY;
            else if (strcasecmp(Str,"base8")==0) Encode=ENCODE_OCTAL;
            else if (strcasecmp(Str,"base16")==0) Encode=ENCODE_HEX;
            else if (strcasecmp(Str,"base32")==0) Encode=ENCODE_BASE32;
            else if (strcasecmp(Str,"b32")==0) Encode=ENCODE_BASE32;
            else if (strcasecmp(Str,"bech32")==0) Encode=ENCODE_BECH32;
            else if (strcasecmp(Str,"base64")==0) Encode=ENCODE_BASE64;
            else if (strcasecmp(Str,"b64")==0) Encode=ENCODE_BASE64;
            break;

        case 'c':
        case 'C':
            if (CompareStr(Str,"crypt")==0) Encode=ENCODE_CRYPT;
            else if (strcasecmp(Str,"c32")==0) Encode=ENCODE_CBASE32;
            else if (strcasecmp(Str,"cbase32")==0) Encode=ENCODE_CBASE32;
            break;

        case 'd':
        case 'D':
            if (strcasecmp(Str,"dec")==0) Encode=ENCODE_DECIMAL;
            else if (strcasecmp(Str,"decimal")==0) Encode=ENCODE_DECIMAL;
            break;

        case 'h':
        case 'H':
            if (strcasecmp(Str,"hex")==0) Encode=ENCODE_HEX;
            else if (strcasecmp(Str,"hexupper")==0) Encode=ENCODE_HEXUPPER;
            else if (strcasecmp(Str,"http")==0) Encode=ENCODE_QUOTED_HTTP;
            else if (strcasecmp(Str,"h32")==0) Encode=ENCODE_HBASE32;
            else if (strcasecmp(Str,"hbase32")==0) Encode=ENCODE_HBASE32;
            break;

        case 'm':
        case 'M':
            if (strcasecmp(Str,"mime")==0) Encode=ENCODE_QUOTED_MIME;
            else if (strcasecmp(Str,"mime")==0) Encode=ENCODE_QUOTED_MIME;
            break;


        case 'o':
        case 'O':
            if (strcasecmp(Str,"oct")==0) Encode=ENCODE_OCTAL;
            else if (strcasecmp(Str,"octal")==0) Encode=ENCODE_OCTAL;
            break;

        case 'i':
        case 'I':
            if (strcasecmp(Str,"ibase64")==0) Encode=ENCODE_IBASE64;
            else if (strcasecmp(Str,"i64")==0) Encode=ENCODE_IBASE64;
            break;

        case 'p':
        case 'P':
            if (strcasecmp(Str,"pbase64")==0) Encode=ENCODE_PBASE64;
            else if (strcasecmp(Str,"p64")==0) Encode=ENCODE_PBASE64;
            break;

        case 'q':
        case 'Q':
            if (CompareStr(Str,"quoted-printable")==0) Encode=ENCODE_QUOTED_MIME;
            else if (CompareStr(Str,"quoted")==0) Encode=ENCODE_QUOTED_MIME;
            else if (CompareStr(Str,"quoted-http")==0) Encode=ENCODE_QUOTED_HTTP;
            break;


        case 'r':
        case 'R':
            if (CompareStr(Str,"r64")==0) Encode=ENCODE_RBASE64;
            else if (CompareStr(Str,"rbase64")==0) Encode=ENCODE_RBASE64;
            else if (CompareStr(Str,"rfc4648")==0) Encode=ENCODE_RBASE64;
            break;

        case 'u':
        case 'U':
            if (strcasecmp(Str,"uu")==0) Encode=ENCODE_UUENC;
            else if (strcasecmp(Str,"uuencode")==0) Encode=ENCODE_UUENC;
            else if (strcasecmp(Str,"uuenc")==0) Encode=ENCODE_UUENC;
            break;

        case 'w':
        case 'W':
            if (strcasecmp(Str,"wbase32")==0) Encode=ENCODE_WBASE32;
            else if (strcasecmp(Str,"w32")==0) Encode=ENCODE_WBASE32;
            break;

        case 'x':
        case 'X':
            if (strcasecmp(Str,"xx")==0) Encode=ENCODE_XXENC;
            else if (strcasecmp(Str,"xxencode")==0) Encode=ENCODE_XXENC;
            else if (strcasecmp(Str,"xxenc")==0) Encode=ENCODE_XXENC;
            break;

        case 'y':
        case 'Y':
            if (strcasecmp(Str,"yenc")==0) Encode=ENCODE_YENCODE;
            else if (strcasecmp(Str,"yencode")==0) Encode=ENCODE_YENCODE;
            break;

        case 'z':
        case 'Z':
            if (strcasecmp(Str,"z85")==0) Encode=ENCODE_Z85;
            else if (strcasecmp(Str,"z32")==0) Encode=ENCODE_ZBASE32;
            else if (strcasecmp(Str,"zbase32")==0) Encode=ENCODE_ZBASE32;
            break;
        }
    }

    return(Encode);
}




char *Ascii85(char *RetStr, const char *Bytes, int ilen, const char *CharMap)
{
    const char *ptr, *block, *end;
    uint32_t val, mod;
    int i;
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


char *EncodeBytes(char *RetStr, const char *Bytes, int len, int Encoding)
{
    char *Tempstr=NULL;
    char PrintBuff[10];
    int i, olen;

    switch (Encoding)
    {
    case ENCODE_BINARY:
        if (len > 0) RetStr=encode_bcd_bytes(RetStr, Bytes, len);
        break;

    case ENCODE_OCTAL:
        olen=StrLen(RetStr);
        for (i=0; i < len; i++)
        {
            snprintf(PrintBuff,8,"%03o",Bytes[i] & 255);
            RetStr=AddBytesToBuffer(RetStr, olen, PrintBuff, 3);
            olen+=3;
        }
        StrUnsafeTrunc(RetStr, olen);
        break;

    case ENCODE_DECIMAL:
        olen=StrLen(RetStr);
        for (i=0; i < len; i++)
        {
            snprintf(PrintBuff,8,"%03d",Bytes[i] & 255);
            RetStr=AddBytesToBuffer(RetStr, olen, PrintBuff, 3);
            olen+=3;
        }
        StrUnsafeTrunc(RetStr, olen);
        break;

    case ENCODE_HEX:
        olen=StrLen(RetStr);
        for (i=0; i < len; i++)
        {
            snprintf(PrintBuff,8,"%02x",Bytes[i] & 255);
            RetStr=AddBytesToBuffer(RetStr, olen, PrintBuff, 2);
            olen+=2;
        }
        StrUnsafeTrunc(RetStr, olen);
        break;

    case ENCODE_HEXUPPER:
        for (i=0; i < len; i++)
        {
            snprintf(PrintBuff,8,"%02X",Bytes[i] & 255);
            RetStr=AddBytesToBuffer(RetStr, olen, PrintBuff, 2);
            olen+=2;
        }
        StrUnsafeTrunc(RetStr, olen);
        break;


    case ENCODE_BASE32:
        RetStr=base32encode(RetStr, Bytes, len, BASE32_RFC4648_CHARS, '=');
        break;

    case ENCODE_CBASE32:
        RetStr=base32encode(RetStr, Bytes, len, BASE32_CROCKFORD_CHARS, '\0');
        break;

    case ENCODE_HBASE32:
        RetStr=base32encode(RetStr, Bytes, len, BASE32_HEX_CHARS, '=');
        break;

    case ENCODE_ZBASE32:
        RetStr=base32encode(RetStr, Bytes, len, BASE32_ZBASE32_CHARS, '\0');
        break;

    case ENCODE_WBASE32:
        RetStr=base32encode(RetStr, Bytes, len, BASE32_WORDSAFE_CHARS, '=');
        break;

    case ENCODE_BECH32:
        RetStr=base32encode(RetStr, Bytes, len, BASE32_BECH32_CHARS, '\0');
        break;

    case ENCODE_BASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        to64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len);
        break;

    case ENCODE_IBASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes, len, IBASE64_CHARS,'\0');
        break;

    case ENCODE_PBASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes, len, PBASE64_CHARS,'=');
        break;

    case ENCODE_RBASE64:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes, len, RBASE64_CHARS,'=');
        break;

    case ENCODE_CRYPT:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes, len, CRYPT_CHARS,'\0');
        break;

    case ENCODE_XXENC:
        RetStr=SetStrLen(RetStr,len * 4);
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes, len, XXENC_CHARS,'\0');
        break;

    case ENCODE_UUENC:
        RetStr=SetStrLen(RetStr,len * 4);
        Tempstr=CopyStr(Tempstr, Bytes);
        strrep(Tempstr, ' ', '`');
        Radix64frombits((unsigned char *) RetStr,(unsigned char *) Bytes,len,UUENC_CHARS,'`');
        break;

    case ENCODE_ASCII85:
        RetStr=Ascii85(RetStr,Bytes,len,ASCII85_CHARS);
        break;

    case ENCODE_Z85:
        RetStr=Ascii85(RetStr,Bytes,len,Z85_CHARS);
        break;

    case ENCODE_QUOTED_MIME:
        RetStr=EncodeQuoted(RetStr, Bytes, len, '=');
        break;

    case ENCODE_YENCODE:
        RetStr=EncodeYenc(RetStr, Bytes, len, '=');
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


char *EncodingPad(char *RetStr, const char *Input, int InputLen, char PadByte, int BlockSize)
{
    int val;

    RetStr=CopyStr(RetStr, Input);
    val=4 - InputLen % 4;
    if (val==0) return(RetStr);
    if (val==4) return(RetStr);

    return(PadStr(RetStr, PadByte, val));
}


int DecodeBytes(char **Return, const char *Text, int Encoding)
{
    long len=0, val, i=0;
    const char *ptr, *end;
    char *Padded=NULL;

    len=StrLen(Text);
    //for all these encodings the result will be no bigger than the input
    *Return=SetStrLen(*Return,len);
    memset(*Return,0,len);


    switch (Encoding)
    {
    case ENCODE_BINARY:
        ptr=Text;
        end=ptr+len;
        while (ptr < end)
        {
            strntol(&ptr, 8, 2, &val);
            (*Return)[i]=val & 0xFF;
            i++;
        }
        len=i;
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

    case ENCODE_BASE32:
        len=base32decode((unsigned char *) *Return, Text, BASE32_RFC4648_CHARS);
        break;

    case ENCODE_CBASE32:
        len=base32decode((unsigned char *) *Return, Text, BASE32_CROCKFORD_CHARS);
        break;

    case ENCODE_HBASE32:
        len=base32decode((unsigned char *) *Return, Text, BASE32_HEX_CHARS);
        break;

    case ENCODE_WBASE32:
        len=base32decode((unsigned char *) *Return, Text, BASE32_WORDSAFE_CHARS);
        break;

    case ENCODE_ZBASE32:
        len=base32decode((unsigned char *) *Return, Text, BASE32_ZBASE32_CHARS);
        break;

    case ENCODE_BECH32:
        len=base32decode((unsigned char *) *Return, Text, BASE32_BECH32_CHARS);
        break;

    case ENCODE_BASE64:
        Padded=EncodingPad(Padded, Text, len, '=', 4);
        len=Radix64tobits(*Return, Padded, BASE64_CHARS,'=');
        break;

    case ENCODE_IBASE64:
        Padded=EncodingPad(Padded, Text, len, '=', 4);
        len=Radix64tobits(*Return, Padded, IBASE64_CHARS,'=');
        break;

    case ENCODE_PBASE64:
        Padded=EncodingPad(Padded, Text, len, '=', 4);
        len=Radix64tobits(*Return, Padded, PBASE64_CHARS,'=');
        break;

    case ENCODE_RBASE64:
        Padded=EncodingPad(Padded, Text, len, '=', 4);
        len=Radix64tobits(*Return, Padded, RBASE64_CHARS,'=');
        break;

    case ENCODE_CRYPT:
        Padded=EncodingPad(Padded, Text, len, '=', 4);
        len=Radix64tobits(*Return, Padded, CRYPT_CHARS,'=');
        break;

    case ENCODE_XXENC:
        Padded=EncodingPad(Padded, Text, len, '=', 4);
        len=Radix64tobits(*Return, Padded, XXENC_CHARS,'=');
        break;

    case ENCODE_UUENC:
        Padded=EncodingPad(Padded, Text, len, '|', 4);
        len=Radix64tobits(*Return, Padded, UUENC_CHARS,'|');
        break;

    case ENCODE_ASCII85:
        //RetStr=Ascii85(RetStr,Bytes,len,ASCII85_CHARS); break;
        break;

    case ENCODE_Z85:
        //RetStr=Ascii85(RetStr,Bytes,len,Z85_CHARS); break;
        break;

    case ENCODE_QUOTED_MIME:
        len=DecodeQuoted(Return,Text,'=');
        break;

    case ENCODE_YENCODE:
        len=DecodeYenc(Return,Text,'=');
        break;

    case ENCODE_QUOTED_HTTP:
        *Return=HTTPUnQuote(*Return, Text);
        len=StrLen(*Return);
        break;



    default:
        break;
    }

    return(len);
}



char *DecodeToText(char *RetStr, const char *Text, int Encoding)
{
    int len;

    len=DecodeBytes(&RetStr, Text, Encoding);
    StrUnsafeTrunc(RetStr, len);

    return(RetStr);
}
