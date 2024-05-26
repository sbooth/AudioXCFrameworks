/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Target processor is big endian. */
#if defined __BIG_ENDIAN__
#define CPU_IS_BIG_ENDIAN 1
#else
#define CPU_IS_BIG_ENDIAN 0
#endif

/* Target processor is little endian. */
#if defined __LITTLE_ENDIAN__
#define CPU_IS_LITTLE_ENDIAN 1
#else
#define CPU_IS_LITTLE_ENDIAN 0
#endif

/* Set FLAC__BYTES_PER_WORD to 8 (this is the default) */
#define ENABLE_64_BIT_WORDS 1

/* libtool defines DLL_EXPORT for windows dll builds,
   but flac code relies on FLAC_API_EXPORTS instead. */
#ifdef DLL_EXPORT
#ifdef __cplusplus
# define FLACPP_API_EXPORTS
#else
# define FLAC_API_EXPORTS
#endif
#endif

/* define to align allocated memory on 32-byte boundaries */
#define FLAC__ALIGN_MALLOC_DATA 1

/* define if building for ARM */
#if __aarch64__
#define FLAC__CPU_ARM64 1
#endif

/* define if building for ia32/i386 */
/* #undef FLAC__CPU_IA32 */

/* define if building for x86_64 */
#if __x86_64__
#define FLAC__CPU_X86_64 1
#endif

/* Set to 1 if <arm_neon.h> has A64 instructions. */
#if __aarch64__
#define FLAC__HAS_A64NEONINTRIN 1
#endif

/* Set to 1 if <arm_neon.h> is available. */
#if __aarch64__
#define FLAC__HAS_NEONINTRIN 1
#endif

/* define if you have the ogg library */
#define FLAC__HAS_OGG 1

/* define if you have pandoc */
/* #undef FLAC__HAS_PANDOC */

/* Set to 1 if <x86intrin.h> is available. */
#if __x86_64__
#define FLAC__HAS_X86INTRIN 1
#endif

/* define to disable use of assembly code */
/* #undef FLAC__NO_ASM */

/* define if building for Darwin / MacOS X */
#define FLAC__SYS_DARWIN 1

/* define if building for Linux */
/* #undef FLAC__SYS_LINUX */

/* define to enable use of AVX instructions */
#define FLAC__USE_AVX 1

/* "Define to the commit date of the current git HEAD" */
/* #undef GIT_COMMIT_DATE */

/* "Define to the short hash of the current git HEAD" */
/* #undef GIT_COMMIT_HASH */

/* "Define to the tag of the current git HEAD" */
/* #undef GIT_COMMIT_TAG */

/* Define to 1 if you have the <arm_neon.h> header file. */
#if __aarch64__
#define HAVE_ARM_NEON_H 1
#endif

/* Compiler has the __builtin_bswap16 intrinsic */
#define HAVE_BSWAP16 1

/* Compiler has the __builtin_bswap32 intrinsic */
#define HAVE_BSWAP32 1

/* Define to 1 if you have the <byteswap.h> header file. */
/* #undef HAVE_BYTESWAP_H */

/* define if you have clock_gettime */
/* #undef HAVE_CLOCK_GETTIME */

/* Define to 1 if you have the <cpuid.h> header file. */
#if __x86_64__
#define HAVE_CPUID_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Define to 1 if you have the `getauxval' function. */
/* #undef HAVE_GETAUXVAL */

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
#define HAVE_LANGINFO_CODESET 1

/* lround support */
#define HAVE_LROUND 1

/* Define to 1 if you have the <minix/config.h> header file. */
/* #undef HAVE_MINIX_CONFIG_H */

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/auxv.h> header file. */
/* #undef HAVE_SYS_AUXV_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if typeof works with your compiler. */
#define HAVE_TYPEOF 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the <x86intrin.h> header file. */
#if __x86_64__
#define HAVE_X86INTRIN_H 1
#endif

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST 

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Define if debugging is disabled */
#define NDEBUG /**/

/* Name of package */
#define PACKAGE "flac"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "flac-dev@xiph.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "flac"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "flac 1.4.3"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "flac"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://www.xiph.org/flac/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.4.3"

/* The size of `off_t', as computed by sizeof. */
#define SIZEOF_OFF_T 8

/* The size of `void*', as computed by sizeof. */
#define SIZEOF_VOIDP 8

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
# define _DARWIN_C_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable X/Open compliant socket functions that do not require linking
   with -lxnet on HP-UX 11.11.  */
#ifndef _HPUX_ALT_XOPEN_SOCKET_API
# define _HPUX_ALT_XOPEN_SOCKET_API 1
#endif
/* Identify the host operating system as Minix.
   This macro does not affect the system headers' behavior.
   A future release of Autoconf may stop defining this macro.  */
#ifndef _MINIX
/* # undef _MINIX */
#endif
/* Enable general extensions on NetBSD.
   Enable NetBSD compatibility extensions on Minix.  */
#ifndef _NETBSD_SOURCE
# define _NETBSD_SOURCE 1
#endif
/* Enable OpenBSD compatibility extensions on NetBSD.
   Oddly enough, this does nothing on OpenBSD.  */
#ifndef _OPENBSD_SOURCE
# define _OPENBSD_SOURCE 1
#endif
/* Define to 1 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_SOURCE
/* # undef _POSIX_SOURCE */
#endif
/* Define to 2 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_1_SOURCE
/* # undef _POSIX_1_SOURCE */
#endif
/* Enable POSIX-compatible threading on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
# define __STDC_WANT_IEC_60559_ATTRIBS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
# define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
# define __STDC_WANT_IEC_60559_DFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
# define __STDC_WANT_IEC_60559_FUNCS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
# define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
# define __STDC_WANT_LIB_EXT2__ 1
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
# define __STDC_WANT_MATH_SPEC_FUNCS__ 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable X/Open extensions.  Define to 500 only if necessary
   to make mbstate_t available.  */
#ifndef _XOPEN_SOURCE
/* # undef _XOPEN_SOURCE */
#endif


/* Version number of package */
#define VERSION "1.4.3"

/* Target processor is big endian. */
#define WORDS_BIGENDIAN 0

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to __typeof__ if your compiler spells it that way. */
/* #undef typeof */
