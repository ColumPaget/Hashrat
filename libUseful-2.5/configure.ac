#AC_INIT(libUseful,colums.projects@googlemail.com,,http://sites.google.com/site/columscode) 
AC_INIT(defines.h)

VERSION="0.0.1" 
AC_SUBST(VERSION) 

AC_PROG_CC
AC_LANG_C
AC_PROG_MAKE_SET
AC_HEADER_STDC
AC_SYS_LARGEFILE

cf_have_libssl=no
cf_have_libcrypto=no
cf_have_libz=yes
cf_have_oss=no
cf_have_libesd=no
cf_have_sendfile=no
cf_have_ptsname_r=no


AC_CHECK_LIB(c,ptsname_r,,)
cf_have_ptsname_r=$ac_cv_lib_c_ptsname_r
if test "$cf_have_ptsname_r"
then
AC_DEFINE([HAVE_PTSNAME_R])
fi


AC_ARG_ENABLE(sendfile, [  --enable-sendfile       enable sendfile support], cf_use_sendfile=$enableval )

if test "$cf_use_sendfile" != "no"
then
AC_CHECK_HEADER(sys/sendfile.h,cf_have_sendfile=yes,cf_have_sendfile=no,)
fi 

if test "$cf_have_sendfile" = "yes"
then
AC_DEFINE([USE_SENDFILE])
fi



AC_ARG_ENABLE(ip6, [  --enable-ip6       enable IPv6 support], cf_use_ip6=$enableval )

if test "$cf_use_ip6" = "yes"
then
AC_DEFINE([USE_INET6],1)
fi


AC_ARG_ENABLE(ssl, [  --enable-ssl       use Secure Sockets Layer], cf_use_ssl=$enableval )

if test "$cf_use_ssl" = "yes"
then
AC_CHECK_LIB(ssl,SSL_library_init,,)
cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init

AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init

AC_CHECK_FUNCS(OpenSSL_add_all_algorithms EVP_bf_cbc EVP_rc2_cbc EVP_rc4 EVP_rc5_32_12_16_cbc EVP_des_cbc EVP_desx_cbc EVP_cast5_cbc EVP_idea_cbc EVP_aes_128_cbc EVP_aes_256_cbc)

 AC_MSG_CHECKING([that OpenSSL Library is at least version 0.9.7])   
 AC_EGREP_CPP(yes,      [
		#include <openssl/evp.h>
		#if SSLEAY_VERSION_NUMBER >= 0x00907000L
		yes        
		#endif
	],      
	[
		AC_MSG_RESULT([yes])
    AC_DEFINE(USE_OPENSSL_ADD_ALL_ALGORITHMS, 1, [Use OpenSSL_add_all_algorithms]) 
	],[AC_MSG_RESULT([no])])
fi

if test "cf_have_libssl" = "yes"
then
cf_use_zlib=yes
fi


AC_ARG_ENABLE(zlib, [  --enable-zlib       use ZLib Compression], cf_use_zlib=$enableval )

if test "$cf_use_zlib" = "yes"
then
AC_CHECK_LIB(z,deflate,,)
cf_have_libz=$ac_cv_lib_z_deflate
fi


AC_ARG_ENABLE(oss, [  --enable-oss       use Open Sound System], cf_use_oss=$enableval )

dnl Always use oss if available
cf_use_oss=yes

if test "$cf_use_oss" = "yes"
then
AC_CHECK_HEADER(sys/soundcard.h,AC_DEFINE([HAVE_OSS]),cf_have_oss=no,)
fi


#AC_ARG_ENABLE(esd, [  --enable-esd       use ESound], cf_use_esd=$enableval )

if test "$cf_use_esd" = "yes"
then
AC_CHECK_LIB(esd,esd_open_sound,,)
cf_have_libesd=$ac_cv_lib_esd_esd_open_sound
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

if test "$cf_have_sendfile" = "yes"
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


