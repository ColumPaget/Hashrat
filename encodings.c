#include "encodings.h"

const char *EncodingNames[]= {"octal","dec","hex","uhex","base64","ibase64","pbase64","xxencode","uuencode","crypt","ascii85","z85",NULL};
const char *EncodingDescriptions[]= {"Octal","Decimal","Hexadecimal","Uppercase Hexadecimal","Base64","IBase64","PBase64","XXEncode","UUEncode","Crypt","ASCII85","Z85",NULL};
int Encodings[]= {ENCODE_OCTAL, ENCODE_DECIMAL, ENCODE_HEX, ENCODE_HEXUPPER, ENCODE_BASE64, ENCODE_IBASE64, ENCODE_PBASE64, ENCODE_XXENC, ENCODE_UUENC, ENCODE_CRYPT, ENCODE_ASCII85, ENCODE_Z85, -1};


const char *EncodingNameFromID(int id)
{
    int i;

    for (i=0; Encodings[i] != -1; i++)
    {
        if (Encodings[i]==id) return(EncodingNames[i]);
    }

    return("");
}


const char *EncodingDescriptionFromID(int id)
{
    int i;

    for (i=0; Encodings[i] != -1; i++)
    {
        if (Encodings[i]==id) return(EncodingDescriptions[i]);
    }

    return("");
}
