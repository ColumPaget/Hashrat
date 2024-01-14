#include "HashOpenSSL.h"

#ifdef HAVE_LIBSSL
#include <openssl/evp.h>
#include <openssl/objects.h>


static int OpenSSLFinishHash(HASH *Hash, char **Digest)
{
    int Len;

    *Digest=SetStrLen(*Digest, EVP_MAX_MD_SIZE);
    EVP_DigestFinal((EVP_MD_CTX *) Hash->Ctx, *Digest, &Len);

#ifdef HAVE_EVP_MD_CTX_FREE
    EVP_MD_CTX_free(Hash->Ctx);
#else
    EVP_MD_CTX_destroy(Hash->Ctx);
#endif

    Hash->Ctx=NULL;
    return(Len);
}


static void OpenSSLUpdateHash(HASH *Hash, const char *Bytes, int Len)
{
    EVP_DigestUpdate((EVP_MD_CTX *) Hash->Ctx, Bytes, Len);
}

static int OpenSSLInitHash(HASH *Hash, const char *Name, int Size)
{
    const EVP_MD *MD;
    const char *p_Name;

    p_Name=Name;
    if (strncmp(p_Name, "openssl:", 8)==0) p_Name += 8;
    MD=EVP_get_digestbyname(p_Name);
    if (MD)
    {
#ifdef HAVE_EVP_MD_CTX_NEW
        Hash->Ctx=(EVP_MD_CTX *) EVP_MD_CTX_new();
#else
        Hash->Ctx=(EVP_MD_CTX *) EVP_MD_CTX_create();
#endif

        EVP_DigestInit(Hash->Ctx, MD);
        Hash->Update=OpenSSLUpdateHash;
        Hash->Finish=OpenSSLFinishHash;
        return(TRUE);
    }
    return(FALSE);
}

static void OpenSSLDigestCallback(const OBJ_NAME *obj, void *arg)
{
    char *Tempstr=NULL;


    HashRegister(obj->name, 0, OpenSSLInitHash);
    Tempstr=MCopyStr(Tempstr, "openssl:", obj->name, NULL);
    HashRegister(Tempstr, 0, OpenSSLInitHash);

    Destroy(Tempstr);
}
#endif

void HashRegisterOpenSSL()
{
#ifdef HAVE_LIBSSL
    OpenSSL_add_all_digests(); //make sure they're loaded
    OBJ_NAME_do_all(OBJ_NAME_TYPE_MD_METH, OpenSSLDigestCallback, NULL);
#endif
}
