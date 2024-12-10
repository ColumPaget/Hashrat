#include "HashOpenSSL.h"

#ifdef HAVE_LIBSSL
#include <openssl/evp.h>
#include <openssl/objects.h>

static void OpenSSLFreeHashCTX(HASH *Hash)
{
#ifdef HAVE_EVP_MD_CTX_FREE
    EVP_MD_CTX_free(Hash->Ctx);
#else
    EVP_MD_CTX_destroy(Hash->Ctx);
#endif

    Hash->Ctx=NULL;
}


static int OpenSSLFinishHash(HASH *Hash, char **Digest)
{
    unsigned int Len;

    *Digest=SetStrLen(*Digest, EVP_MAX_MD_SIZE);
    EVP_DigestFinal((EVP_MD_CTX *) Hash->Ctx, (unsigned char *) *Digest,  &Len);
    OpenSSLFreeHashCTX(Hash);

    return((int) Len);
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

        if (! EVP_DigestInit(Hash->Ctx, MD))
        {
            OpenSSLFreeHashCTX(Hash);
            return(FALSE);
        }

        Hash->Update=OpenSSLUpdateHash;
        Hash->Finish=OpenSSLFinishHash;

        return(TRUE);
    }
    return(FALSE);
}

//this function gets 'called back' by the call to 'OBJ_NAME_do_all' in HashRegisterOpenSSL
//and is called for each algorithm name that openssl supports
static void OpenSSLDigestCallback(const OBJ_NAME *obj, void *arg)
{
    char *Tempstr=NULL;
    HASH *Hash;

    Hash=(HASH *) calloc(1, sizeof(HASH));
    if (OpenSSLInitHash(Hash, obj->name, 0))
    {
        HashRegister(obj->name, 0, OpenSSLInitHash);
        Tempstr=MCopyStr(Tempstr, "openssl:", obj->name, NULL);
        HashRegister(Tempstr, 0, OpenSSLInitHash);
    }
    OpenSSLFreeHashCTX(Hash);

    Destroy(Tempstr);
}
#endif

void HashRegisterOpenSSL()
{
#ifdef HAVE_LIBSSL
    OpenSSL_add_all_digests(); //make sure they're loaded
    OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_MD_METH, OpenSSLDigestCallback, NULL);
#endif
}
