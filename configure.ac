#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Order is largely irrevellant, although it must start with AC_INIT and end with AC_OUTPUT
# See http://autotoolset.sourceforge.net/tutorial.html
# and http://www.openismus.com/documents/linux/automake/automake.shtml

AC_INIT(BLOOM, 1.4.7, bugs@afflib.org)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile java/Makefile])
AC_CONFIG_FILES([bloom.spec])

AC_CONFIG_HEADERS([src/config.h])

# Better safe than sorry
CPPFLAGS="-D__USE_FILE_OFFSET64 -D__USE_LARGEFILE64 -D_FILE_OFFSET_BITS=64 $CPPFLAGS"
CFLAGS="-D_FORTIFY_SOURCE=2 -Wall -O2 -g $CFLAGS"
CXXFLAGS="-D_FORTIFY_SOURCE=2 -Wall -O2 -g $CXXFLAGS"

# Programs that we will be using
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL


# Try for Java?
AM_PROG_GCJ

# Bring additional directories where things might be found into our
# search path. I don't know why autoconf doesn't do this by default
for spfx in /usr/local /opt/local /sw ; do
    echo checking ${spfx}/include
    if test -d ${spfx}/include; then
        CPPFLAGS="-I${spfx}/include $CPPFLAGS"
        LDFLAGS="-L${spfx}/lib $LDFLAGS"
    fi
    echo checking ${spfx}/lib/mysql
    if test -d ${spfx}/lib/mysql; then
        LDFLAGS="-L${spfx}/lib/mysql $LDFLAGS"
        CPPFLAGS="-I${spfx}/include $CPPFLAGS"
    fi
done

################################################################
### Bloom support
AC_TYPE_INT64_T
AC_CHECK_HEADERS([openssl/hmac.h openssl/pem.h])
AC_CHECK_FUNCS([printf getrusage err errx warn warnx mmap srandomdev])
AC_CHECK_HEADERS([err.h sys/mman.h sys/resource.h unistd.h sys/resource.h])

### for bloom_test
AC_CHECK_FUNCS([ftello ftello64 ftell64])

### 
AC_CHECK_FUNCS([ishexnumber])


################################################################
### pthread support 
###
# AC_CHECK_HEADERS([pthread.h])
# m4_include([ax_pthread.m4])
# AX_PTHREAD()
# LIBS="$PTHREAD_LIBS $LIBS"
# CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
# CPPFLAGS="$CPPFLAGS $PTHREAD_CFLAGS"
# CC="$PTHREAD_CC"
################################################################

AC_HEADER_STDC
AC_CHECK_FUNCS([printf getrusage atoi64])
AC_CHECK_HEADERS([err.h inttypes.h sys/disk.h sys/mman.h sys/resource.h unistd.h openssl/hmac.h openssl/pem.h sys/utsname.h regex.h])
AC_CHECK_LIB([mysqlclient],[mysql_real_connect],,AC_MSG_WARN([mysql client not found]))
AC_CHECK_LIB([ssl],[ssl3_write],,AC_MSG_WARN([OpenSSL developer library 'libssl-dev' may not be installed...]))
AC_CHECK_LIB([crypto],[MD5_Update],,AC_MSG_ERROR([OpenSSL developer library 'libcrypto' not installed; cannot continue.]))
AC_CHECK_LIB([afflib],[af_open])
AC_CHECK_LIB([regex],[regcomp])		# see if we need -lregex
AC_CHECK_LIB([m],[exp])			# see if we need -lm

AC_CHECK_HEADER([expat.h])
AC_CHECK_LIB([expat],[XML_ParserCreate])



############## drop optimization flags ################i
# Special features that can be enabled or disabled
AC_ARG_WITH([noopt], AC_HELP_STRING([--with-noopt],[Drop -O C flags]))
if test x"${AFF_NOOPT}" != "x" ; then
   with_noopt="yes";
fi

if test "${with_noopt}" = "yes" ; then
  CFLAGS=`echo "$CFLAGS" | sed s/-O[[0-9]]//`             # note the double quoting! 
  CXXFLAGS=`echo "$CXXFLAGS" | sed s/-O[[0-9]]//`
fi
################################################################
# I am a glutten for punishment and this is security-critical software
CFLAGS="$CFLAGS -D_FORTIFY_SOURCE=2 -Wall"
CXXFLAGS="$CXXFLAGS -D_FORTIFY_SOURCE=2 -Wall"

AC_OUTPUT

