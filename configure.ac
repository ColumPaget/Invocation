

AC_INIT([server.c])
AC_PROG_CC 
AC_LANG_C 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_HEADER_STDC 
AC_SYS_LARGEFILE


AC_CHECK_HEADER(shadow.h,AC_DEFINE(HAVE_SHADOW_H, 1))
AC_CHECK_LIB(crypt, crypt,,)
cf_have_libcrypt=$ac_cv_lib_crypt_crypt


AC_ARG_WITH(libuseful, [  --with-libuseful		use system-wide libUseful (default=auto) ], cf_system_libuseful=$withval )

if test "$cf_system_libuseful" != "no"
then
	AC_CHECK_LIB([Useful-5],GetToken,[cf_have_libUseful=yes],[cf_have_libUseful=no])
fi

if test "$cf_have_libUseful" = "yes"
then
  echo "installed libUseful found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  AC_DEFINE([HAVE_LIBUSEFUL], [Y])
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  AC_CHECK_LIB([Useful-5],GetToken,,)

	dnl openssl isn't really needed, but if it exists and we're using a system-wide libUseful, then that will
	dnl likely be linked against it
	AC_CHECK_LIB(ssl,SSL_library_init,,)
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
else
  AC_SUBST(LIBUSEFUL_BUNDLED,"libUseful-bundled/libUseful.a")
  AC_CONFIG_SUBDIRS([libUseful-bundled])
  AC_DEFINE([HAVE_LIBUSEFUL], [N])
fi

cf_have_capabilities=no
AC_ARG_ENABLE(capabilities, [  --enable-capabilities       enable linux capabilities support (default=no)], cf_use_capabilities=$enableval )
if test "$cf_use_capabilities" != "no"
then
AC_CHECK_HEADER([sys/capability.h], [cf_have_capabilities=yes],,)
AC_CHECK_HEADER([linux/capability.h], [cf_have_capabilities=yes],,)
fi

if test "$cf_have_capabilities" = "yes"
then
AC_CHECK_LIB(cap,cap_init,,)
AC_DEFINE([USE_LINUX_CAPABILITIES])
fi

AC_CHECK_LIB(pam, pam_start,,)
cf_have_libpam=$ac_cv_lib_pam_pam_start
AC_CHECK_HEADER([security/pam_appl.h], [cf_have_pam_header=yes],,)

AC_ARG_ENABLE(pam, [  --enable-pam    enable Pluggable Authentication Modules (default=autodetect) ], cf_use_pam=$enableval )
if test "$cf_use_pam" != "no" -a "$cf_have_libpam" = "yes" -a "$cf_have_pam_header" = "yes"
then
AC_DEFINE([USE_PAM])
fi



if test "$cf_have_libpam" == "yes" -a "$cf_have_pam_header" = "yes"
then
AC_SUBST([INSTALLABLES], "invoked invoke grant pam_invocation.so")
else
AC_SUBST([INSTALLABLES], "invoked invoke grant")
fi


echo "Build settings"

if test "$cf_have_libUseful" = "yes" 
then
echo "using installed libUseful"
else
echo "using BUNDLED libUseful"
fi


AC_OUTPUT(Makefile)
