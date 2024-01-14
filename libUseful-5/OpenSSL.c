#include "includes.h"
#include "Tokenizer.h"
#include "URL.h"
#include "Time.h"
#include "Encodings.h"
#include "Entropy.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef HAVE_LIBSSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/asn1.h>

static DH *CachedDH=NULL;
#endif


#define LEVEL_SSL3    3
#define LEVEL_TLS1   10
#define LEVEL_TLS1_1 11
#define LEVEL_TLS1_2 12
#define LEVEL_TLS1_3 13




//static, internal functions that only exist if we have lib SSL
#ifdef HAVE_LIBSSL


static void STREAM_INTERNAL_SSL_ADD_SECURE_KEYS_LIST(STREAM *S, SSL_CTX *ctx, ListNode *List, char **VerifyPath, char **VerifyFile)
{
    ListNode *Curr;

    Curr=ListGetNext(List);
    while (Curr)
    {
        if (StrValid(Curr->Tag))
        {
            if (strcasecmp(Curr->Tag,"SSL:CertFile")==0) SSL_CTX_use_certificate_chain_file(ctx,(char *) Curr->Item);
            else if (strcasecmp(Curr->Tag,"SSL:KeyFile")==0) SSL_CTX_use_PrivateKey_file(ctx,(char *) Curr->Item,SSL_FILETYPE_PEM);
            else if (strncasecmp(Curr->Tag,"SSL:VerifyCertDir",18)==0) *VerifyPath=CopyStr(*VerifyPath,(char *) Curr->Item);
            else if (strncasecmp(Curr->Tag,"SSL:VerifyCertFile",19)==0) *VerifyFile=CopyStr(*VerifyFile,(char *) Curr->Item);
            else if (strncasecmp(Curr->Tag,"SSL:VerifyDir",18)==0) *VerifyPath=CopyStr(*VerifyPath,(char *) Curr->Item);
            else if (strncasecmp(Curr->Tag,"SSL:VerifyFile",19)==0) *VerifyFile=CopyStr(*VerifyFile,(char *) Curr->Item);
        }

        Curr=ListGetNext(Curr);
    }
}


static void STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(STREAM *S, SSL_CTX *ctx)
{
    ListNode *Curr;
    char *VerifyFile=NULL, *VerifyPath=NULL;

    SSL_CTX_set_default_verify_paths(ctx);

//Default Verify path
//VerifyFile=CopyStr(VerifyFile,"/etc/ssl/certs/cacert.pem");

    Curr=ListGetNext(LibUsefulValuesGetHead());

    STREAM_INTERNAL_SSL_ADD_SECURE_KEYS_LIST(S, ctx, LibUsefulValuesGetHead(), &VerifyPath, &VerifyFile);
    STREAM_INTERNAL_SSL_ADD_SECURE_KEYS_LIST(S, ctx, S->Values, &VerifyPath, &VerifyFile);

    SSL_CTX_load_verify_locations(ctx,VerifyFile,VerifyPath);

    DestroyString(VerifyFile);
    DestroyString(VerifyPath);

}

static int OpenSSLVerifyCallback(int PreverifyStatus, X509_STORE_CTX *X509)
{
//This does nothing. verification is done in 'OpenSSLVerifyCertificate' instead
    return(1);
}



static char *OpenSSLConvertTime(char *RetStr, const ASN1_TIME *Time)
{
    int result;
    BIO *b;

    RetStr=SetStrLen(RetStr,80);
    b = BIO_new(BIO_s_mem());
    result=ASN1_TIME_print(b, Time);
    if (result > 0) result = BIO_gets(b, RetStr, 80);
    if (result > 0) RetStr[result]='\0';
    BIO_free(b);

    return(RetStr);
}


static void OpenSSLCertError(STREAM *S, const char *Error)
{
    STREAMSetValue(S,"SSL:CertificateVerify",Error);
    RaiseError(0, "SSL:CertificateVerify", Error);
}



static char *OpenSSLGetCertFingerprint(char *RetStr, X509 *cert)
{
    const EVP_MD *digest;
    char *Buffer=NULL;
    unsigned len;

    RetStr=CopyStr(RetStr, "");
    digest = EVP_sha1();
    Buffer=SetStrLen(Buffer, 255); //this will hold raw sha1 fingerprint
    len=255;
    if (X509_digest(cert, digest, (unsigned char*) Buffer, &len) > 0)
    {
        RetStr=EncodeBytes(RetStr, Buffer, len, ENCODE_HEX);
    }

    Destroy(Buffer);

    return(RetStr);
}


char *OpenSSLCertDetailsGetCommonName(char *RetStr, const char *CertDetails)
{
    char *Name=NULL, *Value=NULL;
    const char *ptr;

    RetStr=CopyStr(RetStr, "");
    ptr=GetNameValuePair(CertDetails, "/", "=", &Name, &Value);
    while (ptr)
    {
        if (StrValid(Name) && (strcmp(Name, "CN")==0)) RetStr=CopyStr(RetStr, Value);
        ptr=GetNameValuePair(ptr, "/", "=", &Name, &Value);
    }

    Destroy(Name);
    Destroy(Value);

    return(RetStr);
}



int OpenSSLVerifyCertificate(STREAM *S, int Flags)
{
    int RetVal=FALSE;
    char *Value=NULL;
    const char *ptr;
    int val;
    X509 *cert=NULL;
    SSL *ssl;

    ptr=STREAMGetItem(S,"LIBUSEFUL-SSL:OBJ");
    if (! ptr) return(FALSE);

    ssl=(SSL *) ptr;

    cert=SSL_get_peer_certificate(ssl);
    if (cert)
    {
        STREAMSetValue(S,"SSL:CertificateIssuer",X509_NAME_oneline( X509_get_issuer_name(cert),NULL, 0));
        ptr=X509_NAME_oneline( X509_get_subject_name(cert),NULL, 0);
        STREAMSetValue(S,"SSL:CertificateSubject", ptr);
        Value=OpenSSLCertDetailsGetCommonName(Value, ptr);
        if (StrValid(Value)) STREAMSetValue(S, "SSL:CertificateCommonName", Value);

        Value=OpenSSLConvertTime(Value, X509_get_notBefore(cert));
        STREAMSetValue(S,"SSL:CertificateNotBefore", Value);
        Value=OpenSSLConvertTime(Value, X509_get_notAfter(cert));
        STREAMSetValue(S,"SSL:CertificateNotAfter", Value);
        Value=OpenSSLGetCertFingerprint(Value, cert);
        STREAMSetValue(S,"SSL:CertificateFingerprint", Value);

#ifdef HAVE_X509_CHECK_HOST
        if (Flags & LU_SSL_VERIFY_HOSTNAME)
        {
            if (StrValid(S->Path))
            {
                ParseURL(S->Path,NULL,&Value,NULL,NULL,NULL,NULL,NULL);
                val=X509_check_host(cert, Value, StrLen(Value), 0, NULL);
            }
            else val=0;
        }
        else val=1;

        if (val!=1)	OpenSSLCertError(S, "Certificate hostname missmatch");
        else
#endif
        {
            val=SSL_get_verify_result(ssl);

            switch(val)
            {
            case X509_V_OK:
                STREAMSetValue(S,"SSL:CertificateVerify","OK");
                RetVal=TRUE;
                break;
            case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
                OpenSSLCertError(S,"unable to get issuer");
                break;
            case X509_V_ERR_UNABLE_TO_GET_CRL:
                OpenSSLCertError(S,"unable to get certificate CRL");
                break;
            case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
                OpenSSLCertError(S,"unable to decrypt certificate's signature");
                break;
            case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
                OpenSSLCertError(S,"unable to decrypt CRL's signature");
                break;
            case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
                OpenSSLCertError(S,"unable to decode issuer public key");
                break;
            case X509_V_ERR_CERT_SIGNATURE_FAILURE:
                OpenSSLCertError(S,"certificate signature invalid");
                break;
            case X509_V_ERR_CRL_SIGNATURE_FAILURE:
                OpenSSLCertError(S,"CRL signature invalid");
                break;
            case X509_V_ERR_CERT_NOT_YET_VALID:
                OpenSSLCertError(S,"certificate is not yet valid");
                break;
            case X509_V_ERR_CERT_HAS_EXPIRED:
                OpenSSLCertError(S,"certificate has expired");
                break;
            case X509_V_ERR_CRL_NOT_YET_VALID:
                OpenSSLCertError(S,"CRL is not yet valid");
                break;
            case X509_V_ERR_CRL_HAS_EXPIRED:
                OpenSSLCertError(S,"CRL has expired");
                break;
            case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
                OpenSSLCertError(S,"invalid notBefore value");
                break;
            case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
                OpenSSLCertError(S,"invalid notAfter value");
                break;
            case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
                OpenSSLCertError(S,"invalid CRL lastUpdate value");
                break;
            case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
                OpenSSLCertError(S,"invalid CRL nextUpdate value");
                break;
            case X509_V_ERR_OUT_OF_MEM:
                OpenSSLCertError(S,"out of memory");
                break;
            case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
                OpenSSLCertError(S,"self signed certificate");
                break;
            case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
                OpenSSLCertError(S,"self signed certificate in certificate chain");
                break;
            case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
                OpenSSLCertError(S,"cant find root certificate in local database");
                break;
            case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
                OpenSSLCertError(S,"ERROR: unable to verify the first certificate");
                break;
            case X509_V_ERR_CERT_CHAIN_TOO_LONG:
                OpenSSLCertError(S,"certificate chain too long");
                break;
            case X509_V_ERR_CERT_REVOKED:
                OpenSSLCertError(S,"certificate revoked");
                break;
            case X509_V_ERR_INVALID_CA:
                OpenSSLCertError(S,"invalid CA certificate");
                break;
            case X509_V_ERR_PATH_LENGTH_EXCEEDED:
                OpenSSLCertError(S,"path length constraint exceeded");
                break;
            case X509_V_ERR_INVALID_PURPOSE:
                OpenSSLCertError(S,"unsupported certificate purpose");
                break;
            case X509_V_ERR_CERT_UNTRUSTED:
                OpenSSLCertError(S,"certificate not trusted");
                break;
            case X509_V_ERR_CERT_REJECTED:
                OpenSSLCertError(S,"certificate rejected");
                break;
            case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
                OpenSSLCertError(S,"subject issuer mismatch");
                break;
            case X509_V_ERR_AKID_SKID_MISMATCH:
                OpenSSLCertError(S,"authority and subject key identifier mismatch");
                break;
            case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
                OpenSSLCertError(S,"authority and issuer serial number mismatch");
                break;
            case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
                OpenSSLCertError(S,"key usage does not include certificate signing");
                break;
            case X509_V_ERR_APPLICATION_VERIFICATION:
                OpenSSLCertError(S,"application verification failure");
                break;

            default:
                OpenSSLCertError(S,"unknown error");
                break;

            }
        }
    }
    else OpenSSLCertError(S,"peer provided no certificate");


    DestroyString(Value);


    return(RetVal);
}


// Level is set against a stream as SSL:Level and allows chosing between the following levels:
// ssl - allow SSLv3 and up
// tls - allow all TLS types
// tls1.1 - allow TLSv1.1 and up
// tls1.2 - allow TLSv.12 and up
// tls1.3 - allow TLSv.13 and up
// default. Currently equivalent to tls but may change in future

static int OpenSSLSetOptions(STREAM *S, SSL *ssl, int Options)
{
    const char *ptr;
    int level=LEVEL_SSL3, val;


    //set Permitted ciphers
    ptr=STREAMGetValue(S, "SSL:PermittedCiphers");
    if (! StrValid(ptr)) ptr=LibUsefulGetValue("SSL:PermittedCiphers");
    if (StrValid(ptr))
    {
        SSL_set_cipher_list(ssl, ptr);

#ifdef SSL_set_ciphersuites
        SSL_set_ciphersuites(ssl, ptr);
#endif
    }

    //first convert things to our own enum values, that way we don't
    //depend on any openssl #defines that may be missing in some versions
    //as the SSL_set_min_proto_version function is a new function with
    //new values that deprecates the old method that used SSL_OP_NO_SSL options
    ptr=STREAMGetValue(S,"SSL:Level");
    if (! StrValid(ptr)) ptr=LibUsefulGetValue("SSL:Level");
    if (StrValid(ptr))
    {
        if (strcasecmp(ptr, "ssl") !=0) level=LEVEL_TLS1;

        if (strcasecmp(ptr, "tls1.1") == 0) level=LEVEL_TLS1_1;
        if (strcasecmp(ptr, "tls1.2") == 0) level=LEVEL_TLS1_2;
        if (strcasecmp(ptr, "tls1.3") == 0) level=LEVEL_TLS1_3;
    }

#ifdef HAVE_SSL_SET_MIN_PROTO_VERSION
    val=SSL3_VERSION;
    switch (level)
    {
    case LEVEL_TLS1:
        val=TLS1_VERSION;
        break;
    case LEVEL_TLS1_1:
        val=TLS1_1_VERSION;
        break;
    case LEVEL_TLS1_2:
        val=TLS1_2_VERSION;
        break;

#ifdef TLS1_3_VERSION
    case LEVEL_TLS1_3:
        val=TLS1_3_VERSION;
        break;
#endif
    }
    SSL_set_min_proto_version(ssl, val);
#else
    //anything before SSLv3 is madness
    Options |= SSL_OP_NO_SSLv2;

    switch (level)
    {
    case LEVEL_TLS1:
        Options |= SSL_OP_NO_SSLv3;
        break;
    case LEVEL_TLS1_1:

        Options |= SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
        break;
    case LEVEL_TLS1_2:
        Options |= SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
        break;
    }
#endif


    SSL_set_options(ssl, Options);
    return(Options);
}



static void OpenSSLSetupECDH(SSL_CTX *ctx)
{
    EC_KEY* ecdh;

    ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
//ecdh = EC_KEY_new_by_curve_name( NID_secp384r1);

    if (ecdh)
    {
        SSL_CTX_set_tmp_ecdh(ctx, ecdh);
        EC_KEY_free(ecdh);
    }

}


static void OpenSSLReseedRandom()
{
    int len=32;
    char *Tempstr=NULL;


    len=GenerateRandomBytes(&Tempstr, len, ENCODE_NONE);
    RAND_seed(Tempstr,len);
    memset(Tempstr,0,len); //extra paranoid step, don't keep those bytes in memory!

    DestroyString(Tempstr);
}







static void OpenSSLSetupDH(SSL_CTX *ctx)
{
    char *Tempstr=NULL;
    const char *ptr;
    DH *dh=NULL;
    FILE *paramfile;

    if (CachedDH) dh=CachedDH;
    else
    {
        ptr=LibUsefulGetValue("SSL:DHParams-File");
        if (StrValid(ptr))
        {
            Tempstr=CopyStr(Tempstr,ptr);

            paramfile = fopen(Tempstr, "r");
            if (paramfile)
            {
                CachedDH = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
                dh=CachedDH;
                fclose(paramfile);
            }
        }
    }

    if (dh) SSL_CTX_set_tmp_dh(ctx, dh);

//Don't free these parameters, as they are cached
//DH_KEY_free(dh);

    DestroyString(Tempstr);
}




static int INTERNAL_SSL_INIT()
{
    char *Tempstr=NULL;
    static int InitDone=FALSE;

//Always reseed RAND on a new connection
//OpenSSLReseedRandom();

    if (InitDone) return(TRUE);

    SSL_library_init();
#ifdef HAVE_OPENSSL_ADD_ALL_ALGORITHMS
    OpenSSL_add_all_algorithms();
#endif
    SSL_load_error_strings();
    Tempstr=MCopyStr(Tempstr,"openssl:",SSLeay_version(SSLEAY_VERSION)," : ", SSLeay_version(SSLEAY_BUILT_ON), " : ",SSLeay_version(SSLEAY_CFLAGS),NULL);
    LibUsefulSetValue("SSL:Library", Tempstr);
    if (! StrValid(LibUsefulGetValue("SSL:Level"))) LibUsefulSetValue("SSL:Level", "tls");
    DestroyString(Tempstr);
    InitDone=TRUE;
    return(TRUE);
}



#endif
//end of static functions that only exist if we have libssl



//everything after this exists even if we don't have libssl, and returns a fail value if called





int SSLAvailable()
{
#ifdef HAVE_LIBSSL
    return(INTERNAL_SSL_INIT());
#else
    return(FALSE);
#endif

}

const char *OpenSSLQueryCipher(STREAM *S)
{
    void *ptr;

    if (! S) return(NULL);
    ptr=STREAMGetItem(S,"LIBUSEFUL-SSL:OBJ");
    if (! ptr) return(NULL);

#ifdef HAVE_LIBSSL
    const SSL_CIPHER *Cipher;
    char *Tempstr=NULL;

    Cipher=SSL_get_current_cipher((const SSL *) ptr);
    if (Cipher)
    {
        Tempstr=FormatStr(Tempstr,"%d",SSL_CIPHER_get_bits(Cipher,NULL));
        STREAMSetValue(S,"SSL:CipherBits",Tempstr);
        Tempstr=CopyStr(Tempstr,SSL_CIPHER_get_name(Cipher));
        STREAMSetValue(S,"SSL:Cipher",Tempstr);

        Tempstr=SetStrLen(Tempstr,1024);
        memset(Tempstr, 0, 1024);
        SSL_CIPHER_description(Cipher, Tempstr, 1024);
        //openssl won't have updated our strlen cache, so we have to here
        if (Tempstr)
        {
            StrLenCacheAdd(Tempstr, strlen(Tempstr));
            StripTrailingWhitespace(Tempstr);
        }

        Tempstr=MCatStr(Tempstr, " ", SSL_CIPHER_get_version(Cipher), NULL);
        strrep(Tempstr, '\n', ' ');
        //openssl won't have updated our strlen cache, so we have to here
        if (Tempstr) StrLenCacheAdd(Tempstr, strlen(Tempstr));
        StripTrailingWhitespace(Tempstr);
        STREAMSetValue(S,"SSL:CipherDetails",Tempstr);
    }

    DestroyString(Tempstr);
    return(STREAMGetValue(S,"SSL:Cipher"));

#else
    return(NULL);
#endif
}



//this can be externally called to generate new, randomized DHParams
//for a server using PFS. One day this probably needs to be able to
//save the resulting params
void OpenSSLGenerateDHParams()
{
#ifdef HAVE_LIBSSL
    CachedDH = DH_new();
    if(CachedDH)
    {
        OpenSSLReseedRandom();
        DH_generate_parameters_ex(CachedDH, 512, DH_GENERATOR_5, 0);
        //DH_check(CachedDH, &codes);
    }
#endif
}




int DoSSLClientNegotiation(STREAM *S, int Flags)
{
    int result=FALSE, i, val;
    char *Token=NULL;
#ifdef HAVE_LIBSSL
    const SSL_METHOD *Method;
    SSL_CTX *ctx;
    SSL *ssl;
//struct x509 *cert=NULL;
    const char *ptr;

    if (S)
    {
        INTERNAL_SSL_INIT();
        //  SSL_load_ciphers();
        Method=SSLv23_client_method();
        ctx=SSL_CTX_new(Method);
        if (! ctx) RaiseError(0, "Failed to create SSL_CTX: %s\n",ERR_error_string(ERR_get_error(), NULL));
        else
        {
            STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(S,ctx);

            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, OpenSSLVerifyCallback);
            ssl=SSL_new(ctx);
            SSL_set_fd(ssl,S->in_fd);
            STREAMSetItem(S,"LIBUSEFUL-SSL:CTX",(void *) ctx);
            STREAMSetItem(S,"LIBUSEFUL-SSL:OBJ",(void *) ssl);
            OpenSSLSetOptions(S, ssl, SSL_OP_SINGLE_DH_USE);

#ifdef HAVE_SSL_SET_TLSEXT_HOST_NAME
            //extract hostname from 'tcp://Host:Port' path
            ptr=GetToken(S->Path,":",&Token,0);
            if (ptr)
            {
                while (*ptr=='/') ptr++;
                ptr=GetToken(ptr,":",&Token,0);
                SSL_set_tlsext_host_name(ssl, Token);
            }
#endif
            /*
            if (S->Timeout > 0)
            {
            		//convert centisecs to seconds
            		val=S->Timeout / 100;
            		if (val==0) val++;
            		SSL_CTX_set_timeout (ctx, val);
            }
            */

            result=SSL_connect(ssl);
            for (i=0; i < 3; i ++)
            {
                //if we succeeded don't keep looping
                if (result > -1) break;
                result=SSL_get_error(ssl, result);
                if ( (result != SSL_ERROR_WANT_READ) && (result != SSL_ERROR_WANT_WRITE) && (result != SSL_ERROR_WANT_CONNECT)) break;
                usleep(2000);
                result=SSL_connect(ssl);
            }

            result=SSL_do_handshake(ssl);

            S->State |= SS_SSL;

            OpenSSLQueryCipher(S);
            OpenSSLVerifyCertificate(S, LU_SSL_VERIFY_HOSTNAME);

        }
    }

#else
    RaiseError(0, "DoSSLClientNegotiation", "ssl support not compiled into libUseful");
#endif

    DestroyString(Token);

    return(result);
}




int DoSSLServerNegotiation(STREAM *S, int Flags)
{
    int result=FALSE;
#ifdef HAVE_LIBSSL
    const SSL_METHOD *Method;
    SSL_CTX *ctx;
    SSL *ssl;
    const char *ptr;


    if (S)
    {
        INTERNAL_SSL_INIT();
        Method=SSLv23_server_method();
        if (Method)
        {
            ctx=SSL_CTX_new(Method);

            if (ctx)
            {
                STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(S,ctx);
                if (Flags & LU_SSL_PFS)
                {
                    OpenSSLSetupDH(ctx);
                    OpenSSLSetupECDH(ctx);
                }
                if (Flags & LU_SSL_VERIFY_PEER)
                {
                    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, OpenSSLVerifyCallback);
                    SSL_CTX_set_verify_depth(ctx,1);
                }
                SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
                ssl=SSL_new(ctx);
                OpenSSLSetOptions(S, ssl, SSL_OP_SINGLE_DH_USE|SSL_OP_CIPHER_SERVER_PREFERENCE);

                SSL_set_fd(ssl,S->in_fd);

                STREAMSetItem(S,"LIBUSEFUL-SSL:CTX",(void *) ctx);
                STREAMSetItem(S,"LIBUSEFUL-SSL:OBJ",(void *) ssl);

                SSL_set_accept_state(ssl);

                while (1)
                {
                    result=SSL_accept(ssl);
                    if (result != TRUE) result=SSL_get_error(ssl,result);

                    switch (result)
                    {
                    case SSL_ERROR_WANT_READ:
                    case SSL_ERROR_WANT_WRITE:
                        usleep(300);
                        result=-1;
                        break;

                    case TRUE:
                        S->State |= SS_SSL;
                        if (Flags & LU_SSL_VERIFY_PEER) OpenSSLVerifyCertificate(S, 0);
                        OpenSSLQueryCipher(S);
                        break;

                    default:
                        result=ERR_get_error();
                        STREAMSetValue(S, "SSL:Error", ERR_error_string(result,NULL));
                        result=FALSE;
                        break;
                    }
                    if (result !=-1) break;
                }
            }
        }
    }

#else
    RaiseError(0, "DoSSLServerNegotiation", "ssl support not compiled into libUseful");
#endif
    return(result);
}



int OpenSSLIsPeerAuth(STREAM *S)
{
#ifdef HAVE_LIBSSL
    SSL *ssl;

    ssl=(SSL *) STREAMGetItem(S,"LIBUSEFUL-SSL:OBJ");
    if (! ssl) return(FALSE);

    if (SSL_get_verify_result(ssl)==X509_V_OK)
    {
        if (SSL_get_peer_certificate(ssl) !=NULL) return(TRUE);
    }
#endif
    return(FALSE);
}


int OpenSSLSTREAMCheckForBytes(STREAM *S)
{
    int byte_count=0;
#ifdef HAVE_LIBSSL
    SSL *SSL_OBJ;

//This is used in multiple places below, do don't just move it to within the first place
    SSL_OBJ=(SSL *) STREAMGetItem(S,"LIBUSEFUL-SSL:OBJ");

//if there are bytes available in the internal OpenSSL buffers, when we don't have to
//wait on a select, we can just go straight through to SSL_read
    if (S->State & SS_SSL)
    {
        //ssl pending checks if there's bytes in the SSL buffer, it's not a select
        byte_count=SSL_pending(SSL_OBJ);
    }
#endif

    return(byte_count);
}


int OpenSSLSTREAMReadBytes(STREAM *S, char *Data, int len)
{
    int bytes_read=0, val;
#ifdef HAVE_LIBSSL
    SSL *SSL_OBJ;


    SSL_OBJ=(SSL *) STREAMGetItem(S,"LIBUSEFUL-SSL:OBJ");
    if (S->State & SS_SSL)
    {
        bytes_read=SSL_read(SSL_OBJ, Data, len);
        //  saved_errno is used in all cases to capture errno before another function changes it
        //  saved_errno=errno;
        //zero or less indicates some kind of error. Could be we are waiting for or bytes, or any number of
        //real errors that count as disconnection

        if (bytes_read < 1)
        {
            //turns out you get hangs here if you treat SSL_ERROR_WANT_READ as being 'wait for more bytes'
            //I think that fact we've used SSL_Pending means that we should always get bytes read here
            //and if we don't, the connection is effectively closed
            bytes_read=-1;

	/*
            val=SSL_get_error(SSL_OBJ, bytes_read);
            switch (val)
            {
            //these all mean SSL is waiting for more data, and has nothing to offer us right now
            case SSL_ERROR_WANT_READ:
            	bytes_read=0;
             break;

            //for anything else consider the connection closed
            default:
            	bytes_read=-1;
            break;
            }
	 */
        }
    }
#endif

    return(bytes_read);
}

int OpenSSLSTREAMWriteBytes(STREAM *S, const char *Data, int Len)
{
    int result=STREAM_CLOSED;

#ifdef HAVE_LIBSSL
    SSL *ssl;

    //if this is an SSL stream, it should have an associated SSL object. If it doesn't then the stream
    //either failed to open, or has been closed with STREAMShutdown
    ssl=(SSL *) STREAMGetItem(S,"LIBUSEFUL-SSL:OBJ");
    if (ssl)
    {
        result=SSL_write(ssl, Data, Len);
        if (result < 1) result=STREAM_CLOSED;
    }
#endif

    return(result);
}


void OpenSSLClose(STREAM *S)
{
    ListNode *Node;

#ifdef HAVE_LIBSSL
    Node=ListFindNamedItem(S->Items,"LIBUSEFUL-SSL:OBJ");
    if (Node)
    {
        SSL_free((SSL *) Node->Item);
        ListDeleteNode(Node);
    }

    Node=ListFindNamedItem(S->Items,"LIBUSEFUL-SSL:CTX");
    if (Node)
    {
        SSL_CTX_free((SSL_CTX *) Node->Item);
        ListDeleteNode(Node);
    }
#endif

    S->State &= ~SS_SSL;
}


//Autodetect if a stream is TLS/SSL
int OpenSSLAutoDetect(STREAM *S)
{
    int result, val, RetVal=FALSE;
    char *Tempstr=NULL;

    val=S->Timeout;
    STREAMSetTimeout(S, 5);

//we must not read any bytes into our stream, or else e'll be stealing them
//from ssl, which only has access to the file descriptor
    result=FDSelect(S->in_fd, SELECT_READ, NULL);
    if (result > 0)
    {
        Tempstr=SetStrLen(Tempstr,255);
        //cannot use STREAMPeek here, as bytes must stay in file descriptor for ssl
        result=recv(S->in_fd, Tempstr, 2, MSG_PEEK);
        if (result > 1)
        {
            if (memcmp(Tempstr, "\x16\x03",2)==0)
            {
                //it's SSL/TLS
                RetVal=TRUE;
            }
        }
    }
    STREAMSetTimeout(S, val);

    Destroy(Tempstr);

    return(RetVal);
}


