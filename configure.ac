
AC_INIT
AC_CONFIG_SRCDIR([main.c])


AC_PROG_CC 
AC_LANG([C]) 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP
 
AC_SYS_LARGEFILE

AC_ARG_ENABLE(fortify, [  --enable-fortify            use 'FORTIFY_SOURCE=3' to harden source code], cf_use_fortify=$enableval, cf_use_fortify=no )
AC_ARG_ENABLE(stack-protect, [  --enable-stack-protect      use '-fstack-protector-strong' to harden source code], cf_use_stackprotect=$enableval, cf_use_stackprotect=no )
AC_ARG_ENABLE(harden, [  --enable-harden             use 'FORTIFY_SOURCE=3' and stack-protect to harden source code], cf_use_harden=$enableval, cf_use_harden=no )



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
	AC_CHECK_LIB(c,getxattr,,)
	cf_have_getxattr=$ac_cv_lib_c_getxattr
	if test "$cf_have_getxattr" = "yes"
	then
		AC_DEFINE([USE_XATTR], [1])
	fi
fi


AC_ARG_ENABLE(zlib, [  --enable-zlib               use ZLib Compression], cf_use_zlib=$enableval, cf_use_zlib=yes)

if test "$cf_use_zlib" = "yes" -o "$cf_use_ssl" = "yes"
then
AC_CHECK_LIB(z,deflate,,)
cf_have_libz=$ac_cv_lib_z_deflate
fi


AC_ARG_ENABLE(ssl, [  --enable-ssl       use Secure Sockets Layer], cf_use_ssl=$enableval, cf_use_ssl=yes )

if test "$cf_use_ssl" = "yes"
then

AC_CHECK_HEADER([openssl/crypto.h], cf_have_crypto_h="Y")
AC_CHECK_HEADER([openssl/ssl.h], cf_have_ssl_h="Y")

if test "$cf_have_crypto_h" = "Y" -a "$cf_have_ssl_h" = "Y"
then
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
	cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init

	dnl New versions of openssl seem to lack EVP_CIPHER_CTX_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
	if test "$cf_have_libcrypto" != "yes"
	then
		AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new)
		cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_new
	fi

	AC_CHECK_LIB(ssl,SSL_library_init,,)
	cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init
	dnl New versions of openssl seem to lack SSL_library_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
	if test "$cf_have_libssl" != "yes"
	then
		AC_CHECK_LIB(ssl,OPENSSL_init_ssl)
		cf_have_libssl=$ac_cv_lib_ssl_OPENSSL_init_ssl
	fi
fi
fi


AC_ARG_WITH(system-libuseful, [  --with-system-libuseful       use systemwide libUseful.so rather than bundled version], cf_with_libuseful=$withval )


if test "$cf_with_libuseful" != "no" -a "$cf_with_libuseful" != ""
then
AC_CHECK_LIB(Useful,[GetToken])
AC_CHECK_LIB(Useful,[TOTP])
fi

if test "$ac_cv_lib_Useful_GetToken" == "yes" -a "$ac_cv_lib_Useful_TOTP" == "yes"
then
echo "Found suitable installed libUseful."
AC_CHECK_LIB([Useful],[OpenSSLVerifyCallback], 
  [AC_CHECK_LIB([ssl],[SSL_new],,,[-lcrypto -lz])] 
  [AC_CHECK_LIB([crypto],[ERR_load_CRYPTO_strings],,,[-lz])]
  [AC_CHECK_LIB([z],[inflate])]
  ,,[-lssl -lcrypto -lz])
LUSEFUL="system"
else

if test "$ac_cv_lib_Useful_GetToken" == "yes"
then
echo "Installed libUseful too old, lacks base32 and TOTP functions. Using bundled libUseful."
else
echo "No installed libUseful found, using bundled version."
fi

LUSEFUL="bundled"
STATIC_LIBS="libUseful-bundled/libUseful.a"
AC_SUBST(STATIC_LIBS)
AC_CONFIG_SUBDIRS([libUseful-bundled])
AC_DEFINE([USE_LIBUSEFUL_BUNDLED])
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
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

