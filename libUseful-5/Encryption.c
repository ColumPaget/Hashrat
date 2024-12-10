#include "Encryption.h"
#include "Encodings.h"
#include "Entropy.h"

//require both these libraries, as we shouldn't find libcrypto without libssl
#ifdef HAVE_LIBSSL
#ifdef HAVE_LIBCRYPTO

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

typedef struct
{
    char *Password;
    int PasswordLen;
    char *Key;
    int KeyLen;
    char *Salt;
    int SaltLen;
    char *InputVector;
    int InputVectorLen;
    int BlockSize;
    int KeyStretchIter;
    char *CipherName;
    EVP_CIPHER *Cipher;
    EVP_CIPHER_CTX *enc_ctx;
    EVP_CIPHER_CTX *dec_ctx;
} libCryptoProcessorData;



void libCryptoDataDestroy(libCryptoProcessorData *Data)
{
    Destroy(Data->Password);
    Destroy(Data->Key);
    Destroy(Data->Salt);
    Destroy(Data->InputVector);
    Destroy(Data->CipherName);

//if we have the EVP_CIPHER_FETCH function, then the cipher object
//will have been allocated, and needs to be freed. But if we don't have
//this function, then maybe the we don't have EVP_CIPHER_free either, and
//we certainly don't want to free the cipher object returned by getbyname,
//because it will be shared object
#ifdef HAVE_EVP_CIPHER_FETCH
    if (Data->Cipher) EVP_CIPHER_free(Data->Cipher);
#endif

    if (Data->enc_ctx) EVP_CIPHER_CTX_free(Data->enc_ctx);
    if (Data->dec_ctx) EVP_CIPHER_CTX_free(Data->dec_ctx);
    free(Data);
}


//generate openssl-compatible key and iv from salt and password
static int OpenSSLGenIVKey(EVP_CIPHER *Cipher, const char *Pass, int PassLen, const char *Salt, int SaltLen, int Iter, char **IV, int *IVLen, char **Key, int *KeyLen)
{
    char *PKOutput=NULL;
    int OutLen;

    *KeyLen=0;
    *IVLen=0;
    if (PassLen==0) return(FALSE);

    *KeyLen = EVP_CIPHER_key_length(Cipher);
    *IVLen = EVP_CIPHER_iv_length(Cipher);
    OutLen=*KeyLen + *IVLen;

    PKOutput=SetStrLen(PKOutput, OutLen);
    PKCS5_PBKDF2_HMAC(Pass, PassLen, Salt, SaltLen, Iter, EVP_sha256(), OutLen, PKOutput);

    *Key=SetStrLen(*Key, *KeyLen);
    memcpy(*Key, PKOutput, *KeyLen);
    *IV=SetStrLen(*IV, *IVLen);
    memcpy(*IV, PKOutput + *KeyLen, *IVLen);

    Destroy(PKOutput);

    return(TRUE);
}


static libCryptoProcessorData *InitialiseEncryptionComponents(const char *Args)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;
    libCryptoProcessorData *Data;

    Data=(libCryptoProcessorData *) calloc(1,sizeof(libCryptoProcessorData));
    Data->KeyStretchIter=10000; //openssl default iterations
    Data->CipherName=CopyStr(Data->CipherName, "aes-256-cbc"); //default cipher

    ptr=GetNameValuePair(Args,"\\S","=",&Name,&Value);
    while (ptr)
    {
        if (StrValid(Name))
        {
            if (strcasecmp(Name,"encrypt")==0)
            {
                Data->Password=CopyStr(Data->Password, Value);
                Data->PasswordLen=StrLen(Data->Password);
            }
            else if (strcasecmp(Name,"encrypt_key")==0)
            {
                Data->Key=CopyStr(Data->Key, Value);
                Data->KeyLen=StrLen(Data->Key);
            }
            else if (strcasecmp(Name,"encrypt_salt")==0)
            {
                Data->Salt=CopyStr(Data->Salt, Value);
                Data->SaltLen=StrLen(Data->Salt);
            }
            else if (strcasecmp(Name,"encrypt_iv")==0)
            {
                Data->InputVector=CopyStr(Data->InputVector, Value);
                Data->InputVectorLen=StrLen(Data->InputVector);
            }
            else if (strcasecmp(Name,"encrypt_hexpassword")==0)
            {
                Data->PasswordLen=DecodeBytes(&(Data->Password), Value, ENCODE_HEX);
            }
            else if (strcasecmp(Name,"encrypt_hexkey")==0)
            {
                Data->KeyLen=DecodeBytes(&(Data->Key), Value, ENCODE_HEX);
            }
            else if (strcasecmp(Name,"encrypt_hexsalt")==0)
            {
                Data->SaltLen=DecodeBytes(&(Data->Salt), Value, ENCODE_HEX);
            }
            else if (strcasecmp(Name,"encrypt_hexiv")==0)
            {
                Data->InputVectorLen=DecodeBytes(&(Data->InputVector), Value, ENCODE_HEX);
            }
            else if (strcasecmp(Name,"encrypt_iter")==0)
            {
                Data->KeyStretchIter=atoi(Value);
            }
            else if (strcasecmp(Name,"PadBlock")==0)
            {
                //   else if (strcasecmp(Value,"N")==0) *Flags |= DPM_NOPAD_DATA;
            }
            else if (strcasecmp(Name,"cipher")==0) Data->CipherName=CopyStr(Data->CipherName, Value);
            else if (strcasecmp(Name,"encrypt_cipher")==0) Data->CipherName=CopyStr(Data->CipherName, Value);

        }

        ptr=GetNameValuePair(ptr,"\\S","=",&Name,&Value);
    }

    if (Data->SaltLen == 0)
    {
        Data->Salt=SetStrLen(Data->Salt, 8);
        GenerateRandomBytes(&Data->Salt, 8, 0);
        Data->SaltLen=8;
    }


    DestroyString(Name);
    DestroyString(Value);

    return(Data);
}





static EVP_CIPHER *libCryptoGetCipher(const char *CipherList)
{
    EVP_CIPHER *cipher;
    char *Token=NULL, *CipherName=NULL;
    const char *ptr;

    ptr=GetToken(CipherList, ",", &Token, 0);
    while (ptr)
    {
        if (strcasecmp(Token, "aes")==0) CipherName=CopyStr(CipherName, "AES-128-CBC");
        else if (strcasecmp(Token, "aes-128")==0) CipherName=CopyStr(CipherName, "AES-128-CBC");
        else if (strcasecmp(Token, "aes-256")==0) CipherName=CopyStr(CipherName, "AES-256-CBC");
        else if (strcasecmp(Token, "blowfish")==0) CipherName=CopyStr(CipherName, "BF-CBC");
        else if (strcasecmp(Token, "bf")==0) CipherName=CopyStr(CipherName, "BF-CBC");
        else if (strcasecmp(Token, "camellia")==0) CipherName=CopyStr(CipherName, "CAMELLIA-128-CBC");
        else if (strcasecmp(Token, "camellia-256")==0) CipherName=CopyStr(CipherName, "CAMELLIA-256-CBC");
        else if (strcasecmp(Token, "camellia-256")==0) CipherName=CopyStr(CipherName, "CAMELLIA-256-CBC");
        else if (strcasecmp(Token, "cast")==0) CipherName=CopyStr(CipherName, "CAST5-CBC");
        else if (strcasecmp(Token, "des")==0) CipherName=CopyStr(CipherName, "DES-CBC");
        else if (strcasecmp(Token, "desx")==0) CipherName=CopyStr(CipherName, "DESX-CBC");
        else if (strcasecmp(Token, "rc2")==0) CipherName=CopyStr(CipherName, "RC2-CBC");
        else CipherName=CopyStr(CipherName, Token);

#ifdef HAVE_EVP_CIPHER_FETCH
        cipher=EVP_CIPHER_fetch(NULL, CipherName, NULL);
#else
        cipher=(EVP_CIPHER *) EVP_get_cipherbyname(CipherName);
#endif

        if (cipher != NULL) break;
        ptr=GetToken(ptr, ",", &Token, 0);
    }


    Destroy(CipherName);
    Destroy(Token);

    return(cipher);
}





int libCryptoProcessorInit(TProcessingModule *ProcMod, const char *Args, unsigned char **Header, int *HeadLen)
{
    int result=FALSE;

    libCryptoProcessorData *Data;
    EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *ctx;
    int val;
    char *Tempstr=NULL;

    Data=InitialiseEncryptionComponents(Args);
    cipher=libCryptoGetCipher(Data->CipherName);
    if (cipher)
    {
        OpenSSLGenIVKey(cipher, Data->Password, Data->PasswordLen, Data->Salt, Data->SaltLen, Data->KeyStretchIter, &Data->InputVector, &Data->InputVectorLen, &Data->Key, &Data->KeyLen);

        if (Data->KeyLen > 0)
        {
            Data->Cipher=cipher;
            Data->enc_ctx=EVP_CIPHER_CTX_new();
            Data->dec_ctx=EVP_CIPHER_CTX_new();
            EVP_CIPHER_CTX_init(Data->enc_ctx);
            EVP_CIPHER_CTX_init(Data->dec_ctx);
            Data->BlockSize=EVP_CIPHER_block_size(Data->Cipher);

            Tempstr=EncodeBytes(Tempstr, Data->Key, Data->KeyLen, ENCODE_HEX);
            Tempstr=EncodeBytes(Tempstr, Data->InputVector, Data->InputVectorLen, ENCODE_HEX);


            EVP_EncryptInit_ex(Data->enc_ctx, Data->Cipher, NULL, (unsigned char *) Data->Key, (unsigned char *) Data->InputVector);
            EVP_DecryptInit_ex(Data->dec_ctx, Data->Cipher, NULL, (unsigned char *) Data->Key, (unsigned char *) Data->InputVector);

            if (ProcMod->Flags & DPM_NOPAD_DATA) EVP_CIPHER_CTX_set_padding(Data->enc_ctx,FALSE);

            ProcMod->Data=Data;
            result=TRUE;

            Tempstr=FormatStr(Tempstr,"%d",Data->BlockSize);
            DataProcessorSetValue(ProcMod,"BlockSize",Tempstr);

            if (Header)
            {
                *Header=SetStrLen(*Header, 16);
                memcpy(*Header, "Salted__", 8);
                memcpy((*Header)+8, Data->Salt, 8);
                *HeadLen=16;
            }
        }
        else RaiseError(0, "libCryptoProcessorInit", "no key/password for encryption");
    }
    else RaiseError(0, "libCryptoProcessorInit", "Cipher '%s' not available", Data->CipherName);

    if (! result) libCryptoDataDestroy(Data);

    DestroyString(Tempstr);

    return(result);
}



int libCryptoProcessorWrite(TProcessingModule *ProcMod, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen, int Flush)
{
    int RetVal=0;

    int len=0, result=0, val;
    libCryptoProcessorData *Data;
    EVP_CIPHER_CTX *ctx;
    unsigned char *ptr, *Tempstr=NULL;

    if (ProcMod->Flags & DPM_WRITE_FINAL) return(0);

    Data=(libCryptoProcessorData *) ProcMod->Data;
    ctx=Data->enc_ctx;

    ProcMod->Flags = ProcMod->Flags & ~DPM_WRITE_FINAL;

    if (InLen)
    {
        ptr=(unsigned char *) *OutData;
        if (ProcMod->Flags & DPM_NOPAD_DATA)
        {
            val=InLen % Data->BlockSize;
            Tempstr=CopyStrLen(Tempstr,InData,InLen);
            if (val !=0)
            {
                Tempstr=SetStrLen(Tempstr,InLen + (Data->BlockSize-val));
                memset(Tempstr+InLen,' ', (Data->BlockSize-val));
                val=InLen+(Data->BlockSize-val);
            }
            else val=InLen;

            result=EVP_EncryptUpdate(ctx, ptr, &len, Tempstr, val);
            if (! result) RetVal=STREAM_DATA_ERROR;
            else RetVal=len;
        }
        else
        {
            result=EVP_EncryptUpdate(ctx, ptr, &len, InData, InLen);
            if (! result) RetVal=STREAM_DATA_ERROR;
            else RetVal=len;
        }
    }


    if (Flush)
    {
        ptr=(unsigned char *) *OutData;
        ptr+=len;
        result=EVP_EncryptFinal_ex(ctx, ptr, &val);
        len+=val;
        if (len==0) RetVal=STREAM_CLOSED;
    }

    *OutLen=len;

    DestroyString(Tempstr);

    return(RetVal);
}




int libCryptoProcessorRead(TProcessingModule *ProcMod, const char *InData, unsigned long InLen, char **OutData, unsigned long *OutLen, int Flush)
{
    int len=0;
    int result;
    libCryptoProcessorData *Data;
    EVP_CIPHER_CTX *ctx;
    char *ptr;

    ptr=*OutData;

    Data=(libCryptoProcessorData *) ProcMod->Data;
    if (!Data) return(STREAM_CLOSED);

    if (ProcMod->Flags & DPM_READ_FINAL)
    {
        if (InLen==0) return(STREAM_CLOSED);
        EVP_DecryptInit_ex(Data->dec_ctx,Data->Cipher,NULL,Data->Key,Data->InputVector);

    }

    ctx=Data->dec_ctx;

    if (InLen==0)
    {
        len=0;
        result=EVP_DecryptFinal_ex(ctx, ptr, &len);
        if (len==0) ProcMod->Flags |= DPM_READ_FINAL; //this so we don't try another read
    }
    else
    {
        len=*OutLen;
        result=EVP_DecryptUpdate(ctx, ptr, &len, InData, InLen);
    }


    if (! result) len=STREAM_CLOSED;

    return(len);
}



int libCryptoProcessorClose(TProcessingModule *ProcMod)
{
    libCryptoProcessorData *Data;
    EVP_CIPHER_CTX *ctx;

    Data=(libCryptoProcessorData *) ProcMod->Data;
    if (Data)
    {
        EVP_CIPHER_CTX_cleanup(Data->enc_ctx);
        EVP_CIPHER_CTX_cleanup(Data->dec_ctx);

        DestroyString(Data->Key);
        DestroyString(Data->InputVector);
        free(Data);
    }
    ProcMod->Data=NULL;
    return(TRUE);
}

#endif
#endif


//all that will exist without libcrypto is this function, which will return NULL
TProcessingModule *libCryptoProcessorCreate()
{
    TProcessingModule *Mod=NULL;


#ifdef HAVE_LIBSSL
#ifdef HAVE_LIBCRYPTO
    Mod=(TProcessingModule *) calloc(1, sizeof(TProcessingModule));
    Mod->Init=libCryptoProcessorInit;
    Mod->Write=libCryptoProcessorWrite;
    Mod->Read=libCryptoProcessorRead;
    Mod->Close=libCryptoProcessorClose;
#endif
#else
    RaiseError(0, "libCryptoProcessorCreate", "libSSL/libcrypto not compiled in");
#endif

    return(Mod);
}
