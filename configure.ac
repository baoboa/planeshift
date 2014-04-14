#----------------------------------------------------------------------------
# Copyright (C)2001-2013 by Atomic Blue.
#
# Script taken from the Crystal Space SDK project.
#
# Autoconf input script for PlaneShift. Start the ./autogen.sh script which
# calls autoconf to produce a configure script.
#----------------------------------------------------------------------------
AC_PREREQ([2.56])

#----------------------------------------------------------------------------
# Initialize autoconf
#----------------------------------------------------------------------------
AC_INIT([PlaneShift], [0.6.1], [http://www.hydlaaplaza.com/flyspray/])
CS_PACKAGEINFO([PlaneShift a 3d MMORPG],
	[Copyright (C)2001-2013 Atomic Blue],
	[http://www.planeshift.it/])
AC_CONFIG_SRCDIR([mk/jam/build.jam])
AC_CONFIG_AUX_DIR([mk/autoconf])
AC_CANONICAL_HOST
AC_PREFIX_DEFAULT(/usr/local/planeshift)

#----------------------------------------------------------------------------
# Setup for the configuration header
#----------------------------------------------------------------------------
AC_CONFIG_HEADERS(include/psconfig.h:include/psconfig.h.in)
#AC_CONFIG_HEADERS(include/config.h:include/psconfig.h.in)
AH_TOP([
#ifdef __CONFIG_H__
#   error PLEASE include this file only once in each .cpp file! (not in .h)
#endif
#define __CONFIG_H__

/* we'll use CS everywhere... and that too has it's config.h like type... we
 * just include it here
 */
#include <cssysdef.h>
])

#----------------------------------------------------------------------------
# Check for common stuff
#----------------------------------------------------------------------------
CS_PROG_CC
CS_PROG_CXX
CS_PROG_LINK
CS_CHECK_COMMON_TOOLS_LINK
CS_CHECK_COMMON_TOOLS_BASIC
CS_CHECK_COMMON_TOOLS_DOC_DOXYGEN

#----------------------------------------------------------------------------
# Determine system type
#----------------------------------------------------------------------------
CS_CHECK_HOST

AS_IF([test "$cs_host_family" = "windows"],
     [AC_DEFINE([USE_WINSOCK],, [Define when using winsock])
     CS_JAMCONFIG_PROPERTY([COMPILER.LFLAGS], [-lwsock32], [+])],
     [AC_DEFINE([USE_UNISOCK],, [Define when using normal sockets])])

#----------------------------------------------------------------------------
# Check for syntax problems / header files
#----------------------------------------------------------------------------
TYPE_SOCKLEN_T

AC_NEED_STDINT_H(include/psstdint.h)

#------------------------------------------------------------------------------
# Check for sizeof(long) to determine whether the platform is 32 or 64 bit.
#------------------------------------------------------------------------------
AC_DEFUN([CS_CHECK_LONG_BITS],
    [AC_CHECK_SIZEOF([long])
    AS_IF([test $ac_cv_sizeof_long = 8],
	[cs_long_bits=64], [cs_long_bits=32])])

CS_CHECK_LONG_BITS
CS_HEADER_PROPERTY([CS_PROCESSOR_SIZE], [$cs_long_bits])
CS_EMIT_BUILD_PROPERTY([TARGET.PROCESSORSIZE], [$cs_long_bits])
CS_HEADER_PROPERTY([CS_LONG_SIZE], [$ac_cv_sizeof_long])

#------------------------------------------------------------------------------
# Check how to enable and disable compilation warnings. These are only enabled 
# in debug.
# We enable warnings in optimize mode as well, which can be found in Jamrules.
#------------------------------------------------------------------------------
CS_COMPILER_WARNINGS([C++], [cs_cv_prog_cxx_enable_warnings],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS.debug],
	[$cs_cv_prog_cxx_enable_warnings], [+])])

#------------------------------------------------------------------------------
# Check for compiler- and CPU-specific code generation flags.  These flags are
# mutually exclusive.  Exclusivity is enforced by the CS_CHECK_CODE_GEN_FLAG()
# macro.  Note that use of these flags typically means that the resulting
# binary will not be backward compatible.  For instance, if Pentium-specific
# instructions are used, the program will not work with earlier processors,
# such as the Intel 486.  Because these options are potentially dangerous,
# their use it controlled by the user via --enable-cpu-specific-optimizations
# which accepts "no" (same as "disable"), "minimum" (the default), and
# "maximum".  The "minimum" option selects optimizations which are likely to be
# supported by reasonably modern hardware, thus making the resulting
# executables fairly portable.
#
# CS_CHECK_CODE_GEN_FLAG(CPU-PATTERN, FLAGS, [LANGUAGE])
#	If compiler- and CPU-specific optimization flags have not already been
#	determined, and if the canonical host CPU matches CPU-PATTERN, try
#	using the provided compiler FLAGS with the compiler represented by
#	LANGUAGE (typically either "C" or "C++").  If language is omitted, C++
#	is used.  If the compiler accepts the flags, assign them to the shell
#	variable cs_cv_prog_cxx_code_gen.
#------------------------------------------------------------------------------
AC_DEFUN([CS_CHECK_CODE_GEN_FLAG],
    [AC_REQUIRE([AC_CANONICAL_HOST])
    AS_IF([test "$cs_cv_prog_cxx_code_gen" = no],
	[case $host_cpu in
	    $1) CS_BUILD_IFELSE([],
		    [CS_CREATE_TUPLE([$2])], m4_default([$3],[C++]),
		    [cs_cv_prog_cxx_code_gen=$cs_build_cflags], [])
		;;
	esac])])

AC_MSG_CHECKING([whether to use CPU-specific optimizations])
AC_ARG_ENABLE([cpu-specific-optimizations],
    [AC_HELP_STRING([--enable-cpu-specific-optimizations=level],
	[enable CPU-specific optimizations; recognized levels are `no',
	`minimum' or `min', `maximum' or `max', `native' (default MINIMUM);
	the `minimum' option selects optimizations which are likely to be 
	supported by reasonably modern hardware, thus making the resulting 
	executables fairly portable; use the `maximum' option with caution 
	since code for one processor will not work with earlier processors
	(for example, Intel 686-specific code will not work with a 586). The
	`native' option select optimizations specific for the compiling machine
	(thus giving a fairly unportable executable).])],
    [case $enable_cpu_specific_optimizations in
	yes|min) enable_cpu_specific_optimizations=minimum ;;
	max) enable_cpu_specific_optimizations=maximum ;;
    esac],
    [enable_cpu_specific_optimizations=minimum])
AC_MSG_RESULT([$enable_cpu_specific_optimizations])

case $enable_cpu_specific_optimizations in
    no|minimum|maximum|native) ;;
    *) CS_MSG_ERROR([unrecognized option; use `no', `minimum', `maximum' or `native'])
       ;;
esac

AS_IF([test $enable_cpu_specific_optimizations != no],
    [AC_CACHE_CHECK([for code generation flags], [cs_cv_prog_cxx_code_gen],
	[cs_cv_prog_cxx_code_gen=no
	AS_IF([test $enable_cpu_specific_optimizations = native],
	    [CS_CHECK_CODE_GEN_FLAG([*], [-march=native -mtune=native])])
	AS_IF([test $enable_cpu_specific_optimizations = maximum \
		|| test $enable_cpu_specific_optimizations = native],
	    [CS_CHECK_CODE_GEN_FLAG([*686], [-march=pentium3 -mtune=generic])
	    CS_CHECK_CODE_GEN_FLAG([*686], [-march=pentium3 -mtune=pentium3])
	    CS_CHECK_CODE_GEN_FLAG([*686], [-march=pentium3])])
	AS_IF([test $host_vendor = apple],
	    [# Work around issue on intel macs, where "-fpmath=sse" is on by
	     # default, but this breaks compilation when combined with 
	     # the "-march=i586" selected below. Furthermore, -march=i686 was
	     # reported to not work, but -march=prescott so, so try to use 
	     # that as the "minimum" optimization on x86 Apples, followed by
	     # -march=i686.
	    CS_CHECK_CODE_GEN_FLAG([[*[3-9]86]], [-march=prescott])
	    CS_CHECK_CODE_GEN_FLAG([[*[3-9]86]], [-march=i686])])
	CS_CHECK_CODE_GEN_FLAG([x86_64], [-mtune=generic])
	CS_CHECK_CODE_GEN_FLAG([x86_64], [-march=k8])
	CS_CHECK_CODE_GEN_FLAG([x86_64], [-mtune=k8])
	CS_CHECK_CODE_GEN_FLAG([*686], [-march=i686 -mtune=generic])
	CS_CHECK_CODE_GEN_FLAG([*686], [-march=i686])
	CS_CHECK_CODE_GEN_FLAG([*686], [-mcpu=pentiumpro -march=i686])
	CS_CHECK_CODE_GEN_FLAG([*686], [-mcpu=pentiumpro])
	CS_CHECK_CODE_GEN_FLAG([*686], [-mpentiumpro -march=i686])
	CS_CHECK_CODE_GEN_FLAG([*686], [-mpentiumpro])
	CS_CHECK_CODE_GEN_FLAG([[*[5-6]86]], [-march=i586 -mtune=generic])
	CS_CHECK_CODE_GEN_FLAG([[*[5-6]86]], [-march=i586])
	CS_CHECK_CODE_GEN_FLAG([[*[5-6]86]], [-mcpu=pentium -march=i586])
	CS_CHECK_CODE_GEN_FLAG([[*[5-6]86]], [-mcpu=pentium])
	CS_CHECK_CODE_GEN_FLAG([[*[5-6]86]], [-mpentium -march=i586])
	CS_CHECK_CODE_GEN_FLAG([[*[5-6]86]], [-mpentium])
	CS_CHECK_CODE_GEN_FLAG([[*[3-9]86]], [-march=i486])
	CS_CHECK_CODE_GEN_FLAG([[*[3-9]86]], [-mcpu=i486 -march=i486])
	CS_CHECK_CODE_GEN_FLAG([[*[3-9]86]], [-mcpu=i486])
	CS_CHECK_CODE_GEN_FLAG([[*[3-9]86]], [-m486])
	CS_CHECK_CODE_GEN_FLAG([alpha], [-mieee])])

    AS_IF([test "$cs_cv_prog_cxx_code_gen" != no],
	[CS_EMIT_BUILD_PROPERTY(
	    [COMPILER.CFLAGS], [$cs_cv_prog_cxx_code_gen], [+])])])



#------------------------------------------------------------------------------
# Check for SIMD flags. These checks are performed after CS_CHECK_HOST() and
# the CPU optimization flags detection since these may alter the build 
# environment in a way which affects the SIMD flags tests. For instance,
# they might be impacted on Mac OS X by a universal binary build, or only the
# "-march" parameter chosen by the optimization flags detection may enable
# support for specific SIMD instructions.
#------------------------------------------------------------------------------
CS_CHECK_BUILD_FLAGS([for -mmmx flag], [cs_cv_prog_cc_mmmx],
    [CS_CREATE_TUPLE([-mmmx])], [],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS.MMX], [$cs_cv_prog_cc_mmmx],
        [append])], [],	[$cs_cv_prog_cxx_code_gen])
CS_CHECK_BUILD_FLAGS([for -msse flag], [cs_cv_prog_cc_msse],
    [CS_CREATE_TUPLE([-msse])], [],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS.SSE], [$cs_cv_prog_cc_msse],
        [append])], [],	[$cs_cv_prog_cxx_code_gen])
CS_CHECK_BUILD_FLAGS([for -msse2 flag], [cs_cv_prog_cc_msse2],
    [CS_CREATE_TUPLE([-msse2])], [],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS.SSE2], [$cs_cv_prog_cc_msse2],
        [append])], [],	[$cs_cv_prog_cxx_code_gen])
CS_CHECK_BUILD_FLAGS([for -mfpmath=sse flag], [cs_cv_prog_cc_mfpmath_sse],
    [CS_CREATE_TUPLE([-mfpmath=sse])], [],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS.SSE], [$cs_cv_prog_cc_mfpmath_sse],
        [append])], [],
	[$cs_cv_prog_cxx_code_gen $cs_cv_prog_cxx_enable_errors])

AS_IF([test -n "$cs_cv_prog_cc_mmmx"],
    [CS_CHECK_BUILD([for mmintrin.h],
        [cs_cv_header_mmintrin_h],
        [AC_LANG_PROGRAM([[#include <mmintrin.h>]], [])],
        [CS_CREATE_TUPLE([$cs_cv_prog_cc_mmmx])],
        [C++],
        [CS_HEADER_PROPERTY([CS_SUPPORTS_MMX])
        CS_EMIT_BUILD_PROPERTY([CS_SUPPORTS_MMX], [yes])])])

#------------------------------------------------------------------------------
# Check for compiler debug flags.
#------------------------------------------------------------------------------
binutils_recent=no
AS_IF([test -n "$OBJCOPY"],
    [CS_CHECK_PROG_VERSION([binutils], [$OBJCOPY -V], [2.16], [9.9|.9|.9|.9],
        [binutils_recent=yes])])

AC_MSG_CHECKING([whether to split debug information])
AC_ARG_ENABLE([separate-debug-info],
    [AC_HELP_STRING([--enable-separate-debug-info],
        [split debug information into separate files. (default YES if
        binutils >= 2.16 detected and not on Cygwin, otherwise default NO)])])
AS_IF([test -z "$enable_separate_debug_info"],
    AS_IF([test -z `uname | grep -i CYGWIN`],
        [enable_separate_debug_info=$binutils_recent],
        [enable_separate_debug_info=no]))
AC_MSG_RESULT([$enable_separate_debug_info])

CS_EMIT_BUILD_PROPERTY([LINK.DEBUG.INFO.SEPARATE], [$enable_separate_debug_info])

CS_EMIT_BUILD_FLAGS([how to enable debug mode debugging symbols],
    [cs_cv_prog_cxx_debug_symbols_extra],
    [CS_CREATE_TUPLE([-g3]) CS_CREATE_TUPLE([-g2]) CS_CREATE_TUPLE([-g])],
    [C++], [COMPILER.CFLAGS.debug], [+],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.LFLAGS.debug],
	[$cs_cv_prog_cxx_debug_symbols_extra], [+])])
	
AC_ARG_WITH([optimize-debug-info], [AC_HELP_STRING([--with-optimize-debug-info],
    [whether to enable debug information even in optimize builds. 
    (default YES if separate debug information is enabled, 
    otherwise default NO)])])
AS_IF([test -z "$with_optimize_debug_info"], 
    [with_optimize_debug_info=$with_separate_debug_info])
AS_IF([test "$with_optimize_debug_info" != no],
    [CS_EMIT_BUILD_FLAGS([how to enable optimize mode debugging symbols],
	[cs_cv_prog_cxx_debug_symbols_normal],
	[CS_CREATE_TUPLE([-g2]) CS_CREATE_TUPLE([-g])],
	[C++], [COMPILER.CFLAGS.optimize], [+],
	[CS_EMIT_BUILD_PROPERTY([COMPILER.LFLAGS.optimize],
	    [$cs_cv_prog_cxx_debug_symbols_normal], [+])])])

#------------------------------------------------------------------------------
# Check for compiler optimization flags.
#------------------------------------------------------------------------------
AC_DEFUN([CS_CHECK_OPTIMIZE_FLAGS],
    [AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([AC_PROG_CXX])

    cs_optimize_flags=''

    CS_EMIT_BUILD_FLAGS([how to enable optimizations],
	[cs_cv_prog_cxx_optimize],
	[CS_CREATE_TUPLE([-O3]) CS_CREATE_TUPLE([-O3]) CS_CREATE_TUPLE([-Os]) CS_CREATE_TUPLE([-O])],
	[C++], [COMPILER.CFLAGS.optimize], [+],
	[cs_optimize_flags="$cs_optimize_flags $cs_cv_prog_cxx_optimize"])

    CS_EMIT_BUILD_FLAGS([if -fno-omit-frame-pointer is accepted],
	[cs_cv_prog_cxx_omit_frame_pointer],
	[CS_CREATE_TUPLE([-fno-omit-frame-pointer])],
	[C++], [COMPILER.CFLAGS.optimize], [+],
	[cs_optimize_flags="$cs_optimize_flags \
	    $cs_cv_prog_cxx_omit_frame_pointer"])
		
    CS_EMIT_BUILD_FLAGS([if -ffast-math is accepted],
	[cs_cv_prog_cxx_fast_math],
	[CS_CREATE_TUPLE([-ffast-math])], [C++],
	[COMPILER.CFLAGS.optimize], [+],
	[cs_optimize_flags="$cs_optimize_flags $cs_cv_prog_cxx_fast_math"])])

CS_CHECK_OPTIMIZE_FLAGS

#------------------------------------------------------------------------------
# Check how to declare symbol visibility.
#------------------------------------------------------------------------------
CS_VISIBILITY_FLAG_INLINES_HIDDEN(
    [CS_EMIT_BUILD_PROPERTY([COMPILER.C++FLAGS],
	[$cs_prog_cxx_visibility_inlines_hidden], [+])])

CS_VISIBILITY_FLAG_HIDDEN([C], [],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS], 
	[$cs_cv_prog_c_visibility_hidden], [+])])

CS_VISIBILITY_FLAG_DEFAULT([C], [],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS.VISIBILITY_DEFAULT], 
	[$cs_cv_prog_c_visibility_default])])

CS_VISIBILITY_DECLARE_HIDDEN([C++], [],
    [CS_HEADER_PROPERTY([CS_VISIBILITY_HIDDEN],
	[$cs_cv_prog_cxx_declare_visibility_hidden])])

CS_VISIBILITY_DECLARE_DEFAULT([C++], [],
    [CS_HEADER_PROPERTY([CS_VISIBILITY_DEFAULT],
	[$cs_cv_prog_cxx_declare_visibility_default])])

#------------------------------------------------------------------------------
# Determine whether to link base g++ libraries statically
#------------------------------------------------------------------------------
AC_MSG_CHECKING([whether to link base g++ libraries statically])
AC_ARG_ENABLE([static-baselibs], [AC_HELP_STRING([--enable-static-baselibs],
    [Link statically againts libgcc and libstdc++. Use with caution; enabling
    this option may cause issues when dynamically linking against other C++
    modules. (default NO)])],
    [], [enable_static_baselibs=no])
AC_MSG_RESULT([$enable_static_baselibs])
AS_IF([test $enable_static_baselibs = yes],
    [CS_EMIT_BUILD_FLAGS([for -static-libgcc],
        [cs_cv_prog_link_static_libgcc], [CS_CREATE_TUPLE([-static-libgcc])],
        [C], [COMPILER.LFLAGS], [+])
    libstdcxx=`$CXX -print-file-name=libstdc++.a`
    CS_EMIT_BUILD_FLAGS([for static libstdc++],
        [cs_cv_prog_link_static_libstdcxx],
        CS_CREATE_TUPLE([$libstdcxx]),
        [C++], [COMPILER.LFLAGS], [+])])

#----------------------------------------------------------------------------
# Check for Crystal Space (CS)
#----------------------------------------------------------------------------
AC_ARG_WITH([cs], [AC_HELP_STRING([--without-cs],
  [do not look for an installed Crystal Space SDK; this option is useful only
  if you need to perform routine maintenance tasks, such as generating
  project files or converting documentation, without having the Crystal Space
  SDK installed])])
AS_IF([test -z "$with_cs"], [with_cs=yes])

AS_IF([test $with_cs != no],
[CS_PATH_CRYSTAL_EMIT([2.1], [],
  [AC_MSG_ERROR([
*** Crystal Space could not be found or was unusable. The latest version is
*** always available from http://www.crystalspace3d.org/
*** Also, be sure that you have either installed Crystal Space or set the
*** CRYSTAL environment variable properly.
])], [crystalspace])])

CS_CHECK_COMMON_TOOLS_RELAYTOOL([$CRYSTAL_TOOLS_PREFIX/bin])

#------------------------------------------------------------------------------
# Check for subversion and, if available, whether the source directory is an
# SVN checkout.
#------------------------------------------------------------------------------
CS_CHECK_TOOLS([SUBVERSION], [svn])
AS_IF([test -n "$SUBVERSION"],
    [AC_CACHE_CHECK([whether ${srcdir} is an SVN checkout], [ps_cv_source_is_svn],
	[AS_IF([AC_TRY_COMMAND([$SUBVERSION info ${srcdir} > /dev/null])],
	    [ps_cv_source_is_svn=yes], [ps_cv_source_is_svn=no])])],
    [ps_cv_source_is_svn=no])
CS_EMIT_BUILD_PROPERTY([SOURCE_IS_SVN], [$_cv_source_is_svn])

#----------------------------------------------------------------------------
# Check for MySQL
#----------------------------------------------------------------------------

CS_NOTABLE([mysql], [database], [], [$cs_cv_libmysqlclient],
    [CS_CHECK_LIB_WITH([mysqlclient],
	[AC_LANG_PROGRAM([[#include <mysql.h>]],
		[MYSQL mysql; mysql_init(&mysql);])], [/usr /usr/include/mysql|/usr/lib /usr/local/mysql /usr/include/mysql|/usr/lib/mysql /usr/local/include/mysql|/usr/local/lib/mysql /usr/lib /usr/local/mysql], [],
		[CS_EMIT_BUILD_RESULT([cs_cv_libmysqlclient], [LIBMYSQLCLIENT])])])

#----------------------------------------------------------------------------
# Check for Sqlite3
#----------------------------------------------------------------------------

CS_NOTABLE([Sqlite3], [database], [], [$cs_cv_libsqlite3],
    [CS_CHECK_LIB_WITH([sqlite3],
	[AC_LANG_PROGRAM([[#include <sqlite3.h>]],
		[sqlite3 **db; sqlite3_open("",db);])], [/usr /usr/include/|/usr/lib /usr/local/ /usr/include/|/usr/lib/ /usr/local/include/|/usr/local/lib/ /usr/lib /usr/local/], [],
		[CS_EMIT_BUILD_RESULT([cs_cv_libsqlite3], [LIBSQLITE3])])])

#----------------------------------------------------------------------------
# Check for PostgreSQL
#----------------------------------------------------------------------------

CS_NOTABLE([PostgreSQL], [database], [], [$cs_cv_libpq],
    [CS_CHECK_LIB_WITH([pq],
	[AC_LANG_PROGRAM([[#include <postgres.h>]],
		[PQconnectdb("");])], [/usr /usr/include/postgresql|/usr/lib /usr/local/ /usr/include/postgresql|/usr/lib/ /usr/local/include/postgresql|/usr/local/lib/ /usr/lib /usr/local/], [],
		[CS_EMIT_BUILD_RESULT([cs_cv_libpq], [LIBPQ])])])

#----------------------------------------------------------------------------
# Check for Curl
#----------------------------------------------------------------------------

PS_CHECK_LIBCURL

#----------------------------------------------------------------------------
# Check for hunspell
#----------------------------------------------------------------------------

CS_NOTABLE([hunspell], [], [], [$cs_cv_libhunspell],
    [CS_CHECK_LIB_WITH([hunspell],
	[AC_LANG_PROGRAM([[#include <hunspell.hxx>]],
	[Hunspell* spell=new Hunspell("",""); delete spell;])],	[/usr /usr/include/|usr/lib /usr/local/ /usr/include/hunspell|/usr/lib/ /usr/local/include/|/usr/local/lib/ /usr/local/include/hunspell/|/usr/local/lib/ /usr/lib/], [C++],
	[CS_EMIT_BUILD_RESULT([cs_cv_libhunspell], [HUNSPELL])])])

#----------------------------------------------------------------------------
# Check for Google Test
#----------------------------------------------------------------------------

CS_CHECK_LIB_WITH([gtest],
	[AC_LANG_PROGRAM([[#include <gtest/gtest.h>]],
		[int dummyargc = 1; char* dummyargv = "gtest"; testing::InitGoogleTest(&dummyargc, &dummyargv);])], [], [C++],
		[CS_EMIT_BUILD_RESULT([cs_cv_libgtest], [GTEST])], [], [], [], [], [])
		
#----------------------------------------------------------------------------
# Check for Google Breakpad
#----------------------------------------------------------------------------

CS_CHECK_LIB_WITH([breakpad],
	[AC_LANG_PROGRAM([[#ifdef CS_PLATFORM_MACOSX
#include "client/mac/handler/exception_handler.h"
#else
#include "client/linux/handler/exception_handler.h"
#endif]],
		[google_breakpad::ExceptionHandler test_handler("/tmp/",	NULL, NULL,	NULL, true);])], [src/tools/google-breakpad/src src/tools/google-breakpad/src|src/tools/breakpad-libs], [C++],
		[CS_EMIT_BUILD_RESULT([cs_cv_libbreakpad], [BREAKPAD])], [], [], [], [], [])


#------------------------------------------------------------------------------
# Check if TemplateToolkit2 is installed (needed for MSVC project generation).
#------------------------------------------------------------------------------
CS_CHECK_TEMPLATE_TOOLKIT2([emit])

#------------------------------------------------------------------------------
# The cs-win32libs package is a convenience archive made available to Windows
# users of Crystal Space.  It contains common libraries, headers, and tools
# (such as zlib, libjpeg, cal3d, etc.) usable by MSVC, Mingw/MSYS, and Cygwin
# users.  It saves users the bother of having to install these packages
# manually one at a time.
#------------------------------------------------------------------------------
CS_CHECK_CSWIN32LIBS

#------------------------------------------------------------------------------
# Use the CPPFLAGS, CXXFLAGS, CFLAGS, and LDFLAGS passed to configure, as well
# as any additional flags provided by cs-win32libs.
#------------------------------------------------------------------------------
CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS], [$CPPFLAGS $CFLAGS], [+])
CS_EMIT_BUILD_PROPERTY([COMPILER.C++FLAGS], [$CPPFLAGS $CXXFLAGS], [+])
CS_EMIT_BUILD_PROPERTY([COMPILER.LFLAGS], [$LDFLAGS], [+])

#----------------------------------------------------------------------------
# Check if the Crystal Space static plugins library was built.
#----------------------------------------------------------------------------
AS_IF([test $with_cs != no],
  [AC_MSG_CHECKING([for Crystal Space static plugins])
  cc_have_crystal_static=no
  for m in $CRYSTAL_AVAILABLE_LIBS; do
      AS_IF([test crystalspace_staticplugins = "$m"],
	  [cc_have_crystal_static=yes
	  break])
  done
  AC_MSG_RESULT([$cc_have_crystal_static])],
  [cc_have_crystal_static=no])

AS_IF([test $cc_have_crystal_static != no],
  [CS_EMIT_BUILD_PROPERTY([HAVE_STATIC_PLUGINS], [yes])])

#------------------------------------------------------------------------------
# Check for Cal3D.
#------------------------------------------------------------------------------
CS_CHECK_LIB_WITH([cal3d],
    [AC_LANG_PROGRAM([[#include <cal3d/cal3d.h>]],
    [CalModel* m = 0; m->getMixer();])], [], [C++])

    
AS_IF([test $cs_cv_libcal3d = yes],
    [CS_CHECK_BUILD([if cal3d is sufficiently recent], [cs_cv_cal3d_recent],
    [AC_LANG_PROGRAM(
        [[#include <cal3d/cal3d.h>]],
        [CalCoreAnimation* p = 0; unsigned int n = p->getTrackCount();])],
    [], [C++],
    [CS_EMIT_BUILD_RESULT([cs_cv_libcal3d], [CAL3D])], [], [],
    [$cs_cv_libcal3d_cflags],
    [$cs_cv_libcal3d_lflags],
    [$cs_cv_libcal3d_libs])])

#------------------------------------------------------------------------------
# Determine if plugin meta-information should be embedded directly into plugin
# modules or if it should exist in stand-alone .csplugin files.  On Unix,
# embedding is accomplished via libbfd which carries a GPL license.  If your
# project is compatible with GPL, then set gpl_compatible to "yes".  This will
# cause embedding to be enabled on Unix by default. Otherwise, set it to "no",
# which will cause embedding to be disabled by default on Unix. (In any event,
# the user can override the default setting with the
# --enable-meta-info-embedding option.)
#------------------------------------------------------------------------------
m4_define([gpl_compatible], [yes])
CS_META_INFO_EMBED([emit], gpl_compatible)
AS_IF([test $enable_meta_info_embedding = yes],
    [CS_EMIT_BUILD_PROPERTY([COMPILER.CFLAGS],
	[AS_ESCAPE([$(EMBED_META.CFLAGS)])], [+])
    CS_EMIT_BUILD_PROPERTY([COMPILER.LFLAGS],
	[AS_ESCAPE([$(EMBED_META.LFLAGS)])], [+])])
	
#------------------------------------------------------------------------------
# Determine if memory tracker should be enabled.
#------------------------------------------------------------------------------
AC_MSG_CHECKING([whether to use memory tracker])
AC_ARG_ENABLE([memory-tracker],
    [AC_HELP_STRING([--enable-memory-tracker],
	[enable the memory tracker which checks memory usage per
	source file (default NO)])],
    [], [enable_memory_tracker=no])
AC_MSG_RESULT([$enable_memory_tracker])
CS_EMIT_BUILD_PROPERTY([MEMORY_TRACKER],
    [$enable_memory_tracker])
AS_IF([test $enable_memory_tracker = yes],
    [CS_HEADER_PROPERTY([CS_MEMORY_TRACKER])])

#------------------------------------------------------------------------------
# Determine if reference tracker should be enabled.
#------------------------------------------------------------------------------
AC_MSG_CHECKING([whether to use reference tracker])
AC_ARG_ENABLE([ref-tracker],
    [AC_HELP_STRING([--enable-ref-tracker],
        [enable the reference tracker which dumps information 
        about leaked references (default NO)])],
    [], [enable_ref_tracker=no])
AC_MSG_RESULT([$enable_ref_tracker])
CS_EMIT_BUILD_PROPERTY([REF_TRACKER],
    [$enable_ref_tracker])
AS_IF([test $enable_ref_tracker = yes],
    [CS_HEADER_PROPERTY([CS_REF_TRACKER])])	

#------------------------------------------------------------------------------
# Determine if crash reporter should be enabled.
#------------------------------------------------------------------------------
AC_MSG_CHECKING([whether to use crash reporter])
AC_ARG_ENABLE([reporter],
    [AC_HELP_STRING([--enable-reporter],
	[enable the crash reporter which uploads dumps on crash
	(default NO)])],
    [], [enable_reporter=no])
AC_MSG_RESULT([$enable_reporter])
AS_IF([test $enable_reporter = yes],
  [CS_EMIT_BUILD_PROPERTY([REPORTER], [yes])])
AS_IF([test $enable_reporter = yes],
    [CS_HEADER_PROPERTY([USE_BREAKPAD])])

#----------------------------------------------------------------------------
# Package configuration switches
#----------------------------------------------------------------------------
AC_ARG_ENABLE([debug],
    [AC_HELP_STRING([--enable-debug],
	[build with debugging information (default NO)])],
    [], [enable_debug=no])

AC_MSG_CHECKING([build mode])
AS_IF([test $enable_debug = yes], [build_mode=debug], [build_mode=optimize])
AC_MSG_RESULT([$build_mode])

CS_EMIT_BUILD_PROPERTY([MODE], [$build_mode])

#-----------------------------------------------------------------------------
# Emit install paths and package information.
#-----------------------------------------------------------------------------
CS_OUTPUT_INSTALLDIRS
CS_EMIT_PACKAGEINFO

#----------------------------------------------------------------------------
# Emit generated files.
#----------------------------------------------------------------------------
CS_JAMCONFIG_OUTPUT([Jamconfig])
AC_CONFIG_FILES([Jamfile])
AC_OUTPUT

#------------------------------------------------------------------------------
# `Result Summary.
#------------------------------------------------------------------------------
cs_summary=`CS_SUMMARIZE([$cs_host_family], [Found], [Missing])`
AC_MSG_NOTICE([
******************************************************************************
***                 Planeshift  --   Configuration Summary                 ***
******************************************************************************
Build mode: ${build_mode}
Version: ${PACKAGE_VERSION} (SVN: ${ps_cv_source_is_svn})

Optional Dependencies
---------------------
$cs_summary])

AC_MSG_NOTICE([

Please note that this project uses Jam as its build tool,
therefore you must type 'jam' to build the project.

Jam home: http://www.perforce.com/jam/jam.html
Jam source: ftp://ftp.perforce.com/jam/
])
