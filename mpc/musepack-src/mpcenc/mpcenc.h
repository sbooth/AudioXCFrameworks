/*
 * Musepack audio compression
 * Copyright (c) 2005-2009, The Musepack Development Team
 * Copyright (C) 1999-2004 Buschmann/Klemm/Piecha/Wolf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MPCENC_MPCENC_H
#define MPCENC_MPCENC_H

#include "libmpcenc.h"
#include "libmpcpsy.h"
#include <mpc/datatypes.h>
#include <mpc/minimax.h>

//// optimization/feature defines //////////////////////////////////
#ifndef NOT_INCLUDE_CONFIG_H
# include "config.h"
#endif

//// portable system includes //////////////////////////////////////
#include <stddef.h>
#include <math.h>

//// system dependent system includes //////////////////////////////
// low level I/O, where are prototypes and constants?
#if   defined _WIN32  ||  defined __TURBOC__  ||  defined __ZTC__  ||  defined _MSC_VER
# include <io.h>
# include <fcntl.h>
# include <windows.h>
#elif defined __unix__  ||  defined __linux__  ||  defined __APPLE__
# include <unistd.h>
# include <locale.h>
# include <langinfo.h>
#else
// .... add Includes for new Operating System here (with prefix: #elif defined)
# include <unistd.h>
#endif

#if   defined __linux__
#  include <fpu_control.h>
#elif defined __FreeBSD__
# include <machine/floatingpoint.h>
#elif defined _MSC_VER
# include <float.h>
#endif


#if !defined(__APPLE__)
// use optimized assembler routines for Pentium III/K6-2/Athlon (only 32 bit OS, Intel x86 and no MAKE_xxBITS)
// you need the NASM assembler on your system, the program becomes a little bit larger and decoding
// on AMD K6-2 (x3), AMD K6-III (x3), AMD Duron (x1.7), AMD Athlon (x1.7), Pentium III (x2) and Pentium 4 (x1.8) becomes faster
#define USE_ASM

#endif

// Use termios for reading values from keyboard without echo and ENTER
#define USE_TERMIOS

// make debug output in tags.c stfu
#define STFU

#if INT_MAX < 2147483647L
# undef USE_ASM
#endif

#ifndef O_BINARY
# ifdef _O_BINARY
#  define O_BINARY              _O_BINARY
# else
#  define O_BINARY              0
# endif
#endif

#if defined _WIN32  ||  defined __TURBOC__
# define strncasecmp(__s1,__s2,__n) strnicmp ((__s1), (__s2), (__n))
# define strcasecmp(__s1,__s2)      stricmp  ((__s1), (__s2))
#endif

#if defined _WIN32
# include <direct.h>
# define snprintf                   _snprintf
# define getcwd(__buff,__len)       _getcwd ((__buff), (__len))
#endif

//// Binary/Low-Level-IO ///////////////////////////////////////////
//

#if   defined __BORLANDC__  ||  defined _WIN32
# define FILENO(__fp)          _fileno ((__fp))
#elif defined __CYGWIN__  ||  defined __TURBOC__  ||  defined __unix__  ||  defined __EMX__  ||  defined _MSC_VER
# define FILENO(__fp)          fileno  ((__fp))
#else
# define FILENO(__fp)          fileno  ((__fp))
#endif


//
// If we have access to a file via file name, we can open the file with an
// additional "b" or a O_BINARY within the (f)open function to get a
// transparent untranslated data stream which is necessary for audio bitstream
// data and also for PCM data. If we are working with
// stdin/stdout/FILENO_STDIN/FILENO_STDOUT we can't open the file with these
// attributes, because the files are already open. So we need a non
// standardized sequence to switch to this mode (not necessary for Unix).
// Mostly the sequence is the same for incoming and outgoing streams, but only
// mostly so we need one for IN and one for OUT.
// Macros are called with the file pointer and you get back the untransalted file
// pointer which can be equal or different from the original.
//

#if   defined __EMX__
# define SETBINARY_IN(__fp)     (_fsetmode ( (__fp), "b" ), (__fp))
# define SETBINARY_OUT(__fp)    (_fsetmode ( (__fp), "b" ), (__fp))
#elif defined __TURBOC__ || defined __BORLANDC__
# define SETBINARY_IN(__fp)     (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
#elif defined __CYGWIN__
# define SETBINARY_IN(__fp)     (setmode   ( FILENO ((__fp)), _O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (setmode   ( FILENO ((__fp)), _O_BINARY ), (__fp))
#elif defined _WIN32
# define SETBINARY_IN(__fp)     (_setmode  ( FILENO ((__fp)), _O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (_setmode  ( FILENO ((__fp)), _O_BINARY ), (__fp))
#elif defined _MSC_VER
# define SETBINARY_IN(__fp)     (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
#elif defined __unix__
# define SETBINARY_IN(__fp)     (__fp)
# define SETBINARY_OUT(__fp)    (__fp)
#elif 0
# define SETBINARY_IN(__fp)     (freopen   ( NULL, "rb", (__fp) ), (__fp))
# define SETBINARY_OUT(__fp)    (freopen   ( NULL, "wb", (__fp) ), (__fp))
#else
# define SETBINARY_IN(__fp)     (__fp)
# define SETBINARY_OUT(__fp)    (__fp)
#endif

// file I/O using ANSI buffered file I/O via file pointer FILE* (fopen, fread, fwrite, fclose)
#define READ(fp,ptr,len)       fread  (ptr, 1, len, fp)     // READ    returns -1 or 0 on error/EOF, otherwise > 0
#define READ1(fp,ptr)          fread  (ptr, 1, 1, fp)       // READ    returns -1 or 0 on error/EOF, otherwise > 0

#ifdef _WIN32
# define POPEN_READ_BINARY_OPEN(cmd)    _popen ((cmd), "rb")
#else
# define POPEN_READ_BINARY_OPEN(cmd)    popen ((cmd), "r")
#endif

// Path separator
#if defined __unix__  ||  defined __bsdi__  ||  defined __FreeBSD__  ||  defined __OpenBSD__  ||  defined __NetBSD__  ||  defined __APPLE__
# define PATH_SEP               '/'
# define DRIVE_SEP              '\0'
# define EXE_EXT                ""
# define DEV_NULL               "/dev/null"
# define ENVPATH_SEP            ':'
#elif defined _WIN32  ||  defined __TURBOC__  ||  defined __ZTC__  ||  defined _MSC_VER
# define PATH_SEP               '\\'
# define DRIVE_SEP              ':'
# define EXE_EXT                ".exe"
# define DEV_NULL               "\\nul"
# define ENVPATH_SEP            ';'
#else
# define PATH_SEP               '/'         // Amiga: C:/
# define DRIVE_SEP              ':'
# define EXE_EXT                ""
# define DEV_NULL               "nul"
# define ENVPATH_SEP            ';'
#endif

#ifdef _WIN32
# define TitleBar(text)   SetConsoleTitle (text)
#else
# define TitleBar(text)   (void) (text)
#endif


//// constants /////////////////////////////////////////////////////
#define DECODER_DELAY    (512 - 32 + 1)
#define BLK_SIZE         (36 * 32)


//// procedures/functions //////////////////////////////////////////
// pipeopen.c
FILE*      pipeopen                   ( const char* command, const char* filename );

// stderr.c
void       SetStderrSilent            ( mpc_bool_t state );
mpc_bool_t GetStderrSilent            ( void );
int mpc_cdecl  stderr_printf              ( const char* format, ... );

// quant.h
#define SCFfac              0.832980664785f     // = SCF[n-1]/SCF[n]

// wave_in.h
typedef struct {
    FILE*         fp;                   // File pointer to read data
    mpc_size_t    PCMOffset;            // File offset of PCM data
    long double   SampleFreq;           // Sample frequency in Hz
    mpc_uint_t    BitsPerSample;        // used bits per sample, 8*BytesPerSample-7 <= BitsPerSample <= BytesPerSample
    mpc_uint_t    BytesPerSample;       // allocated bytes per sample
    mpc_uint_t    Channels;             // Number of channels, 1...8
	mpc_size_t    PCMBytes;             // PCM Samples (in 8 bit units)
	mpc_size_t    PCMSamples;           // PCM Samples per Channel
    mpc_bool_t    raw;                  // raw: headerless format
} wave_t;

typedef float SCFTriple [3];

// FIXME : put in lib header

void   Init_FFT      ( PsyModel* );

// FIXME : put in lib header
void   Init_Psychoakustik       ( PsyModel* );
SMRTyp Psychoakustisches_Modell ( PsyModel *, const int, const PCMDataTyp*, int* TransientL, int* TransientR );
void   TransientenCalc          ( int* Transient, const int* TransientL, const int* TransientR );
void   RaiseSMR                 ( PsyModel*, const int, SMRTyp* );
void   MS_LR_Entscheidung       ( const int, unsigned char* MS, SMRTyp*, SubbandFloatTyp* );
void   Init_Psychoakustiktabellen ( PsyModel* );

void   NS_Analyse (PsyModel*, const int, const unsigned char* MS, const SMRTyp, const int* Transient );

void   Analyse_Filter(const PCMDataTyp*, SubbandFloatTyp*, const int);
void   Analyse_Init ( float Left, float Right, SubbandFloatTyp* out, const int MaxBand );

void SetQualityParams (PsyModel *, float );
int TestProfileParams ( PsyModel* );

extern const float  Butfly    [7];              // Antialiasing to calculate the subband powers
extern const float  InvButfly [7];              // Antialiasing to calculate the masking thresholds
extern const float  iw        [PART_LONG];      // inverse partition-width for long
extern const float  iw_short  [PART_SHORT];     // inverse partition-width for short
extern const int    wl        [PART_LONG];      // w_low  for long
extern const int    wl_short  [PART_SHORT];     // w_low  for short
extern const int    wh        [PART_LONG];      // w_high for long
extern const int    wh_short  [PART_SHORT];     // w_high for short


// quant.c
extern float __invSCF [128 + 6];        // tabulated scalefactors (inverted)
#define invSCF  (__invSCF + 6)

float  ISNR_Schaetzer                  ( const float* samples, const float comp, const int res);
float  ISNR_Schaetzer_Trans            ( const float* samples, const float comp, const int res);
void   QuantizeSubband                 ( unsigned int* qu_output, const float* input, const int res, float* errors, const int maxNsOrder );
void   QuantizeSubbandWithNoiseShaping ( unsigned int* qu_output, const float* input, const int res, float* errors, const float* FIR );

void   NoiseInjectionComp ( void );

// keyboard.c
int    WaitKey      ( void );
int    CheckKeyKeep ( void );
int    CheckKey     ( void );


// regress.c
void    Regression       ( float* const _r, float* const _b, const float* p, const float* q );


// tags.c
void    Init_Tags        ( void );
int     FinalizeTags     ( FILE* fp, unsigned int Version, unsigned int flags );
int     addtag           ( const char* key, size_t keylen, const char* value, size_t valuelen, int converttoutf8, int flags );
int     gettag           ( const char* key, char* dst, size_t len );
int     CopyTags         ( const char* filename );


// wave_in.c
int     Open_WAV_Header  ( wave_t* type, const char* name );
size_t  Read_WAV_Samples ( wave_t* t, const size_t RequestedSamples, PCMDataTyp* data, const ptrdiff_t offset, const float scalel, const float scaler, int* Silence );
int     Read_WAV_Header  ( wave_t* type );


// winmsg.c
#ifdef _WIN32
#define WIN32_MESSAGES      1                   // support Windows-Messaging to Frontend
int    SearchForFrontend   ( void );
void   SendQuitMessage     ( void );
void   SendModeMessage     ( const int );
void   SendStartupMessage  ( const char*, const int);
void   SendProgressMessage ( const int, const float, const float );
#else
# define WIN32_MESSAGES                 0
# define SearchForFrontend()            (0)
# define SendQuitMessage()              (void)0
# define SendModeMessage(x)             (void)0
# define SendStartupMessage(x,y)        (void)0
# define SendProgressMessage(x,y,z)     (void)0
#endif /* _WIN32 */


#define MPPENC_DENORMAL_FIX_BASE ( 32. * 1024. /* normalized sample value range */ / ( (float) (1 << 24 /* first bit below 32-bit PCM range */ ) ) )
#define MPPENC_DENORMAL_FIX_LEFT ( MPPENC_DENORMAL_FIX_BASE )
#define MPPENC_DENORMAL_FIX_RIGHT ( MPPENC_DENORMAL_FIX_BASE * 0.5f )

#ifndef LAST_HUFFMAN
# define LAST_HUFFMAN    7
#endif

#endif /* MPCENC_MPCENC_H */

/* end of mpcenc.h */
