#include "includes.h"
#include "Tokenizer.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifdef HAVE_LIBSSL
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

DH *CachedDH=NULL;



void OpenSSLReseedRandom()
{
int len=32;
char *Tempstr=NULL;


len=GenerateRandomBytes(&Tempstr, len, ENCODE_NONE);
RAND_seed(Tempstr,len);
memset(Tempstr,0,len); //extra paranoid step, don't keep those bytes in memory!

DestroyString(Tempstr);
}



void OpenSSLGenerateDHParams()
{
	CachedDH = DH_new();
	if(CachedDH)
	{
		OpenSSLReseedRandom();
		DH_generate_parameters_ex(CachedDH, 512, DH_GENERATOR_5, 0); 
		//DH_check(CachedDH, &codes); 
	}
}








void STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(STREAM *S, SSL_CTX *ctx)
{
ListNode *Curr;
char *VerifyFile=NULL, *VerifyPath=NULL;

Curr=ListGetNext(LibUsefulValuesGetHead());
while (Curr)
{
  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_CERT_FILE:",14)==0))
  {
	  SSL_CTX_use_certificate_chain_file(ctx,(char *) Curr->Item);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_KEY_FILE:",13)==0))
  {
	  SSL_CTX_use_PrivateKey_file(ctx,(char *) Curr->Item,SSL_FILETYPE_PEM);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTDIR",18)==0))
  {
	  VerifyPath=CopyStr(VerifyPath,(char *) Curr->Item);
  }

	if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTFILE",19)==0))
	{
	  VerifyFile=CopyStr(VerifyFile,(char *) Curr->Item);
	}

  Curr=ListGetNext(Curr);
}


Curr=ListGetNext(S->Values);
while (Curr)
{
  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_CERT_FILE:",14)==0))
  {
	  SSL_CTX_use_certificate_chain_file(ctx,(char *) Curr->Item);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_KEY_FILE:",13)==0))
  {
	  SSL_CTX_use_PrivateKey_file(ctx,(char *) Curr->Item,SSL_FILETYPE_PEM);
  }

  if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTDIR",18)==0))
  {
	  VerifyPath=CopyStr(VerifyPath,(char *) Curr->Item);
  }

	if ((StrLen(Curr->Tag)) && (strncasecmp(Curr->Tag,"SSL_VERIFY_CERTFILE",19)==0))
	{
	  VerifyFile=CopyStr(VerifyFile,(char *) Curr->Item);
	}


  Curr=ListGetNext(Curr);
}


SSL_CTX_load_verify_locations(ctx,VerifyFile,VerifyPath);

DestroyString(VerifyFile);
DestroyString(VerifyPath);

}
#endif


void HandleSSLError()
{
#ifdef HAVE_LIBSSL
int val;

	  val=ERR_get_error();
	  fprintf(stderr,"Failed to create SSL_CTX: %s\n",ERR_error_string(val,NULL));
	  fflush(NULL);
#endif
}


int INTERNAL_SSL_INIT()
{
#ifdef HAVE_LIBSSL
char *Tempstr=NULL;
static int InitDone=FALSE;

//Always reseed RAND on a new connection
//OpenSSLReseedRandom();

if (InitDone) return(TRUE);

  SSL_library_init();
#ifdef USE_OPENSSL_ADD_ALL_ALGORITHMS
  OpenSSL_add_all_algorithms();
#endif
  SSL_load_error_strings();
	Tempstr=MCopyStr(Tempstr,SSLeay_version(SSLEAY_VERSION)," : ", SSLeay_version(SSLEAY_BUILT_ON), " : ",SSLeay_version(SSLEAY_CFLAGS),NULL);
	LibUsefulSetValue("SSL-Library", Tempstr);
	LibUsefulSetValue("SSL-Level", "tls");
	DestroyString(Tempstr);
  InitDone=TRUE;
  return(TRUE);
#endif

return(FALSE);
}


int SSLAvailable()
{
	return(INTERNAL_SSL_INIT());
}

const char *OpenSSLQueryCipher(STREAM *S)
{
void *ptr;

if (! S) return(NULL);
ptr=STREAMGetItem(S,"LIBUSEFUL-SSL-CTX");
if (! ptr) return(NULL);

#ifdef HAVE_LIBSSL
const SSL_CIPHER *Cipher;
char *Tempstr=NULL;

Cipher=SSL_get_current_cipher((const SSL *) ptr);

if (Cipher)
{
Tempstr=FormatStr(Tempstr,"%d",SSL_CIPHER_get_bits(Cipher,NULL));
STREAMSetValue(S,"SSL-Bits",Tempstr);
Tempstr=FormatStr(Tempstr,"%s",SSL_CIPHER_get_name(Cipher));
STREAMSetValue(S,"SSL-Cipher",Tempstr);

Tempstr=SetStrLen(Tempstr,1024);
Tempstr=SSL_CIPHER_description(Cipher, Tempstr, 1024);


STREAMSetValue(S,"SSL-Cipher-Details",Tempstr);
}

DestroyString(Tempstr);
return(STREAMGetValue(S,"SSL-Cipher"));

#else
return(NULL);
#endif
}


#ifdef HAVE_LIBSSL
int OpenSSLVerifyCallback(int PreverifyStatus, X509_STORE_CTX *X509)
{
//This does nothing. verification is done in 'OpenSSLVerifyCertificate' instead
return(1);
}
#endif

int OpenSSLVerifyCertificate(STREAM *S)
{
int RetVal=FALSE;
#ifdef HAVE_LIBSSL
char *Name=NULL, *Value=NULL, *ptr;
int val;
X509 *cert=NULL;
SSL *ssl;

ptr=STREAMGetItem(S,"LIBUSEFUL-SSL-CTX");
if (! ptr) return(FALSE);

ssl=(SSL *) ptr;

cert=SSL_get_peer_certificate(ssl);
if (cert)
{
	STREAMSetValue(S,"SSL-Certificate-Issuer",X509_NAME_oneline( X509_get_issuer_name(cert),NULL, 0));
	ptr=X509_NAME_oneline( X509_get_subject_name(cert),NULL, 0);
	STREAMSetValue(S,"SSL-Certificate-Subject", ptr);

	ptr=GetNameValuePair(ptr,"/","=",&Name,&Value);
	while (ptr)
	{
		if (StrLen(Name) && (strcmp(Name,"CN")==0)) STREAMSetValue(S,"SSL-Certificate-CommonName",Value);
		ptr=GetNameValuePair(ptr,"/","=",&Name,&Value);
	}

	val=SSL_get_verify_result(ssl);

	switch(val)
	{
		case X509_V_OK: STREAMSetValue(S,"SSL-Certificate-Verify","OK"); RetVal=TRUE; break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT: STREAMSetValue(S,"SSL-Certificate-Verify","unable to get issuer"); break;
		case X509_V_ERR_UNABLE_TO_GET_CRL: STREAMSetValue(S,"SSL-Certificate-Verify","unable to get certificate CRL"); break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE: STREAMSetValue(S,"SSL-Certificate-Verify","unable to decrypt certificate's signature"); break;
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE: STREAMSetValue(S,"SSL-Certificate-Verify","unable to decrypt CRL's signature"); break;
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY: STREAMSetValue(S,"SSL-Certificate-Verify","unable to decode issuer public key"); break;
		case X509_V_ERR_CERT_SIGNATURE_FAILURE: STREAMSetValue(S,"SSL-Certificate-Verify","certificate signature invalid"); break;
		case X509_V_ERR_CRL_SIGNATURE_FAILURE: STREAMSetValue(S,"SSL-Certificate-Verify","CRL signature invalid"); break;
		case X509_V_ERR_CERT_NOT_YET_VALID: STREAMSetValue(S,"SSL-Certificate-Verify","certificate is not yet valid"); break;
		case X509_V_ERR_CERT_HAS_EXPIRED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate has expired"); break;
		case X509_V_ERR_CRL_NOT_YET_VALID: STREAMSetValue(S,"SSL-Certificate-Verify","CRL is not yet valid the CRL is not yet valid."); break;
		case X509_V_ERR_CRL_HAS_EXPIRED: STREAMSetValue(S,"SSL-Certificate-Verify","CRL has expired the CRL has expired."); break;
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid notBefore value"); break;
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid notAfter value"); break;
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid CRL lastUpdate value"); break;
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD: STREAMSetValue(S,"SSL-Certificate-Verify","invalid CRL nextUpdate value"); break;
		case X509_V_ERR_OUT_OF_MEM: STREAMSetValue(S,"SSL-Certificate-Verify","out of memory"); break;
		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT: STREAMSetValue(S,"SSL-Certificate-Verify","self signed certificate"); break;
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN: STREAMSetValue(S,"SSL-Certificate-Verify","self signed certificate in certificate chain"); break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY: STREAMSetValue(S,"SSL-Certificate-Verify","cant find root certificate in local database"); break;
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE: STREAMSetValue(S,"SSL-Certificate-Verify","ERROR: unable to verify the first certificate"); break;
		case X509_V_ERR_CERT_CHAIN_TOO_LONG: STREAMSetValue(S,"SSL-Certificate-Verify","certificate chain too long"); break;
		case X509_V_ERR_CERT_REVOKED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate revoked"); break;
		case X509_V_ERR_INVALID_CA: STREAMSetValue(S,"SSL-Certificate-Verify","invalid CA certificate"); break;
		case X509_V_ERR_PATH_LENGTH_EXCEEDED: STREAMSetValue(S,"SSL-Certificate-Verify","path length constraint exceeded"); break;
		case X509_V_ERR_INVALID_PURPOSE: STREAMSetValue(S,"SSL-Certificate-Verify","unsupported certificate purpose"); break;
		case X509_V_ERR_CERT_UNTRUSTED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate not trusted"); break;
		case X509_V_ERR_CERT_REJECTED: STREAMSetValue(S,"SSL-Certificate-Verify","certificate rejected"); break;
		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH: STREAMSetValue(S,"SSL-Certificate-Verify","subject issuer mismatch"); break;
		case X509_V_ERR_AKID_SKID_MISMATCH: STREAMSetValue(S,"SSL-Certificate-Verify","authority and subject key identifier mismatch"); break;
		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH: STREAMSetValue(S,"SSL-Certificate-Verify","authority and issuer serial number mismatch"); break;
		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN: STREAMSetValue(S,"SSL-Certificate-Verify","key usage does not include certificate signing"); break;
		case X509_V_ERR_APPLICATION_VERIFICATION: STREAMSetValue(S,"SSL-Certificate-Verify","application verification failure"); break;
	}
}
else
{
	STREAMSetValue(S,"SSL-Certificate-Verify","no certificate");
}


DestroyString(Name);
DestroyString(Value);

#endif

return(RetVal);
}




int DoSSLClientNegotiation(STREAM *S, int Flags)
{
int result=FALSE, Options=0;
#ifdef HAVE_LIBSSL
const SSL_METHOD *Method;
SSL_CTX *ctx;
SSL *ssl;
//struct x509 *cert=NULL;
char *ptr;

if (S)
{
	INTERNAL_SSL_INIT();
	//  SSL_load_ciphers();
  Method=SSLv23_client_method();
  ctx=SSL_CTX_new(Method);
  if (! ctx) HandleSSLError();
  else
  {
  STREAM_INTERNAL_SSL_ADD_SECURE_KEYS(S,ctx);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, OpenSSLVerifyCallback);
  ssl=SSL_new(ctx);
  SSL_set_fd(ssl,S->in_fd);
  STREAMSetItem(S,"LIBUSEFUL-SSL-CTX",ssl);

    Options=SSL_OP_SINGLE_DH_USE | SSL_OP_NO_SSLv2;
    ptr=STREAMGetValue(S,"SSL-Level");
    if (! StrLen(ptr)) ptr=LibUsefulGetValue("SSL-Level");

    if (StrLen(ptr))
    {
			if (strncasecmp(ptr,"ssl",3)==0) Options &= ~(SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
      if (strncasecmp(ptr,"tls",3)==0) Options |=SSL_OP_NO_SSLv3;
      if (strcasecmp(ptr,"tls1.1")==0) Options |=SSL_OP_NO_TLSv1;
      if (strcasecmp(ptr,"tls1.2")==0) Options |=SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
    }

	SSL_set_options(ssl, Options);  
	ptr=LibUsefulGetValue("SSL-Permitted-Ciphers");
	if (ptr) SSL_set_cipher_list(ssl, ptr);
  result=SSL_connect(ssl);
	while (result==-1)
	{
	result=SSL_get_error(ssl, result);
	if ( (result!=SSL_ERROR_WANT_READ) && (result != SSL_ERROR_WANT_WRITE) ) break;
	usleep(300);
  result=SSL_connect(ssl);
	}
  S->State |= SS_SSL;

	OpenSSLQueryCipher(S);
	OpenSSLVerifyCertificate(S);
	}
}

#endif
return(result);
}


#ifdef HAVE_LIBSSL
void OpenSSLSetupECDH(SSL_CTX *ctx)
{
EC_KEY* ecdh;

ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
//ecdh = EC_KEY_new_by_curve_name( NID_secp384r1);

{
	SSL_CTX_set_tmp_ecdh(ctx, ecdh);
	EC_KEY_free(ecdh);
}

}



void OpenSSLSetupDH(SSL_CTX *ctx)
{
char *Tempstr=NULL, *ptr;
DH *dh=NULL;
FILE *paramfile;

if (CachedDH) dh=CachedDH;
else
{
	ptr=LibUsefulGetValue("SSL-DHParams-File");
	if (StrLen(ptr)) Tempstr=CopyStr(Tempstr,ptr);

	paramfile = fopen(Tempstr, "r");
	if (paramfile) 
	{
 	  CachedDH = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
		dh=CachedDH;
 	  fclose(paramfile);
	}

	if (! dh) 
	{
		//OpenSSLGenerateDHParams();
		dh=CachedDH;
	}
}

if (dh) SSL_CTX_set_tmp_dh(ctx, dh);

//Don't free these parameters, as they are cached
//DH_KEY_free(dh);

DestroyString(Tempstr);
}
#endif




int DoSSLServerNegotiation(STREAM *S, int Flags)
{
int result=FALSE, Options=0;
#ifdef HAVE_LIBSSL
const SSL_METHOD *Method;
SSL_CTX *ctx;
SSL *ssl;
char *ptr;


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

    Options=SSL_OP_NO_SSLv2|SSL_OP_SINGLE_DH_USE|SSL_OP_CIPHER_SERVER_PREFERENCE;
    ptr=STREAMGetValue(S,"SSL-Level");
    if (! StrLen(ptr)) ptr=LibUsefulGetValue("SSL-Level");

    if (StrLen(ptr))
    {
			if (strncasecmp(ptr,"ssl",3)==0) Options &= ~(SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
      if (strncasecmp(ptr,"tls",3)==0) Options |=SSL_OP_NO_SSLv3;
      if (strcasecmp(ptr,"tls1.1")==0) Options |=SSL_OP_NO_TLSv1;
      if (strcasecmp(ptr,"tls1.2")==0) Options |=SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
    }

		SSL_set_options(ssl, Options);
	  SSL_set_fd(ssl,S->in_fd);
	  STREAMSetItem(S,"LIBUSEFUL-SSL-CTX",ssl);
		ptr=LibUsefulGetValue("SSL-Permitted-Ciphers");
		if (ptr) SSL_set_cipher_list(ssl, ptr);
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
			if (Flags & SSL_VERIFY_PEER) OpenSSLVerifyCertificate(S);
			OpenSSLQueryCipher(S);
			break;
 
			default:
			result=ERR_get_error();
			STREAMSetValue(S, "SSL-Error", ERR_error_string(result,NULL));
			result=FALSE;
			break;
		}
		if (result !=-1) break;
		}
	}
  }
}

#endif
return(result);
}



int STREAMIsPeerAuth(STREAM *S)
{
#ifdef HAVE_LIBSSL
void *ptr;

ptr=STREAMGetItem(S,"LIBUSEFUL-SSL-CTX");
if (! ptr) return(FALSE);

if (SSL_get_verify_result((SSL *) ptr)==X509_V_OK)
{
  if (SSL_get_peer_certificate((SSL *) ptr) !=NULL) return(TRUE);
}
#endif
return(FALSE);
}


