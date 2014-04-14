# AC_PROG_JAM
#
# Checks if jam/mr is installed. If the script can't find jam some usefull
# informations are displayed for the user.
#
# (c)2002 Matze Braun <MatzeBraun@gmx.de>
dnl AC_PROG_JAM()

AC_DEFUN(AC_PROG_JAM,
[
AC_ARG_ENABLE(jamtest, AC_HELP_STRING([--disable-jamtest],
	[Do not try to check for jam/mr tool]), ,enable_jamtest=yes )

if test x$enable_jamtest = xyes ; then
  AC_CHECK_TOOL(JAM, jam)
  if test "x$JAM" = x ; then
    AC_MSG_ERROR([
*** Couldn't find the jam build tool. This package requires the jam/mr tool
*** for building. Look at planeshift/docs/Compiling.txt for more informations
*** (you can disable this check with --disable-jamtest)
  ])
  fi
fi

])

AC_DEFUN(AC_JAM_FIXINSTALLDIRS,
[
# Unfortunately the install directory autoconf outputs are only usefull for
# Makefiles as they use ${prefix}...
# This hack should fix the issue

test "$exec_prefix" = '${prefix}' && exec_prefix='$(prefix)'
test "x$exec_prefix" = xNONE && exec_prefix='$(prefix)'
#ugly :-/

test "$bindir" = '${exec_prefix}/bin' && bindir='$(exec_prefix)/bin'
test "$sbindir" = '${exec_prefix}/sbin' && sbindir='$(exec_prefix)/sbin'
test "$libexecdir" = '${exec_prefix}/libexec' && libexecdir='$(exec_prefix)/libexec'
test "$datadir" = '${prefix}/share' && datadir='$(prefix)/share'
test "$sysconfdir" = '${prefix}/etc' && sysconfdir='$(prefix)/etc'
test "$sharedstatedir" = '${prefix}/com' && sharedstatedir='$(prefix)/com'
test "$localstatedir" = '${prefix}/var' && localstatedir='$(prefix)/var'
test "$libdir" = '${exec_prefix}/lib' && libdir='$(exec_prefix)/lib'
test "$includedir" = '${prefix}/include' && includedir='$(prefix)/include'
test "$infodir" = '${prefix}/info' && infodir='$(prefix)/info'
test "$mandir" = '${prefix}/man' && mandir='$(prefix)/man'

])

