#AC_INIT(libUseful,colums.projects@googlemail.com,,http://sites.google.com/site/columscode) 
AC_INIT(defines.h)

AC_PROG_CC
AC_LANG_C
AC_PROG_MAKE_SET
AC_HEADER_STDC
AC_SYS_LARGEFILE

cf_have_libssl=no
cf_have_libcrypto=no
cf_have_libz=no

cf_have_ptsname_r=no
cf_have_setresuid=no
cf_have_clearenv=no

cf_use_simd=no
cf_use_sendfile=no
cf_use_xattr=no


AC_CHECK_LIB(c,ptsname_r,,)
cf_have_ptsname_r=$ac_cv_lib_c_ptsname_r
if test "$cf_have_ptsname_r" = "yes"
then
AC_DEFINE([HAVE_PTSNAME_R])
fi

AC_CHECK_LIB(c,clearenv,,)
cf_have_clearenv=$ac_cv_lib_c_clearenv
if test "$cf_have_clearenv" = "yes"
then
AC_DEFINE([HAVE_CLEARENV])
fi

AC_CHECK_LIB(c,setresuid,,)
cf_have_setresuid=$ac_cv_lib_c_setresuid
if test "$cf_have_setresuid" = "yes"
then
AC_DEFINE([HAVE_SETRESUID])
fi

AC_CHECK_LIB(c,get_current_dir_name,,)
cf_have_get_current_dir_name=$ac_cv_lib_c_get_current_dir_name
if test "$cf_have_get_current_dir_name" = "yes"
then
AC_DEFINE([HAVE_GET_CURR_DIR])
fi



AC_CHECK_LIB(c,umount2,,)
cf_have_umount2=$ac_cv_lib_c_umount2
if test "$cf_have_umount2" = "yes"
then
AC_DEFINE([HAVE_UMOUNT2])
fi

AC_CHECK_LIB(c,umount,,)
cf_have_umount=$ac_cv_lib_c_umount
if test "$cf_have_umount" = "yes"
then
AC_DEFINE([HAVE_UMOUNT])
fi

AC_CHECK_LIB(c,mkostemp,,)
cf_have_mkostemp=$ac_cv_lib_c_mkostemp
if test "$cf_have_mkostemp" = "yes"
then
AC_DEFINE([HAVE_MKOSTEMP])
fi

AC_CHECK_LIB(c,poll,,)
cf_have_poll=$ac_cv_lib_c_poll
if test "$cf_have_poll" = "yes"
then
AC_DEFINE([HAVE_POLL])
fi


AC_ARG_ENABLE(sendfile, [  --enable-sendfile       enable 'fastcopy' sendfile support], cf_use_sendfile=$enableval )

if test "$cf_use_sendfile" != "no"
then
AC_CHECK_HEADER(sys/sendfile.h,cf_use_sendfile=yes,cf_use_sendfile=no,)
fi 

if test "$cf_use_sendfile" = "yes"
then
AC_DEFINE([USE_SENDFILE])
fi



AC_ARG_ENABLE(ip6, [  --enable-ip6       enable IPv6 support], cf_use_ip6=$enableval )

if test "$cf_use_ip6" = "yes"
then
AC_DEFINE([USE_INET6],1)
fi


AC_ARG_ENABLE(xattr, [  --enable-xattr       enable filesystem xattr support], cf_use_xattr=$enableval )

if test "$cf_use_xattr" = "yes"
then
AC_CHECK_LIB(c,getxattr,,)
cf_have_getxattr=$ac_cv_lib_c_getxattr
if test "$cf_have_getxattr" = "yes"
then
AC_DEFINE([HAVE_XATTR])
fi
fi


AC_ARG_ENABLE(namespaces, [  --enable-namespaces       enable linux namespace support], cf_use_namespaces=$enableval )

if test "$cf_use_namespaces" = "yes"
then
	AC_CHECK_LIB(c,unshare,,)
	cf_have_unshare=$ac_cv_lib_c_unshare
	if test "$cf_have_unshare" = "yes"
	then
		AC_DEFINE([HAVE_UNSHARE])
	fi

	AC_CHECK_LIB(c,setns,,)
	cf_have_setns=$ac_cv_lib_c_setns
	if test "$cf_have_setns" = "yes"
	then
		AC_DEFINE([HAVE_SETNS])
	fi
fi


AC_ARG_ENABLE(zlib, [  --enable-zlib       use ZLib Compression], cf_use_zlib=$enableval)

if test "$cf_use_zlib" = "yes" -o "$cf_use_ssl" = "yes"
then
AC_CHECK_LIB(z,deflate,,)
cf_have_libz=$ac_cv_lib_z_deflate
fi


AC_ARG_ENABLE(ssl, [  --enable-ssl       use Secure Sockets Layer], cf_use_ssl=$enableval )

if test "$cf_use_ssl" = "yes"
then

AC_CHECK_HEADER([openssl/crypto.h])
AC_CHECK_HEADER([openssl/ssl.h])
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/asn1.h>


AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init

dnl New versions of openssl seem to lack EVP_CIPHER_CTX_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
if test "$cf_have_libcrypto" != "yes"
then
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new,,)
	cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_new
fi


AC_CHECK_LIB(ssl,SSL_library_init,,)
cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init

dnl New versions of openssl seem to lack SSL_library_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
if test "$cf_have_libssl" != "yes"
then
	AC_CHECK_LIB(ssl,OPENSSL_init_ssl,,)
	cf_have_libssl=$ac_cv_lib_ssl_OPENSSL_init_ssl
fi


AC_CHECK_FUNCS(OpenSSL_add_all_algorithms EVP_bf_cbc EVP_rc2_cbc EVP_rc4 EVP_rc5_32_12_16_cbc EVP_des_cbc EVP_desx_cbc EVP_cast5_cbc EVP_idea_cbc EVP_aes_128_cbc EVP_aes_256_cbc X509_check_host SSL_set_tlsext_host_name)


AC_CHECK_DECLS(OpenSSL_add_all_algorithms,AC_DEFINE([HAVE_OPENSSL_ADD_ALL_ALGORITHMS]),[],[[#include <openssl/ssl.h>]])
AC_CHECK_DECLS(SSL_set_tlsext_host_name,AC_DEFINE([HAVE_SSL_SET_TLSEXT_HOST_NAME]),[],[[#include <openssl/ssl.h>]])

 AC_MSG_CHECKING([that OpenSSL Library is at least version 0.9.7])   
 AC_EGREP_CPP(yes,      [
		#include <openssl/evp.h>
		#if SSLEAY_VERSION_NUMBER >= 0x00907000L
		yes        
		#endif
	],      
	[
		AC_MSG_RESULT([yes])
#    AC_DEFINE(USE_OPENSSL_ADD_ALL_ALGORITHMS, 1, [Use OpenSSL_add_all_algorithms]) 
	],[AC_MSG_RESULT([no])])
fi

if test "$cf_have_libssl" = "yes"
then
cf_use_zlib=yes
fi


AC_ARG_ENABLE(simd, [  --enable-simd=<level>       use SIMD cpu extensions, 'level' can be one of mmx, sse, sse2], cf_use_simd=$enableval )


case $cf_use_simd in
mmx) AC_SUBST(SIMD_CFLAGS,"-mmmx");;
sse) AC_SUBST(SIMD_CFLAGS,"-mmmx -msse");;
sse2) AC_SUBST(SIMD_CFLAGS,"-mmmx -msse -msse2");;
esac


cf_have_capabilities=no
AC_ARG_ENABLE(capabilities, [  --enable-capabilities       enable linux capabilities support (default=no)], cf_use_capabilities=$enableval )

if test "$cf_use_capabilities" = "yes"
then
AC_CHECK_HEADER(sys/capability.h,cf_have_capabilities=yes,,)
AC_CHECK_HEADER(linux/capability.h,cf_have_capabilities=yes,,)
fi

if test "$cf_have_capabilities" = "yes"
then
AC_CHECK_LIB(cap,cap_init,,)
AC_DEFINE([USE_CAPABILITIES])
fi


dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)

echo
echo
echo "############# libUseful Build Config: #############"
if test "$ac_cv_sys_file_offset_bits" = "64"
then
echo "Largefiles ( > 2GB ) enabled"
else
echo "Largefiles ( > 2GB ) NOT enabled"
fi

if test "$cf_have_getxattr" = "yes"
then
echo "Filesystem attribues (xattr) enabled"
else
echo "Filesystem attribues (xattr) NOT enabled"
fi


if test "$cf_use_sendfile" = "yes"
then
echo "Linux sendfile syscall enabled"
else
echo "Linux sendfile syscall NOT enabled"
fi


if test "$cf_use_ip6" = "yes"
then
echo "IPv6 enabled"
else
echo "IPv6 NOT enabled"
fi

if test "$cf_have_libssl" = "yes"
then
echo "SSL/TLS enabled"
else
echo "SSL/TLS NOT enabled"
fi

if test "$cf_use_zlib" = "yes"
then
echo "zlib support enabled"
else
echo "zlib NOT enabled"
fi

if test "$cf_have_unshare" = "yes" -o "$cf_have_setns" = "yes"
then
echo "linux namespace support enabled"
else
echo "linux namespace support NOT enabled"
fi

if test "$cf_have_capabilities" = "yes"
then
echo "linux capabilities support enabled"
else
echo "linux capabilities support NOT enabled"
fi

