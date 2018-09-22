
AC_INIT([main.c])

AC_SUBST(VERSION) 


AC_PROG_CC 
AC_LANG_C 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_HEADER_STDC 
AC_SYS_LARGEFILE

AC_ARG_ENABLE(xattr, [  --enable-xattr       enable extended file attributes support], cf_enable_xattr=$enableval )

if test "$cf_enable_xattr" = "yes"
then
AC_CHECK_HEADER([sys/extattr.h],[cf_extended_attribs=extattr],[])
AC_CHECK_HEADER([sys/xattr.h],[cf_extended_attribs=xattr],[])
fi

if test "$cf_extended_attribs" = "extattr"
then
AC_DEFINE([USE_EXTATTR], [1])
elif test "$cf_extended_attribs" = "xattr"
then
AC_DEFINE([USE_XATTR], [1])
fi


AC_ARG_WITH(system-libuseful, [  --with-system-libuseful       use systemwide libUseful.so rather than bundled version], cf_with_libuseful=$withval )

echo "$cf_with_libuseful"

if test "$cf_with_libuseful" != "no" -a "$cf_with_libuseful" != ""
then
AC_CHECK_LIB([Useful],[OpenSSLVerifyCallback], 
  [AC_CHECK_LIB([ssl],[SSL_new],,,[-lcrypto -lz])] 
  [AC_CHECK_LIB([crypto],[ERR_load_CRYPTO_strings],,,[-lz])]
  [AC_CHECK_LIB([z],[inflate])]
  ,,[-lssl -lcrypto -lz])
AC_CHECK_LIB(Useful,GetToken)

LUSEFUL="system"
else
LUSEFUL="bundled"
STATIC_LIBS="libUseful-3/libUseful.a"
AC_SUBST(STATIC_LIBS)
AC_CONFIG_SUBDIRS([libUseful-3])
fi

AC_ARG_ENABLE(ssl, [  --enable-ssl       use Secure Sockets Layer], cf_use_ssl=$enableval )

if test "$cf_use_ssl" = "yes"
then

AC_CHECK_HEADER([openssl/crypto.h])
AC_CHECK_HEADER([openssl/ssl.h])

AC_CHECK_LIB(ssl,SSL_library_init,,)
cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init

AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init
fi

echo
echo
echo "############# Hashrat Build Config: #############"
echo "Using $LUSEFUL libUseful"
if test "$ac_cv_sys_file_offset_bits" = "64"
then
echo "Largefiles ( > 2GB ) enabled"
else
echo "Largefiles ( > 2GB ) NOT enabled"
fi


if test "$cf_extended_attribs" = "xattr"
then
echo "File system attributes (xattr) enabled"
elif test "$cf_extended_attribs" = "extattr"
then
echo "File system attributes (extattr) enabled"
else
echo "File system attributes NOT enabled"
fi

if test "$cf_have_libssl" = "yes"
then
echo "SSL/TLS enabled"
else
echo "SSL/TLS NOT enabled"
fi
echo
echo

dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)

