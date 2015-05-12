#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.63])
AC_INIT([masa-core], [1.3.9.1024], [edans.sandes@gmail.com], [masa-core], [https://github.com/edanssandes/masa-core])
AC_CONFIG_AUX_DIR(admin)
AM_INIT_AUTOMAKE
m4_include([m4/ax_check_compile_flag.m4])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile])

AC_PROG_RANLIB


#
# Checks for programs.
#
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET




# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h stdlib.h string.h sys/socket.h sys/time.h unistd.h getopt.h errno.h wordexp.h],,
	AC_MSG_ERROR([Missing required header files.]))

	
	
	
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T




# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset mkdir socket strcasecmp])


OPTIMIZATION="-O3"


AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--enable-debug],[enable cpu debug]),
	[DEBUG="-ggdb"; OPTIMIZATION=""]
)



#default: 64bit
AC_ARG_ENABLE([32bit],
	AC_HELP_STRING([--enable-32bit],[force compilation in 32 bit mode.]),
	[ARCH="386"; PLATFORM="-m32";
		AC_MSG_NOTICE([Enabling 32bit compilation])
	],
	[ARCH="x86_64"; PLATFORM="-m64";
		AC_MSG_NOTICE([Enabling 64bit compilation])	
	]
)

#find out what version we are running
#ARCH=`uname -m`

#AC_CHECK_HEADERS([readline/readline.h])
#AC_CHECK_LIB([readline], [readline])

AX_CHECK_COMPILE_FLAG([-malign-double],	[COMMONFLAGS="$COMMONFLAGS -malign-double"],
	AC_MSG_WARN([Compiler does not support -malign-double parameter]))
AX_CHECK_COMPILE_FLAG([-fno-strict-aliasing],	[COMMONFLAGS="$COMMONFLAGS -fno-strict-aliasing"],
	AC_MSG_ERROR([Compiler does not support -fno-strict-aliasing]))
	
COMMONFLAGS="-DUNIX $COMMONFLAGS $PLATFORM $OPTIMIZATION"
CXXFLAGS="$DEBUG"

AC_SUBST(CXXFLAGS)
AC_SUBST(COMMONFLAGS)



AC_OUTPUT

