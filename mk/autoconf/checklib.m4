# checklib.m4                                                  -*- Autoconf -*-
#==============================================================================
# Copyright (C)2003-2006 by Eric Sunshine <sunshine@sunshineco.com>
#
#    This library is free software; you can redistribute it and/or modify it
#    under the terms of the GNU Library General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or (at your
#    option) any later version.
#
#    This library is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
#    License for more details.
#
#    You should have received a copy of the GNU Library General Public License
#    along with this library; if not, write to the Free Software Foundation,
#    Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#==============================================================================
AC_PREREQ([2.56])

#------------------------------------------------------------------------------
# cs_lib_paths_default
#	Whitespace delimited list of directory tuples in which to search, by
#	default, for external libraries.  Each list item can specify an
#	include|library directory tuple (for example, "/usr/include|/usr/lib"),
#	or a single directory (for example, "/usr").  If the second form is
#	used, then "include" and "lib" subdirectories of the directory are
#	searched.  If the library resources are not found, then the directory
#	itself is searched.  Thus, "/proj" is shorthand for
#	"/proj/include|/proj/lib /proj|/proj".
#
# Present Cases:
#	/usr/local -- Not all compilers search here by default, so we specify
#		it manually.
#	/sw -- Fink, the MacOS/X manager of Unix packages, installs here by
#		default.
#	/opt/local -- DarwinPorts installs here by default.
#------------------------------------------------------------------------------
m4_define([cs_lib_paths_default],
    [/usr/local/include|/usr/local/lib \
    /sw/include|/sw/lib \
    /opt/local/include|/opt/local/lib \
    /opt/include|/opt/lib])



#------------------------------------------------------------------------------
# cs_pkg_paths_default
#	Comma delimited list of additional directories in which the
#	`pkg-config' command should search for its `.pc' files.
#
# Present Cases:
#	/usr/local/lib/pkgconfig -- Although a common location for .pc files
#		installed by "make install", many `pkg-config' commands neglect
#		to search here automatically.
#	/sw/lib/pkgconfig -- Fink, the MacOS/X manager of Unix packages,
#		installs .pc files here by default.
#	/opt/local/lib/pkgconfig -- DarwinPorts installs .pc files here by
#		default.
#------------------------------------------------------------------------------
m4_define([cs_pkg_paths_default],
    [/usr/local/lib/pkgconfig,
    /sw/lib/pkgconfig,
    /opt/local/lib/pkgconfig,
    /opt/lib/pkgconfig])



#------------------------------------------------------------------------------
# CS_CHECK_LIB_WITH(LIBRARY, PROGRAM, [SEARCH-LIST], [LANGUAGE],
#                   [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [OTHER-CFLAGS],
#                   [OTHER-LFLAGS], [OTHER-LIBS], [ALIASES])
#	Very roughly similar in concept to AC_CHECK_LIB(), but allows caller to
#	to provide list of directories in which to search for LIBRARY; allows
#	user to augment the library search path via --with-LIBRARY=dir; and
#       consults `pkg-config' (if present) and `LIBRARY-config' (if present,
#       i.e. `sdl-config') in order to obtain compiler and linker flags.
#       LIBRARY is the name of the library or MacOS/X framework which is to be 
#       located	(for example, "readline" for `libreadline.a' or 
#       `readline.framework').
#	PROGRAM, which is typically composed with AC_LANG_PROGRAM(), is a
#	program which references at least one function or symbol in LIBRARY.
#	SEARCH-LIST is a whitespace-delimited list of paths in which to search
#	for the library and its header files, in addition to those searched by
#	the compiler and linker by default, and those referenced by the
#	cs_lib_paths_default macro.  Each list item can specify an
#	`include|library' directory tuple (for example,
#	"/usr/include|/usr/lib"), or a single directory (for example, "/usr").
#	If the second form is used, then "include" and "lib" subdirectories of
#	the directory are searched.  If the library resources are not found,
#	then the directory itself is searched.  Thus, "/proj" is shorthand for
#	"/proj/include|/proj/lib /proj|/proj".  Items in the search list can
#	include wildcards.  SEARCH-LIST can be overridden by the user with the
#	--with-LIBRARY=dir option, in which case only "dir/include|dir/lib" and
#	"dir|dir" are searched.  If SEARCH-LIST is omitted and the user did not
#	override the search list via --with-LIBRARY=dir, then only the
#	directories normally searched by the compiler and the directories
#	mentioned via cs_lib_paths_default are searched.  LANGUAGE is typically
#	either C or C++ and specifies which compiler to use for the test.  If
#	LANGUAGE is omitted, C is used.  OTHER-CFLAGS, OTHER-LFLAGS, and
#	OTHER-LIBS can specify additional compiler flags, linker flags, and
#	libraries needed to successfully link with LIBRARY.  The optional
#	ALIASES is a comma-delimited list of library names for which to search
#	in case LIBRARY is not located (for example "[sdl1.2, sdl12]" for
#	libsdl1.2.a, sdl1.2.framework, libsdl12.a, and sdl12.framework).  If
#	the library or one of its aliases is found and can be successfully
#	linked into a program, then the shell cache variable cs_cv_libLIBRARY
#	is set to "yes"; cs_cv_libLIBRARY_cflags, cs_cv_libLIBRARY_lflags, and
#	cs_cv_libLIBRARY_libs are set, respectively, to the compiler flags
#	(including OTHER-CFLAGS), linker flags (including OTHER-LFLAGS), and
#	library references (including OTHER-LIBS) which resulted in a
#	successful build; and ACTION-IF-FOUND is invoked.  If the library was
#	not found or was unlinkable, or if the user disabled the library via
#	--without-LIBRARY, then cs_cv_libLIBRARY is set to "no" and
#	ACTION-IF-NOT-FOUND is invoked.  Note that the exported shell variable
#	names are always composed from LIBRARY regardless of whether the test
#	succeeded because the primary library was discovered or one of the
#	aliases.
#------------------------------------------------------------------------------
AC_DEFUN([CS_CHECK_LIB_WITH],
    [AC_ARG_WITH([$1], [AC_HELP_STRING([--with-$1=dir],
	[specify additional location to search lib$1 if not detected automatically; 
	directories searched include dir/include, dir/lib, and dir])])

    # Backward compatibility: Recognize --with-lib$1 as alias for --with-$1.
    AS_IF([test -n "$_CS_CLW_WITH_DEPRECATED([$1])" &&
           test -z "$_CS_CLW_WITH([$1])"],
	[_CS_CLW_WITH([$1])="$_CS_CLW_WITH_DEPRECATED([$1])"])

    AS_IF([test -z "$_CS_CLW_WITH([$1])"], [_CS_CLW_WITH([$1])=yes])
    AS_IF([test "$_CS_CLW_WITH([$1])" != no],
	[
	# If --with-$1 value is same as cached value, then assume other
	 # cached values are also valid; otherwise, ignore all cached values.
	AS_IF([test "$_CS_CLW_WITH([$1])" != "$_CS_CLW_WITH_CVAR([$1])"],
	    [cs_ignore_cache=yes], [cs_ignore_cache=no])

	# Put relevant subdirs of the library directory into PATH and PKGCONFIG
	PATH_SAVE="$PATH"
	AS_IF([test $_CS_CLW_WITH([$1]) != yes],
	    [PATH="$_CS_CLW_WITH([$1])/bin$PATH_SEPARATOR$PATH"
	    _CS_CHECK_PKG_CONFIG_PREPARE_PATH([$_CS_CLW_WITH([$1]),
		$_CS_CLW_WITH([$1])/lib/pkgconfig])])
	    
	cs_check_lib_flags=''
	m4_foreach([cs_check_lib_alias], 
	    [m4_ifval([$10], [$1, $10], [$1])],
	    [_CS_CHECK_LIB_PKG_CONFIG_FLAGS([cs_check_lib_flags],
		cs_check_lib_alias)
	    _CS_CHECK_LIB_CONFIG_FLAGS([cs_check_lib_flags],
		cs_check_lib_alias)
	    ])

	AS_IF([test $_CS_CLW_WITH([$1]) != yes],
	    [cs_check_lib_paths=$_CS_CLW_WITH([$1])],
	    [cs_check_lib_paths="| cs_lib_paths_default $3"])
	m4_foreach([cs_check_lib_alias], 
	    [m4_ifval([$10], [$1, $10], [$1])],
	    [_CS_CHECK_LIB_CREATE_FLAGS([cs_check_lib_flags],
		cs_check_lib_alias, [$cs_check_lib_paths], [$1])
	    ])

	CS_CHECK_BUILD([for lib$1], [_CS_CLW_LIB_CVAR([$1])], [$2],
            [$cs_check_lib_flags], [$4], [], [], [$cs_ignore_cache],
            [$7], [$8], [$9])
	    
	PATH="$PATH_SAVE"
	AS_IF([test $_CS_CLW_WITH([$1]) != yes],
	    [_CS_CHECK_PKG_CONFIG_UNPREPARE_PATH])
	],
	[_CS_CLW_LIB_CVAR([$1])=no])

    _CS_CLW_WITH_CVAR([$1])="$_CS_CLW_WITH([$1])"
    AS_IF([test "$_CS_CLW_LIB_CVAR([$1])" = yes], [$5], [$6])])

AC_DEFUN([_CS_CLW_WITH], [AS_TR_SH([with_$1])])
AC_DEFUN([_CS_CLW_WITH_DEPRECATED], [AS_TR_SH([with_lib$1])])
AC_DEFUN([_CS_CLW_WITH_CVAR], [AS_TR_SH([cs_cv_with_$1])])
AC_DEFUN([_CS_CLW_LIB_CVAR], [AS_TR_SH([cs_cv_lib$1])])



#------------------------------------------------------------------------------
# CS_CHECK_PKG_CONFIG
#	Check if the `pkg-config' command is available and reasonably recent.
#	This program acts as a central repository of build flags for various
#	packages.  For example, to determine the compiler flags for FreeType2
#	use, "pkg-config --cflags freetype2"; and "pkg-config --libs freetype2"
#	to determine the linker flags. If `pkg-config' is found and is
#	sufficiently recent, PKG_CONFIG is set and AC_SUBST() invoked.
#------------------------------------------------------------------------------
m4_define([CS_PKG_CONFIG_MIN], [0.9.0])
AC_DEFUN([CS_CHECK_PKG_CONFIG],
    [AS_IF([test "$cs_prog_pkg_config_checked" != yes],
	[CS_CHECK_TOOLS([PKG_CONFIG], [pkg-config])
	cs_prog_pkg_config_checked=yes])
    AS_IF([test -z "$cs_cv_prog_pkg_config_ok"],
	[AS_IF([test -n "$PKG_CONFIG"],
	    [AS_IF([$PKG_CONFIG --atleast-pkgconfig-version=CS_PKG_CONFIG_MIN],
		[cs_cv_prog_pkg_config_ok=yes],
		[cs_cv_prog_pkg_config_ok=no])],
	    [cs_cv_prog_pkg_config_ok=no])])])

AC_DEFUN([_CS_CHECK_PKG_CONFIG_PREPARE_PATH],
    [PKG_CONFIG_PATH_SAVE_1="$PKG_CONFIG_PATH"
    PKG_CONFIG_PATH="m4_foreach([cs_pkg_path], [cs_pkg_paths_default],
	[cs_pkg_path$PATH_SEPARATOR])$PKG_CONFIG_PATH"
    PKG_CONFIG_PATH="m4_foreach([cs_pkg_path], [$1], 
	[cs_pkg_path$PATH_SEPARATOR])$PKG_CONFIG_PATH"
    export PKG_CONFIG_PATH])
AC_DEFUN([_CS_CHECK_PKG_CONFIG_UNPREPARE_PATH],
    [PKG_CONFIG_PATH="$PKG_CONFIG_PATH_SAVE_1"
    export PKG_CONFIG_PATH])


#------------------------------------------------------------------------------
# _CS_CHECK_LIB_PKG_CONFIG_FLAGS(VARIABLE, LIBRARY)
#	Helper macro for CS_CHECK_LIB_WITH().  Checks if `pkg-config' knows
#	about LIBRARY and, if so, appends a build tuple consisting of the
#	compiler and linker flags reported by `pkg-config' to the list of
#	tuples stored in the shell variable VARIABLE.
#------------------------------------------------------------------------------
AC_DEFUN([_CS_CHECK_LIB_PKG_CONFIG_FLAGS],
    [CS_CHECK_PKG_CONFIG
    AS_IF([test $cs_cv_prog_pkg_config_ok = yes],
	[AC_CACHE_CHECK([if $PKG_CONFIG recognizes $2], [_CS_CLPCF_CVAR([$2])],
	    [AS_IF([$PKG_CONFIG --exists $2],
		[_CS_CLPCF_CVAR([$2])=yes], [_CS_CLPCF_CVAR([$2])=no])])
	AS_IF([test $_CS_CLPCF_CVAR([$2]) = yes],
	    [_CS_CHECK_LIB_CONFIG_PROG_FLAGS([$1], [pkg_config_$2],
		[$PKG_CONFIG], [$2])])])])

AC_DEFUN([_CS_CLPCF_CVAR], [AS_TR_SH([cs_cv_prog_pkg_config_$1])])



#------------------------------------------------------------------------------
# _CS_CHECK_LIB_CONFIG_FLAGS(VARIABLE, LIBRARY)
#	Helper macro for CS_CHECK_LIB_WITH().  Checks if `LIBRARY-config'
#	(i.e. `sdl-config') exists and, if so, appends a build tuple consisting
#	of the compiler and linker flags reported by `LIBRARY-config' to the
#	list of tuples stored in the shell variable VARIABLE.
#------------------------------------------------------------------------------
AC_DEFUN([_CS_CHECK_LIB_CONFIG_FLAGS],
    [CS_PATH_TOOL(_CS_CLCF_SHVAR([$2]), [$2-config])
    AS_IF([test -n "$_CS_CLCF_SHVAR([$2])"],
	[AS_IF([test -z "$_CS_CLCF_CVAR([$2])"],
	    [AS_IF([$_CS_CLCF_SHVAR([$2]) --cflags --libs >/dev/null 2>&1],
		[_CS_CLCF_CVAR([$2])=yes], [_CS_CLCF_CVAR([$2])=no])])
	AS_IF([test $_CS_CLCF_CVAR([$2]) = yes],
	    [_CS_CHECK_LIB_CONFIG_PROG_FLAGS([$1], [config_$2],
		[$_CS_CLCF_SHVAR([$2])])])])])

AC_DEFUN([_CS_CLCF_CVAR], [AS_TR_SH([cs_cv_prog_config_$1_ok])])
AC_DEFUN([_CS_CLCF_SHVAR], [m4_toupper(AS_TR_SH([CONFIG_$1]))])



#------------------------------------------------------------------------------
# _CS_CHECK_LIB_CONFIG_PROG_FLAGS(VARIABLE, TAG, CONFIG-PROGRAM, [ARGS])
#	Helper macro for _CS_CHECK_LIB_PKG_CONFIG_FLAGS() and
#	_CS_CHECK_LIB_CONFIG_FLAGS(). CONFIG-PROGRAM is a command which
#	responds to the --cflags and --libs options and returns suitable
#	compiler and linker flags for some package. ARGS, if supplied, is
#	passed to CONFIG-PROGRAM after the --cflags or --libs argument. The
#	results of the --cflags and --libs options are packed into a build
#	tuple and appended to the list of tuples stored in the shell variable
#	VARIABLE. TAG is used to compose the name of the cache variable. A good
#	choice for TAG is some unique combination of the library name and
#	configuration program.
#------------------------------------------------------------------------------
AC_DEFUN([_CS_CHECK_LIB_CONFIG_PROG_FLAGS],
    [AS_IF([test -z "$_CS_CLCPF_CVAR([$2])"],
	[cs_check_lib_cflag=CS_RUN_PATH_NORMALIZE([$3 --cflags $4])
	cs_check_lib_lflag=''
	cs_check_lib_libs=CS_RUN_PATH_NORMALIZE([$3 --libs $4])
	_CS_CLCPF_CVAR([$2])=CS_CREATE_TUPLE(
	    [$cs_check_lib_cflag],
	    [$cs_check_lib_lflag],
	    [$cs_check_lib_libs])])
    $1="$$1 $_CS_CLCPF_CVAR([$2])"])

AC_DEFUN([_CS_CLCPF_CVAR], [AS_TR_SH([cs_cv_prog_$1_flags])])



#------------------------------------------------------------------------------
# _CS_CHECK_LIB_CREATE_FLAGS(VARIABLE, LIBRARY, PATHS, UNALIASEDLIB)
#	Helper macro for CS_CHECK_LIB_WITH().  Constructs a list of build
#	tuples suitable for CS_CHECK_BUILD() and appends the tuple list to the
#	shell variable VARIABLE.  LIBRARY and PATHS have the same meanings as
#	the like-named arguments of CS_CHECK_LIB_WITH().
#------------------------------------------------------------------------------
AC_DEFUN([_CS_CHECK_LIB_CREATE_FLAGS],
    [for cs_lib_item in $3
    do
	case $cs_lib_item in
	    *\|*) CS_SPLIT(
		    [$cs_lib_item], [cs_check_incdir,cs_check_libdir], [|])
		_CS_CHECK_LIB_CREATE_FLAG([$1],
		    [$cs_check_incdir], [$cs_check_libdir], [$2], [$4])
		;;
	    *)  _CS_CHECK_LIB_CREATE_FLAG([$1],
		    [$cs_lib_item/include], [$cs_lib_item/lib], [$2], [$4])
		_CS_CHECK_LIB_CREATE_FLAG(
		    [$1], [$cs_lib_item], [$cs_lib_item], [$2], [$4])
		;;
	esac
    done])



#------------------------------------------------------------------------------
# _CS_CHECK_LIB_CREATE_FLAG(VARIABLE, HEADER-DIR, LIBRARY-DIR, LIBRARY,
#                           UNALIASEDLIB)
#	Helper macro for _CS_CHECK_LIB_CREATE_FLAGS().  Constructs build tuples
#	suitable for CS_CHECK_BUILD() for given header and library directories,
#	and appends the tuples to the shell variable VARIABLE. Synthesizes
#	tuples which check for LIBRARY as a MacOS/X framework, and a standard
#	link library. For MacOS/X framework, also #defines a preprocessor macro
#	CS_{UNALIASED_LIBRARY}_PATH with value LIBRARY, where UNALIASED_LIBRARY
#	is the primary name by which the library is known, even if searching
#	for one of its aliases. This macro is useful when the framework name
#	differs from the typical directory name in which header files reside
#	for a particular package. As an example, on most platforms, OpenGL
#	headers normally are #included as <GL/foo.h>, but the MacOS/X framework
#	is named OpenGL.framework, which requires #include <OpenGL/foo.h>.
#	Clients can take this difference into account by checking if
#	CS_{UNALIASED_LIBRARY}_PATH is #defined. If so, compose an #include
#	using its value, which will be the name of the framework. If not,
#	#include the header from its normal non-framework location.
#------------------------------------------------------------------------------
AC_DEFUN([_CS_CHECK_LIB_CREATE_FLAG],
   [AS_IF([test -n "$2"], [cs_check_lib_cflag="-I$2"], [cs_check_lib_cflag=''])
    AS_IF([test -n "$3"], [cs_check_lib_lflag="-L$3"], [cs_check_lib_lflag=''])
    AS_IF([test -n "$4"],
	[cs_check_lib_libs="-l$4"
	cs_check_lib_framework="-framework $4"
	cs_check_lib_framework_define="[-DCS_]AS_TR_CPP([$5])_PATH=$4"],
	[cs_check_lib_libs=''
	cs_check_lib_framework=''
	cs_check_lib_framework_define=''])
    $1="$$1
	CS_CREATE_TUPLE(
	    [$cs_check_lib_cflag $cs_check_lib_framework_define],
	    [$cs_check_lib_lflag],
	    [$cs_check_lib_framework])
	CS_CREATE_TUPLE(
	    [$cs_check_lib_cflag],
	    [$cs_check_lib_lflag],
	    [$cs_check_lib_libs])"])
