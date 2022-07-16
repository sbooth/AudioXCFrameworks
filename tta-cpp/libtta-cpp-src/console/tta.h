/*
 * tta.h
 *
 * Description: TTA general portability definitions
 * Copyright (c) 1999-2015 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

#ifndef _TTA_H
#define _TTA_H

#ifdef __GNUC__
#ifdef CARIBBEAN
#define ALLOW_OS_CODE 1

#include "../../../rmdef/rmdef.h"
#include "../../../rmlibcw/include/rmlibcw.h"
#include "../../../rmcore/include/rmcore.h"
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <locale.h>
#else // MSVC
#include <io.h>
#include <stdio.h>
#include <locale.h>
#include <intrin.h>
#endif

#ifdef __GNUC__
typedef char (TTAwchar);
#define T(s) s
#define L(s) s
#define LOCALE ""
#define tta_main main
#define tta_strlen strlen
#ifdef CPU_ARM
#define tta_print printf
#else
#define tta_print(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)
#endif
#define INVALID_SET_FILE_POINTER (-1)
#ifdef CARIBBEAN
typedef RMfile (HANDLE);
#define INVALID_HANDLE_VALUE (NULL)
#define tta_open_read(__name) RMOpenFile(__name,RM_FILE_OPEN_READ)
#define tta_open_write(__name) RMOpenFile(__name,RM_FILE_OPEN_WRITE)
#define tta_close(__handle) (RMCloseFile(__handle)==RM_OK?(0):(-1))
#define tta_unlink(__name) (RMFileUnlink(__name)==RM_OK?(0):(-1))
#define tta_read(__handle,__buffer,__size,__result) (RMReadFile(__handle,__buffer,__size,&(__result))==RM_OK?(1):(0))
#define tta_write(__handle,__buffer,__size,__result) (RMWriteFile(__handle,__buffer,__size,&(__result))==RM_OK?(1):(0))
#define tta_seek(__handle,__offset) (RMSeekFile(__handle,__offset,RM_FILE_SEEK_START)==RM_OK?(0):(-1))
#define tta_memclear(__dest,__length) RMMemset(__dest,0,__length)
#define tta_memcpy(__dest,__source,__length) RMMemcpy(__dest,__source,__length)
#define tta_malloc RMMalloc
#define tta_free RMFree
#else // NOT CARIBBEAN
typedef int (HANDLE);
#define INVALID_HANDLE_VALUE (-1)
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__FreeBSD__) || \
	(defined(__APPLE__) && defined(__MACH__))
#define lseek64 lseek
#endif
#define tta_open_read(__name) open(__name,O_RDONLY|O_NONBLOCK)
#define tta_open_write(__name) open(__name,O_RDWR|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR)
#define tta_close(__handle) close(__handle)
#define tta_unlink(__name) unlink(__name)
#define tta_read(__handle,__buffer,__size,__result) (__result=read(__handle,__buffer,__size))
#define tta_write(__handle,__buffer,__size,__result) (__result=write(__handle,__buffer,__size))
#define tta_seek(__handle,__offset) lseek64(__handle,__offset,SEEK_SET)
#define tta_reset(__handle) lseek64(__handle,0,SEEK_SET)
#define tta_memclear(__dest,__length) memset(__dest,0,__length)
#define tta_memcpy(__dest,__source,__length) memcpy(__dest,__source,__length)
#define tta_malloc(__length) aligned_alloc(16,__length)
#define tta_free free

#define tta_cpuid(func,ax,bx,cx,dx) \
	__asm__ __volatile__ ("cpuid": \
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
#endif // GNUC
#else // MSVC
typedef wchar_t (TTAwchar);
#define T(s) L##s
#define L(s) T(s)
#define LOCALE ".OCP"
#define STDIN_FILENO GetStdHandle(STD_INPUT_HANDLE)
#define STDOUT_FILENO GetStdHandle(STD_OUTPUT_HANDLE)
#define tta_main __cdecl wmain
#define tta_strlen (int) wcslen
#define tta_print(fmt, ...) fwprintf(stderr, L##fmt, ##__VA_ARGS__)
#define tta_open_read(__name) CreateFileW(__name,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)
#define tta_open_write(__name) CreateFileW(__name,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL)
#define tta_close(__handle) (CloseHandle(__handle)==TRUE?(0):(-1))
#define tta_unlink(__name) (DeleteFile(__name)==TRUE?(0):(-1))
#define tta_read(__handle,__buffer,__size,__result) ReadFile(__handle,__buffer,__size,(LPDWORD)&(__result),NULL)
#define tta_write(__handle,__buffer,__size,__result) WriteFile(__handle,__buffer,__size,(LPDWORD)&(__result),NULL)
#define tta_seek(__handle,__offset) SetFilePointer(__handle,(LONG)__offset,(PLONG)&__offset+1,FILE_BEGIN)
#define tta_reset(__handle) SetFilePointer(__handle,0,0,FILE_BEGIN)
#define tta_memclear(__dest,__length) ZeroMemory(__dest,__length)
#define tta_memcpy(__dest,__source,__length) CopyMemory(__dest,__source,__length)
#define tta_malloc(__length) _aligned_malloc(__length, 16)
#define tta_free(__dest) _aligned_free(__dest)

#define tta_cpuid(func,ax,bx,cx,dx) { \
	int cpuid[4]; \
	__cpuid(cpuid,func); ax=cpuid[0]; bx=cpuid[1]; cx=cpuid[2]; dx=cpuid[3]; }
#endif // MSVC

#ifdef __GNUC__
TTAuint32 GetTickCount() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

extern TTAwchar *optarg;
extern int optind;

#else // MSVC

TTAwchar *optarg;	// global argument pointer
int optind = 1;	// global argv index

int getopt(int argc, TTAwchar **argv, char *opts) {
	static int sp = 1;
	register char c;
	register char *cp;

	if (sp == 1) {
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == L'\0')
			return -1;
		else if(wcscmp(argv[optind], L"--") == 0) {
			optind++;
			return -1;
		}
	}

	c = argv[optind][sp] & 0xff;
	if (c == ':' || (cp = strchr(opts, c)) == NULL) {
		tta_print("%s: illegal option -- %c\n", argv[0], c);
		if (argv[optind][++sp] == L'\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}

	if (*++cp == ':') {
		if (argv[optind][sp + 1] != L'\0') {
			optarg = &argv[optind++][sp + 1];
		} else if (++optind >= argc) {
			if (*++cp != ':') {
				tta_print("%s: option requires an argument -- %c\n", argv[0], c);
				return('?');
			}
		} else {
			optarg = argv[optind++];
		}
		sp = 1;
	} else {
		if (argv[optind][++sp] == L'\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}

	return(c);
} // getopt

HANDLE mkstemp(TTAwchar *name_buffer) {
	if (_wmktemp_s(name_buffer, 10) == 0)
		return tta_open_write(name_buffer);
	return INVALID_HANDLE_VALUE;
} // tta_mktemp

#endif // MSVC

#endif // _TTA_H
