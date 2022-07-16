AC_INIT([libtta-cpp], [2.3], [info@true-audio.com])
AC_CANONICAL_TARGET

AC_CONFIG_SRCDIR([libtta.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Initialize ranlib
AC_PROG_RANLIB

# Common compiler flags
CXXFLAGS="-Wall -O2 -funroll-loops -fomit-frame-pointer"

# Determine CPU
AM_CONDITIONAL(CPU_X86, false)
AM_CONDITIONAL(CPU_ARM, false)
AM_CONDITIONAL(CPU_MIPS, false)

case "${host_cpu}" in
i?86*|x86_64*|amd64*)
    AM_CONDITIONAL(CPU_X86, true)
    AC_DEFINE(CPU_X86,, [Define if building for X86]) ;;
arm*)
    AM_CONDITIONAL(CPU_ARM, true)
    AC_DEFINE(CPU_ARM,, [Define if building for ARM]) ;;
mipsel*)
    AM_CONDITIONAL(CPU_MIPS, true)
    AC_DEFINE(CPU_MIPS,, [Define if building for MIPS])
    CXXFLAGS = "$CXXFLAGS -mips32r2 -mtune=24kf" ;;
*)
    AC_MSG_ERROR([CPU ${host_cpu} is not supported]) ;;
esac

# Checks for programs
AC_PROG_CXX
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_GCC_TRADITIONAL

# Checks for header files
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics
AC_SYS_LARGEFILE

# Checks for library functions
AC_CHECK_FUNCS([read write malloc memset memcpy lseek64 aligned_alloc])

# Specific optimization features
AM_CONDITIONAL(ENABLE_ASM, false)
AC_ARG_ENABLE(asm,
AC_HELP_STRING([--enable-asm], [enable assembly optimizations]),
[AC_DEFINE(ENABLE_ASM,, [Define to enable assembly optimizations])
    AM_CONDITIONAL(ENABLE_ASM, true)],)

AC_ARG_ENABLE(sse2,
AC_HELP_STRING([--enable-sse2], [build with SSE2 support]),
[AC_DEFINE(ENABLE_SSE2,, [Define to use SSE2 instructions])
    CXXFLAGS="-msse2 $CXXFLAGS"],)

AC_ARG_ENABLE(sse4,
AC_HELP_STRING([--enable-sse4], [build with SSE4 support]),
[AC_DEFINE(ENABLE_SSE4,, [Define to use SSE4 instructions])
    CXXFLAGS="-msse4 $CXXFLAGS"],)

if false; then
AC_DEFINE(ENABLE_FRW,, [Define to enable FRW optimization])
fi

AC_CONFIG_FILES(Makefile console/Makefile)
AC_OUTPUT
