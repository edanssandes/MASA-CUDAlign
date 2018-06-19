#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.63])
AC_INIT([MASA-CUDAlign], [3.9.1.1024], [edans.sandes@gmail.com], [masa-cudalign], [https://github.com/edanssandes/masa-cudalign])
AC_CONFIG_AUX_DIR(admin)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([libs/masa-core])

AC_PROG_RANLIB

LIBMASA_PATH="libs/masa-core"


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
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset mkdir pow socket strcasecmp])



# Cuda Options

OPTIMIZATION="-O3"


AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--enable-debug],[enable cpu debug]),
	[DEBUG="-ggdb"; OPTIMIZATION=""]
)

AC_ARG_ENABLE([cuda-debug],
	AC_HELP_STRING([--enable-cuda-debug],[enable cuda debug]),
	[NVCC_CUDA_DEBUG="--debug"; OPTIMIZATION=""]
)

AC_ARG_ENABLE([device-debug],
	AC_HELP_STRING([--enable-device-debug],[enable cuda debug]),
	[NVCC_CUDA_DEBUG="--debug --device-debug"; OPTIMIZATION=""]
)

AC_ARG_WITH([cuda-arch],
	AC_HELP_STRING([--with-cuda-arch=ARCH],[select one cuda architecture, e.g. sm_30]),
	[CUDA_ARCH="$withval"],
	[CUDA_ARCH="sm_30"]
)
NVCC_CUDA_ARCH="--gpu-architecture=$CUDA_ARCH"
AC_DEFINE_UNQUOTED([COMPILED_CUDA_ARCH], ["$CUDA_ARCH"], [architecture used in --gpu-architecture nvcc parameter])

AC_ARG_WITH([threads],
	AC_HELP_STRING([--with-threads=COUNT],[choose the number of cuda threads [default=128] (use power of 2)]),
	[CUDALIGN_THREADS="-DTHREADS_COUNT=$withval"],
	[CUDALIGN_THREADS="-DTHREADS_COUNT=128"]
)

AC_ARG_WITH([maxregcount],
	AC_HELP_STRING([--with-maxregcount=COUNT],[choose the maximum number of registers per threads [default=no]]),
	[CUDA_REGISTERS="--maxrregcount $withval"],
	[CUDA_REGISTERS=""]
)

#default: 64bit
AC_ARG_ENABLE([32bit],
	AC_HELP_STRING([--enable-32bit],[force compilation in 32 bit mode.]),
	[ARCH="386"; CUDA_LIB_DIR="lib"; PLATFORM="-m32";
		AC_MSG_NOTICE([Enabling 32bit compilation])
	],
	[ARCH="x86_64"; CUDA_LIB_DIR="lib64"; PLATFORM="-m64";
		AC_MSG_NOTICE([Enabling 64bit compilation])	
	]
)
#find out what version we are running
#ARCH=`uname -m`


# Setup CUDA paths
# ------------------------------------------------------------------------------
AC_ARG_WITH([cuda],
   [AC_HELP_STRING([--with-cuda=PATH],[path where cuda is installed (default=/usr/local/cuda)])],
   [CUDA_HOME=$withval],
   [CUDA_HOME=/usr/local/cuda])

AC_ARG_WITH([dummy],
	[AC_HELP_STRING([--without-cuda],[disable CUDA compilation])]
	)

AM_CONDITIONAL([COMPILE_CUDA], [test "x$CUDA_HOME" != "xno"])

AS_IF([test "x$CUDA_HOME" != "xno"], [

	CUDA_CFLAGS="-I$CUDA_HOME/include"
	CUDA_LIBS="-L$CUDA_HOME/$CUDA_LIB_DIR"
	CUDA_LDFLAGS="-L$CUDA_HOME/$CUDA_LIB_DIR"

	AC_CHECK_FILE($CUDA_HOME/$CUDA_LIB_DIR/libcudart.so, [],
		[AC_MSG_ERROR([libcudart.so library not found in path: $CUDA_HOME/$CUDA_LIB_DIR. Try running 
	                  configure again with the './configure --with-cuda=PATH' argument.])]
	)

	AC_ARG_ENABLE([compiler_bindir],
		AC_HELP_STRING([--enable-compiler-bindir=PATH],[force some g++ version to nvcc, e.g. /usr/bin/g++-4.4.]),
		[NVCC_COMPILER_BINDIR="--compiler-bindir=$enableval"; 
			AC_MSG_NOTICE([using nvcc parameter: $NVCC_COMPILER_BINDIR])
		],
		[AC_MSG_WARN([if the compilation fails with an unsupported compiler version, 
	                  try other g++ compiler version instead 
	                  (e.g.  ./configure --enable-compiler-bindir=/usr/bin/g++-4.4)])]
	)

	AC_ARG_WITH([nvcc],
	   [AC_HELP_STRING([--with-nvcc=PATH],[path where nvcc is installed (e.g. /usr/local/cuda/bin)])],
	   [NVCC_PATH=$withval],
	   [NVCC_PATH=$CUDA_HOME/bin]
	)
	AC_PATH_PROG([NVCC],[nvcc],[],[$NVCC_PATH])
	if test -z $NVCC; then
		AC_MSG_ERROR([CUDA nvcc compiler not found in path: $NVCC_PATH. Try running 
	                  configure again with the './configure --with-nvcc=PATH' argument.])
	fi
	
	#NVCCFLAGS="--compiler-bindir $NVCC_COMPILER_BINDIR $CUDA_CFLAGS"



#	AC_CHECK_LIB(readline, readline, [], [AC_MSG_ERROR(libreadline is required)])
#	AC_CHECK_HEADERS(readline/readline.h, [], [AC_MSG_ERROR(readline/readline.h (libreadline-dev) is required)])
	
	#
	# Check for CUDA libraries
	#
	save_LDFLAGS="$LDFLAGS"
	if [[ $ARCH == "386" ]]; then
		LDFLAGS="$LDFLAGS $CUDA_LDFLAGS -m32"
		echo $LDFLAGS
		
		AC_CHECK_LIB([z], [deflate],[],[AC_MSG_ERROR([
	                  libz.so:i386 error. Install the lib32z1-dev package or
	                  try running configure again with the './configure --enable-64bit' argument.
	                  You can also supply a ./configure LDFLAGS='-L<DIR>', where <DIR>
	                  is the directory where the 32bit version of libz.so resides.])]
		)
		
		AC_CHECK_LIB([stdc++], [memcpy],[],[AC_MSG_ERROR([
	                  libstdc++.so:i386 error. Install the g++-multilib package or
	                  try running configure again with the './configure --enable-64bit' argument.
	                  You can also supply a ./configure LDFLAGS='-L<DIR>', where <DIR>
	                  is the directory where the 32bit version of libstdc++.so resides.])]
		)
		
		AC_CHECK_LIB([cudart], [cudaMemcpy],[],[AC_MSG_ERROR([
	                  libcudart.so:i386 error. Install the CUDA 32bit package or
	                  try running configure again with the './configure --enable-64bit' argument.])]
		)
		
		AC_CHECK_LIB([cuda], [cuMemcpy],[],[AC_MSG_ERROR([
	                  libcuda.so:i386 error. Install the CUDA 32bit package or
	                  try running configure again with the './configure --enable-64bit' argument.
	                  You can also supply a ./configure LDFLAGS='-L<DIR>', where <DIR>
	                  is the directory where the 32bit version of libcuda.so resides.])]
		)
	else
		LDFLAGS="$LDFLAGS $CUDA_LDFLAGS"
		AC_CHECK_LIB([cudart], [cudaMemcpy],[],[AC_MSG_ERROR([
	                  libcudart.so (cudaMemcpy) was not found.])
		])
		
		AC_CHECK_LIB([cuda], [cuMemcpy],[],[AC_MSG_ERROR([
	                  libcuda.so (cuMemcpy) was not found.])
		])
	fi
	LDFLAGS="$save_LDFLAGS"
	
	
	save_CPPFLAGS="$LDFLAGS"
	CPPFLAGS+=$CUDA_CFLAGS
	# Checks for header files.
	AC_CHECK_HEADERS([cuda_runtime.h],,
		AC_MSG_ERROR([Missing required header files.]))
	CPPFLAGS="$save_CPPFLAGS"


	NVCCFLAGS="$CUDA_CFLAGS $NVCCFLAGS $NVCC_COMPILER_BINDIR $NVCC_CUDA_ARCH $NVCC_CUDA_DEBUG $CUDA_REGISTERS"

])


COMMONFLAGS="-DUNIX  $CUDALIGN_THREADS $COMMONFLAGS $PLATFORM $OPTIMIZATION -I$LIBMASA_PATH/src"
CXXFLAGS="$DEBUG"

AC_SUBST(CUDA_CFLAGS)
AC_SUBST(CUDA_LIBS)
AC_SUBST(NVCC)
AC_SUBST(NVCCFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(COMMONFLAGS)

AC_SUBST(LIBMASA_PATH)


AC_OUTPUT

