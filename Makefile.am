ACLOCAL_AMFLAGS = -I m4 # use custom macro's in ./m4 
lib_LTLIBRARIES=
bin_PROGRAMS=
include_HEADERS=

# To Profile w/ gprof:
# 1) Modify configure: ./configure --disable-shared CFLAGS="-pg"
# 2) Run the program. ./foo
# 3) Run gprof /libtool --mode=execute gprof ./foo


# C compiler flags
# _GNU_SOURCE - turns on GNU specific extensions and gives correct prototype for double log2(double) 
# -Wall turn on all warnings
# -Wno-multichar - turns off multi-character constant warnings from cmAudioFile.c


AM_CPPFLAGS =   -I..  -I$(srcdir)/src/libcw
AM_CFLAGS   = -Wno-multichar 
AM_CXXFLAGS = -Wno-multichar -std=c++20 
AM_LDFLAGS  =  


# debug/release switches
if DEBUG
  AM_CFLAGS   += -g 
  AM_CXXFLAGS += -g -Wall -Wextra -Wno-unused  -fsanitize=undefined
else
  AM_CFLAGS   += -O3
  AM_CXXFLAGS += -O3
endif

# Linux specific
if OS_LINUX

if OS_64
  AM_LDFLAGS += -L/usr/lib64/atlas -L/usr/lib64
  AM_CFLAGS  += -m64
endif	

if cwALSA
  AM_LDFLAGS += -lasound #   -Wl,-Bstatic -lasound -Wl,-Bdynamic
endif

endif

if cwWEB

  AM_CXXFLAGS += --std=c++11

  FFT_DIR   = $(HOME)/src/fftw-3.3.8/build
  FFT_DIR_D   = $(FFT_DIR)/d
  FFT_DIR_S   = $(FFT_DIR)/s


  AM_CPPFLAGS += -I$(FFT_DIR)/include -I$(FFT_DIR_D)/include -I$(FFT_DIR_S)/include
  AM_LDFLAGS  += -L$(FFT_DIR)/lib     -L$(FFT_DIR_D)/lib     -L$(FFT_DIR_S)/lib
else
  AM_CXXFLAGS += --std=c++17
endif

include src/libcw/Makefile.am

#libcw_la_SOURCES  = $(libcwSRC) $(libcwHDR)
#lib_LTLIBRARIES  += libcw.la 
#include_HEADERS  += $(libcwHDR)

src_caw_caw_SOURCES   =  $(libcwHDR) $(libcwSRC) src/caw/main.cpp  src/caw/cawUi.cpp

# 1) autoconfig manual recommends setting direct referenes to non-3rd party libraries rather than using -L and -l
# 2) -ldl is required for dlopen(),dlclose() ...
# src_caw_caw_LDADD     = libcw.la  -lpthread -ldl

# note: libdl is the dynamic loader library
src_caw_caw_LDADD     =   -lpthread -ldl

if cwFFTW
  src_caw_caw_LDADD += -lfftw3 -lfftw3f
endif

if cwWEB
  src_caw_caw_LDADD += -lfftw3 -lfftw3f
endif

if cwWEBSOCK
  src_caw_caw_LDADD +=  -lwebsockets
endif


# src_caw_caw_CPPFLAGS  = -I$(srcdir)/src/libcw $(AM_CPPFLAGS)
bin_PROGRAMS               += src/caw/caw

# ${exec_prefix} is the install prefix given to 'configure' by the user.
# ${srcdir} is the directory of this Makefile and is set by autoconf.

# distclean-local sets the source tree back to it's minimal, pre-configure, state.
distclean-local:
	rm -rf ${exec_prefix}/src        
	rm -rf ${srcdir}/autom4te.cache
	rm -rf ${srcdir}/build-aux
	rm -f  ${srcdir}/m4/libtool.m4   ${srcdir}/m4/lt~obsolete.m4 ${srcdir}/m4/ltsugar.m4
	rm -f  ${srcdir}/m4/ltversion.m4 ${srcdir}/m4/ltoptions.m4
	rm -f  ${srcdir}/aclocal.m4      ${srcdir}/config.h.in ${srcdir}/config.h.in~
	rm -f  ${srcdir}/Makefile.in     ${srcdir}/configure
