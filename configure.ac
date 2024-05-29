#
# Use "autoreconf --force --install" to update depedent files after changing 
# this configure.ac or any of the Makefile.am files.
#

AC_COPYRIGHT([Copyright (C) 2019-2024 Kevin Larke])
AC_INIT([caw],[1.0],[caw@larke.org])
AC_CONFIG_SRCDIR([src/caw/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])                    # put aux files in build-aux
AM_INIT_AUTOMAKE([1.9 -Wall foreign subdir-objects]) # subdir-objects needed for non-recursive make
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_PROG_AR

LT_INIT

# Check for programs
AC_PROG_CC
AC_PROG_CXX
# AC_PROG_RANLIB  # required for static librarires

AM_PROG_CC_C_O

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h mach/mach.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([clock_gettime floor memmove memset mkdir pow rint select socket sqrt strcasecmp strchr strcspn strerror strspn strstr strtol])

# Checks for libraries.
fftw=false
AC_CHECK_LIB([fftw3],[fftw_malloc],[fftw=true],[])
AM_CONDITIONAL([cwFFTW],[test x"${fftw}" = xtrue])

if test x"$fftw" = xtrue; then
AC_DEFINE([cwFFTW], 1,[Use libfftw3.])
fi

echo "fftw='${fftw}'"

# The following is a custom macro in ./m4/os_type.m4
# be sure to also set "ACLOCAL_AMFLAGS = -I m4" in ./Makefile.am
# Defines the config.h variable OS_LINUX or OS_OSX
AX_FUNC_OS_TYPE

AX_FUNC_OS_64

# ac_cv_os_type is set by AX_FUNC_OS_TYPE
AM_CONDITIONAL([OS_LINUX],[test x"${ax_cv_os_type}" == xLinux])
AM_CONDITIONAL([OS_OSX],[test x"${ax_cv_os_type}" == xDarwin])
echo "OS='${ax_cv_os_type}'"

AM_CONDITIONAL([OS_64],[test x"${ax_cv_os_64}" == xx86_64])
echo "ptr width='${ax_cv_os_64}'"

#
# Enable Debug
#

AC_ARG_ENABLE([debug],
     [  --enable-debug    Turn on debugging],
     [case "${enableval}" in
       yes) debug=true ;;
       no)  debug=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
     esac],[debug=false])
     
echo "debug=${debug}"
     
AM_CONDITIONAL([DEBUG], [test x$debug == xtrue])

if test x$debug = xfalse; then
AC_DEFINE([NDEBUG], 1,[Debugging off.])
fi


#
# Enable libwebsock
#
AC_ARG_ENABLE([websock],
     [  --enable-websock    Include websock dependencies],
     [case "${enableval}" in
       yes) websock=true ;;
       no)  websock=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-websock]) ;;
     esac],[websock=false])

if test x"$websock" = xtrue; then
  AC_CHECK_LIB([websockets],[lws_get_library_version],[AC_MSG_RESULT([The 'websockets' library was found.])],[AC_MSG_ERROR([The 'websockets' library was not found.])])
  AC_DEFINE([cwWEBSOCK], 1,[Use libwebsock.])
fi
AM_CONDITIONAL([cwWEBSOCK], [test x$websock == xtrue])
echo "websock=${websock}"


#
# Enable ALSA
#
AC_ARG_ENABLE([alsa],
     [  --enable-alsa    Include ALSA dependencies],
     [case "${enableval}" in
       yes) alsa=true ;;
       no)  alsa=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-alsa]) ;;
     esac],[alsa=false])

	
if test x"$alsa" = xtrue; then
AC_CHECK_LIB([asound],[snd_asoundlib_version],[AC_MSG_RESULT([The 'ALSA' library was found.])],[AC_MSG_ERROR([The 'ALSA' library was not found.])])
AC_DEFINE([cwALSA], 1,[Use libalsa.])
fi

AM_CONDITIONAL([cwALSA], [test x$alsa == xtrue])

echo "alsa=${alsa}"

#
# Enable Web
#

AC_ARG_ENABLE([web],
     [  --enable-web    Include WEB dependencies],
     [case "${enableval}" in
       yes) web=true ;;
       no)  web=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-web]) ;;
     esac],[web=false])

	
if test x"$web" = xtrue; then
AC_DEFINE([cwWEB], 1,[Use libweb.])
fi

AM_CONDITIONAL([cwWEB], [test x$web == xtrue])

echo "web=${web}"




AC_CONFIG_FILES([ Makefile ])
AC_OUTPUT
