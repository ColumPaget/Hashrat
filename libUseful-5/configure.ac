AC_INIT

AC_PROG_CC
AC_LANG([C])
AC_PROG_MAKE_SET
AC_PROG_EGREP
AC_SYS_LARGEFILE

cf_have_libssl=no
cf_have_libcrypto=no
cf_have_libz=no

cf_use_simd=no
cf_use_sendfile=no
cf_use_xattr=no

cf_use_hashopenssl=auto

AC_ARG_ENABLE(wall, [  --enable-wall               use -Wall to get maximum compile warnings], cf_use_wall=$enableval, cf_use_wall=no )
if test "$cf_use_wall" = "yes"
then
CFLAGS="$CFLAGS -Wall"
fi


AC_ARG_ENABLE(werror, [  --enable-werror             treat compile warnings as errors], cf_use_werror=$enableval, cf_use_werror=no )
if test "$cf_use_werror" = "yes"
then
CFLAGS="$CFLAGS -Werror"
fi


AC_ARG_ENABLE(fortify, [  --enable-fortify            use 'FORTIFY_SOURCE=3' to harden source code], cf_use_fortify=$enableval, cf_use_fortify=no )
AC_ARG_ENABLE(stack-protect, [  --enable-stack-protect      use '-fstack-protector-strong' to harden source code], cf_use_stackprotect=$enableval, cf_use_stackprotect=no )
AC_ARG_ENABLE(harden, [  --enable-harden             use 'FORTIFY_SOURCE=3' and stack-protect to harden source code], cf_use_harden=$enableval, cf_use_harden=no )
AC_ARG_ENABLE(soname, [  --enable-soname             use 'soname' in shared lib. Defaults to 'yes' for gcc 'no' otherwise], cf_use_soname=$enableval, cf_use_soname=gcc )

AC_SUBST(SONAME_FLAGS,"")
if test "$cf_use_soname" != "no"
then
  if test "$GCC" = "yes"
  then
        AC_SUBST([SONAME_FLAGS], ['-Wl,-soname,${SONAME}'])
  else
      if test "$cf_use_soname" = "yes"
      then
        AC_SUBST([SONAME_FLAGS], ['-Wl,-soname,${SONAME}'])
      fi
  fi
fi


if test "$cf_use_harden" = "yes"
then
cf_use_fortify="yes"
cf_use_stackprotect="yes"
fi


if test "$cf_use_fortify" = "yes"
then
CFLAGS="$CFLAGS -O2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3"
fi

if test "$cf_use_stackprotect" = "yes"
then
CFLAGS="$CFLAGS -fstack-protector-strong"
fi



AC_CHECK_LIB(c,get_current_dir_name,,)
cf_have_get_current_dir_name=$ac_cv_lib_c_get_current_dir_name
if test "$cf_have_get_current_dir_name" = "yes"
then
AC_DEFINE([HAVE_GET_CURR_DIR])
fi


AC_CHECK_FUNCS(ptsname_r clearenv setresuid initgroups poll mlock mlockall munlockall madvise htonll ntohll mkostemp mount umount umount2 getentropy)


AC_ARG_ENABLE(sendfile, [  --enable-sendfile           enable 'fastcopy' sendfile support], cf_use_sendfile=$enableval, cf_use_sendfile=auto )
if test "$cf_use_sendfile" != "no"
then
AC_CHECK_HEADER(sys/sendfile.h,cf_use_sendfile=yes,cf_use_sendfile=no,)
if test "$cf_use_sendfile" != "no"
then
AC_CHECK_FUNC(sendfile, AC_DEFINE([HAVE_SENDFILE]))
fi

fi 



AC_ARG_ENABLE(ip6, [  --enable-ip6                enable IPv6 support], cf_use_ip6=$enableval, cf_use_ip6=yes )
if test "$cf_use_ip6" = "yes"
then
AC_DEFINE([USE_INET6],1)
fi


AC_ARG_ENABLE(xattr, [  --enable-xattr              enable filesystem xattr support], cf_use_xattr=$enableval, cf_use_xattr=yes )
if test "$cf_use_xattr" = "yes"
then
AC_CHECK_LIB(c,getxattr,,)
cf_have_getxattr=$ac_cv_lib_c_getxattr
if test "$cf_have_getxattr" = "yes"
then
AC_DEFINE([HAVE_XATTR])
fi
fi


AC_ARG_ENABLE(namespaces, [  --enable-namespaces         enable linux namespace support], cf_use_namespaces=$enableval, cf_use_namespaces=yes )
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



AC_ARG_ENABLE(ssl, [  --enable-ssl                use Secure Sockets Layer], cf_use_ssl=$enableval, cf_use_ssl=yes )

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



if test "$cf_have_libssl" = "yes"
then
cf_use_zlib=yes

AC_CHECK_FUNCS(OpenSSL_add_all_algorithms EVP_MD_CTX_create EVP_MD_CTX_new EVP_MD_CTX_destroy EVP_MD_CTX_free EVP_bf_cbc EVP_rc2_cbc EVP_rc4 EVP_rc5_32_12_16_cbc EVP_des_cbc EVP_desx_cbc EVP_cast5_cbc EVP_idea_cbc EVP_aes_128_cbc EVP_aes_256_cbc X509_check_host SSL_set_tlsext_host_name SSL_set_min_proto_version)


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


fi



AC_ARG_ENABLE(zlib, [  --enable-zlib               use ZLib Compression], cf_use_zlib=$enableval, cf_use_zlib=yes)

if test "$cf_use_zlib" = "yes" -o "$cf_use_ssl" = "yes"
then
AC_CHECK_LIB(z,deflate,,)
cf_have_libz=$ac_cv_lib_z_deflate
fi


AC_ARG_ENABLE(simd, [  --enable-simd=<level>       use SIMD cpu extensions, 'level' can be one of mmx, sse, sse2], cf_use_simd=$enableval )


case $cf_use_simd in
mmx) AC_SUBST(SIMD_CFLAGS,"-mmmx");;
sse) AC_SUBST(SIMD_CFLAGS,"-mmmx -msse");;
sse2) AC_SUBST(SIMD_CFLAGS,"-mmmx -msse -msse2");;
esac


cf_have_capabilities=no
AC_ARG_ENABLE(capabilities, [  --enable-capabilities       enable linux capabilities support (default=no)], cf_use_capabilities=$enableval, cf_use_capabilities=no )

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
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo
echo
echo "############# libUseful Build Config: #############"

echo "compile with -Wall - $cf_use_wall"
echo "compile with -Werror - $cf_use_werror"
echo "compile with FORTIFY_SOURCE=3 - $cf_use_fortify"
echo "compile with -fstack-protector-strong - $cf_use_stackprotect"

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


if test "$ac_cv_func_sendfile" = "yes"
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

if test "$cf_have_libz" = "yes"
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

