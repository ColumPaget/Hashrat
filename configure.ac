
AC_INIT([main.c])

AC_SUBST(VERSION) 

AC_CONFIG_SUBDIRS([libUseful-2.1])

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


dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)

if test "$cf_extended_attribs" = "extattr"
then
echo "DEFINE"
AC_DEFINE([USE_EXTATTR],[1])
elif test "$cf_extended_attribs" = "xattr"
then
AC_DEFINE([USE_XATTR],[1])
fi

echo
echo
echo "############# Hashrat Build Config: #############"
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



