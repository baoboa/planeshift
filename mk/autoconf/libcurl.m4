dnl MY_CURL
dnl -------
dnl set my_cv_curl_vers to the version of libcurl or NONE
dnl if libcurl is not found or is too old
AC_DEFUN(PS_CHECK_LIBCURL,[
 AC_CACHE_VAL(my_cv_curl_vers,[
 my_cv_curl_vers=NONE
 dnl check is the plain-text version of the required version
 check="7.7.2"
 AC_MSG_CHECKING([for curl])
 if eval curl-config --version 2>/dev/null >/dev/null; then
   ver=`curl-config --version | sed -e "s/libcurl //g"`
   CURL_CFLAG=`curl-config --cflags $4`
   CURL_LIBS=`curl-config --libs $4`
   CURL_VERSION=`curl-config --version | sed -e "s/libcurl //g"`
   CS_JAMCONFIG_PROPERTY([CURL.AVAILABLE], [yes])
   CS_JAMCONFIG_PROPERTY([CURL.CFLAGS], [$CURL_CFLAG])
   CS_JAMCONFIG_PROPERTY([CURL.LFLAGS], [$CURL_LIBS])
   AC_MSG_RESULT([$CURL_VERSION])
 else
   AC_MSG_RESULT(no)
 fi
 ])
])

