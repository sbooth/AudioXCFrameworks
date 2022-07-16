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

# include <string.h>
#include "mpcenc.h"

#ifdef _WIN32

static int
init_in ( const int  SampleCount,
          const int  SampleFreq,
          const int  Channels,
          const int  BitsPerSample );
static size_t
get_in ( void* DataPtr );

#endif

#define EXT(x)  (0 == strcasecmp (ext, #x))

int
Open_WAV_Header ( wave_t* type, const char* filename )
{
    const char*  ext = strrchr ( filename, '.');
    FILE*        fp;

    type -> raw = 0;

    if ( 0 == strcmp ( filename, "-")  ||  0 == strcmp ( filename, "/dev/stdin") ) {
        fp = SETBINARY_IN ( stdin );
    }
    else if ( ext == NULL ) {
        fp = NULL;
    }
    else if ( EXT(.wav) ) {
        fp = fopen ( filename, "rb" );
    }
    else if ( EXT(.wv) ) {                              // wavpack (www.wavpack.com)
        fp = pipeopen ( "wvunpack # -", filename );
    }
    else if ( EXT(.la) ) {                              // lossless-audio (www.lossless-audio.com)
        fp = pipeopen ( "la -console #", filename );
    }
    else if ( EXT(.raw)  ||  EXT(.cdr)  ||  EXT(.pcm) ) {
        fp = fopen ( filename, "rb" );
        type->Channels       = 2;
        type->BitsPerSample  = 16;
        type->BytesPerSample = 2;
        type->SampleFreq     = 44100.;
        type->raw            = 1;
        type->PCMOffset      = 0;
        type->PCMBytes       = 0xFFFFFFFF;
        type->PCMSamples     = 86400 * type->SampleFreq;
    }
    else if ( EXT(.pac)  ||  EXT(.lpac)  ||  EXT(.lpa) ) {
        fp = pipeopen ( "lpac -o -x #", filename );
    }
    else if ( EXT(.fla)  ||  EXT(.flac) ) {
#ifdef _WIN32
        stderr_printf ( "*** Install at least version 1.03 of FLAC.EXE. Thanks! ***\n\n" );
#endif
        fp = pipeopen ( "flac -d -s -c - < #", filename );
    }
    else if ( EXT(.rka)  ||  EXT(.rkau) ) {
        fp = pipeopen ( "rkau # -", filename );
    }
    else if ( EXT(.sz) ) {
        fp = pipeopen ( "szip -d < #", filename );
    }
    else if ( EXT(.sz2) ) {
        fp = pipeopen ( "szip2 -d < #", filename );
    }
    else if ( EXT(.ofr) ) {
        fp = pipeopen ( "optimfrog d # -", filename );
    }
    else if ( EXT(.ape) ) {
        fp = pipeopen ( "mac # - -d", filename );
    }
    else if ( EXT(.shn)  ||  EXT(.shorten) ) {
#ifdef _WIN32
        stderr_printf ( "*** Install at least version 3.4 of Shorten.exe. Thanks! ***\n\n" );
#endif
        fp = pipeopen ( "shorten -x # -", filename );           // Test if it's okay !!!!
        if ( fp == NULL )
            fp = pipeopen ( "shortn32 -x # -", filename );
    }
    else if ( EXT(.mod) ) {
        fp = pipeopen ( "xmp -b16 -c -f44100 --stereo -o- #", filename );
        type->Channels       = 2;
        type->BitsPerSample  = 16;
        type->BytesPerSample = 2;
        type->SampleFreq     = 44100.;
        type->raw            = 1;
        type->PCMOffset      = 0;
        type->PCMBytes       = 0xFFFFFFFF;
        type->PCMSamples     = 86400 * type->SampleFreq;
    }
    else {
        fp = NULL;
    }

    type -> fp  = fp;
    return fp == NULL  ?  -1  :  0;
}

#undef EXT


static float f0  ( const void* p )
{
    return (void)p, 0.;
}

static float f8  ( const void* p )
{
    return (((unsigned char*)p)[0] - 128) * 256.;
}

static float f16 ( const void* p )
{
    return ((unsigned char*)p)[0] + 256. * ((signed char*)p)[1];
}

static float f24 ( const void* p )
{
    return ((unsigned char*)p)[0]*(1./256) + ((unsigned char*)p)[1] + 256 * ((signed char*)p)[2];
}

static float f32 ( const void* p )
{
    return ((unsigned char*)p)[0]*(1./65536) + ((unsigned char*)p)[1]*(1./256) + ((unsigned char*)p)[2] + 256 * ((signed char*)p)[3];
}


typedef float (*rf_t) (const void*);

static int
DigitalSilence ( void* buffer, size_t len )
{
    unsigned long*  pl;
    unsigned char*  pc;
    size_t          loops;

    for ( pl = buffer, loops = len >> 3; loops--; pl += 2 )
        if ( pl[0] | pl[1] )
            return 0;

    for ( pc = (unsigned char*)pl, loops = len & 7; loops--; pc++ )
        if ( pc[0] )
            return 0;

    return 1;
}


size_t
Read_WAV_Samples ( wave_t*          t,
                   const size_t     RequestedSamples,
                   PCMDataTyp*      data,
                   const ptrdiff_t  offset,
                   const float      scalel,
                   const float      scaler,
                   int*             Silence )
{
    static const rf_t rf [5] = { f0, f8, f16, f24, f32 };
    short   Buffer [8 * 32/16 * BLOCK]; // read buffer, up to 8 channels, up to 32 bit
    size_t  ReadSamples;                // returns number of read samples
    size_t  i;
    short*  b = (short*) Buffer;
    char*   c = (char*) Buffer;
    float*  l = data -> L + offset;
    float*  r = data -> R + offset;
    float*  m = data -> M + offset;
    float*  s = data -> S + offset;

    // Read PCM data
#ifdef _WIN32
    if ( t->fp != (FILE*)-1 ) {
        ReadSamples = fread ( b, t->BytesPerSample * t->Channels, RequestedSamples, t->fp );
    }
    else {
        while (1) {
            ReadSamples = get_in (b) / ( t->Channels * t->BytesPerSample );
            if ( ReadSamples != 0 )
                break;
            Sleep (10);
        }
    }
#else
    ReadSamples = fread ( b, t->BytesPerSample * t->Channels, RequestedSamples, t->fp );
#endif


    *Silence    = DigitalSilence ( b, ReadSamples * t->BytesPerSample * t->Channels );

    // Add Null Samples if EOF is reached
    if ( ReadSamples != RequestedSamples )
        //memset ( b + ReadSamples * t->Channels, 0, (RequestedSamples - ReadSamples) * (sizeof(short) * t->Channels) );
		memset ( c + ReadSamples * t->Channels * t->BytesPerSample, t->BytesPerSample == 1 ? 0x80 : 0, (RequestedSamples - ReadSamples) * (t->BytesPerSample * t->Channels) );

    // Convert to float and calculate M=(L+R)/2 and S=(L-R)/2 signals
#ifndef MPC_BIG_ENDIAN
    if ( t->BytesPerSample == 2 ) {
        switch ( t->Channels ) {
        case 1:
            for ( i = 0; i < RequestedSamples; i++, b++ ) {
				float temp = b[0] * scalel;
				l[i] = temp + MPPENC_DENORMAL_FIX_LEFT;
				r[i] = temp + MPPENC_DENORMAL_FIX_RIGHT;
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        case 2:
            for ( i = 0; i < RequestedSamples; i++, b += 2 ) {
                l[i] = b[0] * scalel + MPPENC_DENORMAL_FIX_LEFT;           // left
                r[i] = b[1] * scaler + MPPENC_DENORMAL_FIX_RIGHT;           // right
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        case 5:
        case 6:
        case 7:
        case 8:
            for ( i = 0; i < RequestedSamples; i++, b += t->Channels ) {
                l[i] = (0.4142 * b[0] + 0.2928 * b[1] + 0.2928 * b[3] - 0.1464 * b[4]) * scalel + MPPENC_DENORMAL_FIX_LEFT;           // left
                r[i] = (0.4142 * b[2] + 0.2928 * b[1] + 0.2928 * b[4] - 0.1464 * b[3]) * scaler + MPPENC_DENORMAL_FIX_RIGHT;           // right
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        default:
            for ( i = 0; i < RequestedSamples; i++, b += t->Channels ) {
                l[i] = b[0] * scalel + MPPENC_DENORMAL_FIX_LEFT;           // left
                r[i] = b[1] * scaler + MPPENC_DENORMAL_FIX_RIGHT;           // right
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        }
    }
    else
#endif
         {
        unsigned int  bytes = t->BytesPerSample;
        rf_t          f     = rf [bytes];

        c = (char*)b;
        switch ( t->Channels ) {
        case 1:
            for ( i = 0; i < RequestedSamples; i++, c += bytes ) {
				float temp = f(c) * scalel;
				l[i] = temp + MPPENC_DENORMAL_FIX_LEFT;
				r[i] = temp + MPPENC_DENORMAL_FIX_RIGHT;
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        case 2:
            for ( i = 0; i < RequestedSamples; i++, c += 2*bytes ) {
                l[i] = f(c)       * scalel + MPPENC_DENORMAL_FIX_LEFT;     // left
                r[i] = f(c+bytes) * scaler + MPPENC_DENORMAL_FIX_RIGHT;     // right
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        default:
            for ( i = 0; i < RequestedSamples; i++, c += bytes * t->Channels ) {
                l[i] = f(c)       * scalel + MPPENC_DENORMAL_FIX_LEFT;     // left
                r[i] = f(c+bytes) * scaler + MPPENC_DENORMAL_FIX_RIGHT;     // right
                m[i] = (l[i] + r[i]) * 0.5f;
                s[i] = (l[i] - r[i]) * 0.5f;
            }
            break;
        }
    }
    return ReadSamples;
}


// read WAVE header

static unsigned short
Read16 ( FILE* fp )
{
    unsigned char  buff [2];

    if (fread ( buff, 1, 2, fp ) != 2 )
        return -1;
    return buff[0] | (buff[1] << 8);
}

static unsigned long
Read32 ( FILE* fp )
{
    unsigned char  buff [4];

    if ( fread ( buff, 1, 4, fp ) != 4 )
        return -1;
    return (buff[0] | (buff[1] << 8)) | ((unsigned long)(buff[2] | (buff[3] << 8)) << 16);
}


int
Read_WAV_Header ( wave_t* type )
{
	int bytealign;

    FILE*  fp = type->fp;

    if ( type->raw )
        return 0;

    fseek ( fp, 0, SEEK_SET );
    if ( Read32 (fp) != 0x46464952 ) {                  // 4 Byte: check for "RIFF"
        stderr_printf ( Read32(fp) == -1  ?  " ERROR: Empty file or no data from coprocess!\n\n"
                                          :  " ERROR: 'RIFF' not found in WAVE header!\n\n");
        return -1;
    }
    Read32 (fp);                                        // 4 Byte: chunk size (ignored)
    if ( Read32 (fp) != 0x45564157 ) {                  // 4 Byte: check for "WAVE"
        stderr_printf ( " ERROR: 'WAVE' not found in WAVE header!\n\n");
        return -1;
    }
    if ( Read32 (fp) != 0x20746D66 ) {                  // 4 Byte: check for "fmt "
        stderr_printf ( " ERROR: 'fmt ' not found in WAVE header!\n\n");
        return -1;
    }
    Read32 (fp);                                        // 4 Byte: read chunk-size (ignored)
    if ( Read16 (fp) != 0x0001 ) {                      // 2 Byte: check for linear PCM
        stderr_printf ( " ERROR: WAVE file has no linear PCM format!\n\n");
        return -1;
    }
    type -> Channels    = Read16 (fp);                  // 2 Byte: read no. of channels
    type -> SampleFreq  = Read32 (fp);                  // 4 Byte: read sampling frequency
    Read32 (fp);                                        // 4 Byte: read avg. blocksize (fs*channels*bytepersample)
    bytealign = Read16 (fp);							// 2 Byte: read byte-alignment (channels*bytepersample)
    type->BitsPerSample = Read16 (fp);                  // 2 Byte: read bits per sample
    type->BytesPerSample= (type->BitsPerSample + 7) / 8;
    while ( 1 ) {                                       // search for "data"
        if ( feof (fp) )
            return -1;
        if ( Read16 (fp) != 0x6164 )
            continue;
        if ( Read16 (fp) == 0x6174 )
            break;
    }
    type->PCMBytes      = Read32 (fp);                  // 4 Byte: no. of byte in file
    if ( feof (fp) ) return -1;

														// finally calculate number of samples
    if (type->PCMBytes >= 0xFFFFFF00  ||
			type->PCMBytes == 0  ||
			(mpc_uint32_t)type->PCMBytes % (type -> Channels * type->BytesPerSample) != 0) {
		type->PCMSamples = 36000000 * type->SampleFreq;
	}
	else {
		type->PCMSamples = type->PCMBytes / bytealign;
	}
    type->PCMOffset     = ftell (fp);
    return 0;
}


#ifdef _WIN32

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <mmsystem.h>
#ifndef __MINGW32__
#include <mmreg.h>
#endif
#include <io.h>
#include <fcntl.h>


#define NBLK  383               // 10 sec of audio


typedef struct {
    int      active;
    char*    data;
    size_t   datalen;
    WAVEHDR  hdr;
} oblk_t;

static HWAVEIN       Input_WAVHandle;
static HWAVEOUT      Output_WAVHandle;
static size_t        BufferBytes;
static WAVEHDR       whi    [NBLK];
static char*         data   [NBLK];
static oblk_t        array  [NBLK];
static unsigned int  NextInputIndex;
static unsigned int  NextOutputIndex;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
init_in ( const int  SampleCount,
          const int  SampleFreq,
          const int  Channels,
          const int  BitsPerSample )
{

    WAVEFORMATEX  pwf;
    MMRESULT      r;
    int           i;

    pwf.wFormatTag      = WAVE_FORMAT_PCM;
    pwf.nChannels       = Channels;
    pwf.nSamplesPerSec  = SampleFreq;
    pwf.nAvgBytesPerSec = SampleFreq * Channels * ((BitsPerSample + 7) / 8);
    pwf.nBlockAlign     = Channels * ((BitsPerSample + 7) / 8);
    pwf.wBitsPerSample  = BitsPerSample;
    pwf.cbSize          = 0;

    r = waveInOpen ( &Input_WAVHandle, WAVE_MAPPER, &pwf, 0, 0, CALLBACK_EVENT );
    if ( r != MMSYSERR_NOERROR ) {
        fprintf ( stderr, "waveInOpen failed: ");
        switch (r) {
        case MMSYSERR_ALLOCATED:   fprintf ( stderr, "resource already allocated\n" );                                  break;
        case MMSYSERR_INVALPARAM:  fprintf ( stderr, "invalid Params\n" );                                              break;
        case MMSYSERR_BADDEVICEID: fprintf ( stderr, "device identifier out of range\n" );                              break;
        case MMSYSERR_NODRIVER:    fprintf ( stderr, "no device driver present\n" );                                    break;
        case MMSYSERR_NOMEM:       fprintf ( stderr, "unable to allocate or lock memory\n" );                           break;
        case WAVERR_BADFORMAT:     fprintf ( stderr, "attempted to open with an unsupported waveform-audio format\n" ); break;
        case WAVERR_SYNC:          fprintf ( stderr, "device is synchronous but waveOutOpen was\n" );                   break;
        default:                   fprintf ( stderr, "unknown error code: %#X\n", r );                                  break;
        }
        return -1;
    }

    BufferBytes = SampleCount * Channels * ((BitsPerSample + 7) / 8);

    for ( i = 0; i < NBLK; i++ ) {
        whi [i].lpData         = data [i] = malloc (BufferBytes);
        whi [i].dwBufferLength = BufferBytes;
        whi [i].dwFlags        = 0;
        whi [i].dwLoops        = 0;

        r = waveInPrepareHeader ( Input_WAVHandle, whi + i, sizeof (*whi) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveInPrepareHeader  (%u) failed\n", i );  return -1; }
        r = waveInAddBuffer     ( Input_WAVHandle, whi + i, sizeof (*whi) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveInAddBuffer      (%u) failed\n", i );  return -1; }
    }
    NextInputIndex = 0;
    waveInStart (Input_WAVHandle);
    return 0;
}


size_t
get_in ( void* DataPtr )
{
    MMRESULT  r;
    size_t    Bytes;

    if ( whi [NextInputIndex].dwFlags & WHDR_DONE ) {
        Bytes = whi [NextInputIndex].dwBytesRecorded;
        memcpy ( DataPtr, data [NextInputIndex], Bytes );

        r = waveInUnprepareHeader ( Input_WAVHandle, whi + NextInputIndex, sizeof (*whi) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveInUnprepareHeader (%d) failed\n", NextInputIndex ); return -1; }
        whi [NextInputIndex].lpData         = data [NextInputIndex];
        whi [NextInputIndex].dwBufferLength = BufferBytes;
        whi [NextInputIndex].dwFlags        = 0;
        whi [NextInputIndex].dwLoops        = 0;
        r = waveInPrepareHeader   ( Input_WAVHandle, whi + NextInputIndex, sizeof (*whi) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveInPrepareHeader   (%d) failed\n", NextInputIndex ); return -1; }
        r = waveInAddBuffer       ( Input_WAVHandle, whi + NextInputIndex, sizeof (*whi) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveInAddBuffer       (%d) failed\n", NextInputIndex ); return -1; }
        NextInputIndex = (NextInputIndex + 1) % NBLK;
        return  Bytes;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
init_out ( const int  SampleCount,
           const int  SampleFreq,
           const int  Channels,
           const int  BitsPerSample )
{
    WAVEFORMATEX  pwf;
    MMRESULT      r;
    int           i;

    pwf.wFormatTag      = WAVE_FORMAT_PCM;
    pwf.nChannels       = Channels;
    pwf.nSamplesPerSec  = SampleFreq;
    pwf.nAvgBytesPerSec = SampleFreq * Channels * ((BitsPerSample + 7) / 8);
    pwf.nBlockAlign     = Channels * ((BitsPerSample + 7) / 8);
    pwf.wBitsPerSample  = BitsPerSample;
    pwf.cbSize          = 0;

    r = waveOutOpen ( &Output_WAVHandle, WAVE_MAPPER, &pwf, 0, 0, CALLBACK_EVENT );
    if ( r != MMSYSERR_NOERROR ) {
        fprintf ( stderr, "waveOutOpen failed\n" );
        switch (r) {
        case MMSYSERR_ALLOCATED:   fprintf ( stderr, "resource already allocated\n" );                                  break;
        case MMSYSERR_INVALPARAM:  fprintf ( stderr, "invalid Params\n" );                                              break;
        case MMSYSERR_BADDEVICEID: fprintf ( stderr, "device identifier out of range\n" );                              break;
        case MMSYSERR_NODRIVER:    fprintf ( stderr, "no device driver present\n" );                                    break;
        case MMSYSERR_NOMEM:       fprintf ( stderr, "unable to allocate or lock memory\n" );                           break;
        case WAVERR_BADFORMAT:     fprintf ( stderr, "attempted to open with an unsupported waveform-audio format\n" ); break;
        case WAVERR_SYNC:          fprintf ( stderr, "device is synchronous but waveOutOpen was\n" );                   break;
        default:                   fprintf ( stderr, "unknown error code: %#X\n", r );                                  break;
        }
        return -1;
    }

    BufferBytes = SampleCount * Channels * ((BitsPerSample + 7) / 8);

    for ( i = 0; i < NBLK; i++ ) {
        array [i].active = 0;
        array [i].data   = malloc (BufferBytes);
    }
    NextOutputIndex = 0;
    return 0;
}


int
put_out ( const void*   DataPtr,
          const size_t  Bytes )
{
    MMRESULT  r;
    int       i = NextOutputIndex;

    if ( array [i].active )
        while ( ! (array [i].hdr.dwFlags & WHDR_DONE) )
            Sleep (26);

    r = waveOutUnprepareHeader ( Output_WAVHandle, &(array [i].hdr), sizeof (array [i].hdr) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveOutUnprepareHeader (%d) failed\n", i ); return -1; }

    array [i].active             = 1;
    array [i].hdr.lpData         = array [i].data;
    array [i].hdr.dwBufferLength = Bytes;
    array [i].hdr.dwFlags        = 0;
    array [i].hdr.dwLoops        = 0;
    memcpy ( array [i].data, DataPtr, Bytes );

    r = waveOutPrepareHeader   ( Output_WAVHandle, &(array [i].hdr), sizeof (array [i].hdr) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveOutPrepareHeader   (%d) failed\n", i ); return -1; }
    r = waveOutWrite           ( Output_WAVHandle, &(array [i].hdr), sizeof (array [i].hdr) ); if ( r != MMSYSERR_NOERROR ) { fprintf ( stderr, "waveOutAddBuffer       (%d) failed\n", i ); return -1; }

    NextInputIndex = (NextInputIndex + 1) % NBLK;
    return Bytes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif

/* end of wave_in.c */
