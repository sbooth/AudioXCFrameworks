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

/* overflow of subband-samples */

#include <memory.h>
#include <time.h>
#include <errno.h>

#include "mpcenc.h"
#include <mpc/mpcmath.h>

/* G L O B A L  V A R I A B L E S */
float         Power_L    [32] [3];
float         Power_R    [32] [3];

/* MS-Coding */
int           PredictionBands =  0;
int           DisplayUpdateTime = 1;
int           APE_Version     = 2000;
int           LowDelay        = 0;
mpc_bool_t    IsEndBeep       = MPC_FALSE;

#define MODE_OVERWRITE          0
#define MODE_NEVER_OVERWRITE    1
#define MODE_ASK_FOR_OVERWRITE  2

/* other general global variables */
unsigned int  DelInput        = 0;      // deleting the input file after encoding
unsigned int  WriteMode       = MODE_ASK_FOR_OVERWRITE;      // overwriting a possibly existing MPC file
unsigned int  verbose         = 0;      // more information during output
unsigned int  NoUnicode       = 1;      // console is unicode or not (tag translation)
unsigned int  NoEncoderInfo   = 0;      // write encoder info block or not
unsigned int  NoSeekTable     = 0;      // write seek table block or not
unsigned int  FramesBlockPwr  = 6;      // must be even : frames_per_block = 1 << FramesBlockPwr
unsigned int  SeekDistance    = 1;      // keep a seek table entry every 2^SeekDistance block
mpc_uint64_t  SamplesInWAVE   = 0;      // number of samples per channel in the WAV file
float         MaxOverFlow     = 0.f;    // maximum overflow
float         ScalingFactorl  = 1.f;    // Scaling the input signal
float         ScalingFactorr  = 1.f;    // Scaling the input signal
float         FadeShape       = 1.f;    // Shape of the fade
float         FadeInTime      = 0.f;    // Duration of FadeIn in secs
float         FadeOutTime     = 0.f;    // Duration of FadeOut in secs
float         SkipTime        = 0.f;    // Skip the beginning of the file (sec)
double        Duration        = 1.e+99; // Maximum encoded audio length
mpc_bool_t    FrontendPresent = 0;      // Flag for frontend-detection

#if MPCENC_MINOR % 2 == 0
#define _cat(a,b,c) #a"."#b"."#c" --stable--"
#else
#define _cat(a,b,c) #a"."#b"."#c" --unstable--"
#endif

#define cat(a,b,c) _cat(a,b,c)
#define MPCENC_VERSION cat(MPCENC_MAJOR,MPCENC_MINOR,MPCENC_BUILD)

const char    About []        = "MPC Encoder " MPCENC_VERSION " (C) 1999-2009 Buschmann/Klemm/Piecha/MDT\nBuilt " __DATE__ " " __TIME__;


#if defined _WIN32  ||  defined __TURBOC__
# include <conio.h>
#else

# ifdef USE_TERMIOS
#  include <termios.h>

static struct termios  stored_settings;

static void
echo_on ( void )
{
    tcsetattr ( 0, TCSANOW, &stored_settings );
}

static void
echo_off ( void )
{
    struct termios  new_settings;

    tcgetattr ( 0, &stored_settings );
    new_settings = stored_settings;

    new_settings.c_lflag     &= ~ECHO;
    new_settings.c_lflag     &= ~ICANON;        /* Disable canonical mode, and set buffer size to 1 byte */
    new_settings.c_cc[VTIME]  = 0;
    new_settings.c_cc[VMIN]   = 1;

    tcsetattr ( 0, TCSANOW, &new_settings );
}

# else
#  define echo_off()  (void)0
#  define echo_on()   (void)0
# endif

static int
getch ( void )
{
    unsigned char  buff [1];
    int            ret;

    echo_off ();
    ret = READ1 ( stdin, buff );
    echo_on ();
    return ret == 1  ?  buff[0]  :  -1;
}

#endif


static int
waitkey ( void )
{
    int  c;

    fflush (stdout);
    while ( (c = getch() ) <= ' ' )
        ;
    return c;
}




static void
longhelp ( PsyModel * m )
{
    stderr_printf (
             "\n"
             "\033[1m\rUsage:\033[0m\n"
             "  mpcenc [--options] <Input_File>\n"
             "  mpcenc [--options] <Input_File> <Output_File>\n"
             "\n" );

    stderr_printf (
             "\033[1m\rInput_File must be of the following:\033[0m\n"
             "  -                stdin                 (only RIFF WAVE files)\n"
             "  *.wav            RIFF WAVE file\n"
             "  *.raw/cdr        Raw PCM               (2ch, 16bit, 44.1kHz)\n"
             "  *.pac/lpac       LPAC file             (Windows Only)\n"
             "  *.fla/flac       FLAC file\n"
             "  *.ape            Monkey's Audio file   (APE extension only)\n"
             "  *.rka/rkau       RK Audio file         (Windows Only)\n"
             "  *.sz             SZIP file\n"
             "  *.shn            Shorten file\n"
             "  *.wv             Wavpack File\n"
             "  *.ofr            OptimFROG file        (Windows Only)\n"
             "\n"
             "  Currently only 32, 37.8, 44.1 and 48 kHz, 1-8 channels, 8-32 bit linear PCM\n"
             "  is supported. When using one of the lossless compressed formats, a proper\n"
             "  binary must be installed within the system's $PATH.\n"
             "\n"
             "\033[1m\rOutput_File must be of the following: (or generated from Input_File)\033[0m\n"
             "  *.mpc            Musepack file\n"
             "  *.mp+/mpp        MPEGplus file         (Deprecated)\n"
             "  -                stdout\n"
             "  /dev/null        trash can\n"
             "\n" );

    stderr_printf (
             "\033[1m\rProfiles and Quality Scale:\033[0m\n"
             "  Option of using a profile (--radio) or mapped quality scale (--quality 4.0).\n"
             "  In addition, quality scale is effective centesimally. (i.e. --quality 4.25)\n"
             "  Available options are as follows:\n"
             "\n"
             "  below telephone  (--quality 0.00)   poor quality          (~  20 kbps)\n"
             "  below telephone  (--quality 1.00)   poor quality          (~  30 kbps)\n"
             "  --telephone      (--quality 2.00)   low quality           (~  60 kbps)\n"
             "  --thumb          (--quality 3.00)   low/medium quality    (~  90 kbps)\n"
             "  --radio          (--quality 4.00)   medium quality        (~ 130 kbps)\n"
             "  --standard       (--quality 5.00)   high quality  (dflt)  (~ 180 kbps)\n"
             "   (or --normal)\n"
             "  --extreme        (--quality 6.00)   excellent quality     (~ 210 kbps)\n"
             "   (or --xtreme)\n"
             "  --insane         (--quality 7.00)   excellent quality     (~ 240 kbps)\n"
             "  --braindead      (--quality 8.00)   excellent quality     (~ 270 kbps)\n"
             "  above braindead  (--quality 9.00)   excellent quality     (~ 300 kbps)\n"
             "  above braindead  (--quality 10.00)  excellent quality     (~ 350 kbps)\n"
             "\n" );

	stderr_printf (
			"\033[1m\rBitstream formating:\033[0m\n"
			"  --no_ei           do not write encoder information packet   (dflt: off)\n"
			"  --no_st           do not write the seek table               (dflt: off)\n"
			"  --num_frames x    x = 0..7 number of frames in packet = 4^x (dflt: 3)\n"
			"  --seek_distance x x = 0..15 keep a seek table entry every 2^x audio packet (dflt: 1)\n"
			"\n" );

    stderr_printf (
             "\033[1m\rFile/Message handling:\033[0m\n"
             "  --silent         repress console messages                 (dflt: off)\n"
             "  --verbose        increase verbosity                       (dflt: off)\n"
             "  --longhelp       print this help text\n"
             "  --stderr foo     append messages to file 'foo'\n"
             "  --neveroverwrite never overwrite existing Output_File     (dflt: off)\n"
             "  --interactive    ask to overwrite an existing Output_File (dflt: on)\n"
             "  --overwrite      overwrite existing Output_File           (dflt: off)\n"
             "  --deleteinput    delete Input_File after encoding         (dflt: off)\n"
             "  --beep           beep when encoding is finished           (dflt: off)\n"
             "\n" );

    stderr_printf (
             "\033[1m\rTagging (uses APE 2.0 tags):\033[0m\n"
             "  --tag key=value  Add tag \"key\" with \"value\" as contents\n"
             "  --tagfile key=f  dto., take value from a file 'f'\n"
             "  --tagfile key    dto., take value from console\n"
             "  --artist 'value' shortcut for --tag 'Artist=value'\n"
             "  --album 'value'  shortcut for --tag 'Album=value'\n"
             "                   other possible keys are: debutalbum, publisher, conductor,\n"
             "                   title, subtitle, track, comment, composer, copyright,\n"
             "                   publicationright, filename, recordlocation, recorddate,\n"
             "                   ean/upc, year, releasedate, genre, media, index, isrc,\n"
             "                   abstract, bibliography, introplay, media, language, ...\n"
			 "  --unicode        Toggle unicode input from console on/off if autodetection\n"
			 "                   doesn't work. This switch MUST be before any tag switch.\n"
             "\n" );

    stderr_printf (
             "\033[1m\rAudio processing:\033[0m\n" );
    stderr_printf (
             "  --skip x         skip the first x seconds  (dflt: %3.1f)\n",   SkipTime );
    stderr_printf (
             "  --dur x          stop encoding after at most x seconds of encoded audio\n" );
    stderr_printf (
             "  --fade x         fadein+out in seconds\n" );
    stderr_printf (
             "  --fadein x       fadein  in seconds (dflt: %3.1f)\n",                   FadeInTime );
    stderr_printf (
             "  --fadeout x      fadeout in seconds (dflt: %3.1f)\n",                   FadeOutTime );
    stderr_printf (
             "  --fadeshape x    fade shape (dflt: %3.1f),\n"
             "                   see http://www.uni-jena.de/~pfk/mpc/img/fade.png\n",   FadeShape );
    stderr_printf (
             "  --scale x        scale input signal by x (dflt: %7.5f)\n",              ScalingFactorl );
    stderr_printf (
             "  --scale x,y      scale input signal, separate for each channel\n" );
	stderr_printf (
             "\n" );

    stderr_printf (
             "\033[1m\rExpert settings:\033[0m\n" );
    stderr_printf (
             "==Masking thresholds======\n" );
    stderr_printf (
             "  --quality x      set Quality to x (dflt: 5)\n" );
    stderr_printf (
             "  --nmt x          set NMT value to x dB (dflt: %4.1f)\n", m->NMT );
    stderr_printf (
             "  --tmn x          set TMN value to x dB (dflt: %4.1f)\n", m->TMN );
    stderr_printf (
             "  --pns x          set PNS value to x dB (dflt: %4.1f)\n", m->PNS );
    stderr_printf (
             "==ATH/Bandwidth settings==\n" );
    stderr_printf (
             "  --bw x           maximum bandwidth in Hz (dflt: %4.1f kHz)\n", (m->Max_Band+1)*(m->SampleFreq/32000.) );
    stderr_printf (
             "  --minSMR x       minimum SMR of x dB over encoded bandwidth (dflt: %2.1f)\n",  m->minSMR );
    stderr_printf (
             "  --ltq xyy        x=0: ISO threshold in quiet (not recommended)\n"
             "                   x=1: more sensitive threshold in quiet (Buschmann)\n"
             "                   x=2: even more sensitive threshold in quiet (Filburt)\n"
             "                   x=3: Klemm\n"
             "                   x=4: Buschmann-Klemm Mix\n"
             "                   x=5: minimum of Klemm and Buschmann (dflt)\n"
             "                   y=00...99: HF roll-off (00:+30 dB, 99:-30 dB @20 kHz\n" );
    stderr_printf (
             "  --ltq_gain x     add offset of x dB to chosen ltq (dflt: %+4.1f)\n",       m->Ltq_offset   );
    stderr_printf (
             "  --ltq_max x      maximum level for ltq (dflt: %4.1f dB)\n",                m->Ltq_max      );
    stderr_printf (
			"  --ltq_var x      adaptive threshold in quiet: 0: off, >0: on (dflt: %g)\n", m->varLtq    );
    stderr_printf (
			"  --tmpMask x      exploit postmasking: 0: off, 1: on (dflt: %i)\n",         m->tmpMask_used );
    stderr_printf (
             "==Other settings==========\n" );
    stderr_printf (
             "  --ms x           Mid/Side Stereo, 0: off, 1: reduced, 2: on, 3: decoupled,\n"
             "                   10: enhanced 1.5/3 dB, 11: 2/6 dB, 12: 2.5/9 dB,\n"
             "                   13: 3/12 dB, 15: 3/oo dB (dflt: %i)\n",                        m->MS_Channelmode );
    stderr_printf (
			"  --ans x          Adaptive Noise Shaping Order: 0: off, 1...6: on (dflt: %i)\n", m->NS_Order );
    stderr_printf (
			"  --cvd x          ClearVoiceDetection, 0: off, 1: on, 2: dual (dflt: %i)\n",     m->CVD_used );
    stderr_printf (
			"  --shortthr x     short FFT threshold (dflt: %4.1f)\n",                          m->ShortThr );
    stderr_printf (
             "  --transdet x     slewrate for transient detection (dflt: %3.1f)\n",             m->TransDetect );
    stderr_printf (
             "  --minval x       calculation of MinVal (1:Buschmann, 2,3:Klemm)\n" );
    stderr_printf (
             "\n" );

    stderr_printf (
             "\033[1m\rExamples:\033[0m\n"
             "  mpcenc inputfile.wav\n"
             "  mpcenc inputfile.wav outputfile.mpc\n"
             "  mpcenc --radio inputfile.wav outputfile.mpc\n"
             "  mpcenc --silent --radio --pns 0.25 inputfile.wav outputfile.mpc\n"
             "  mpcenc --nmt 12 --tmn 28 inputfile.wav outputfile.mpc\n"
             "\n");
}


static void
shorthelp ( void )
{
    stderr_printf (
             "\n"
             "\033[1m\rUsage:\033[0m\n"
             "  mpcenc [--options] <Input_File>\n"
             "  mpcenc [--options] <Input_File> <Output_File>\n"
             "\n"

             "\033[1m\rStandard options:\033[0m\n"
             "  --silent         repress console messages                 (dflt: off)\n"
             "  --verbose        increase verbosity                       (dflt: off)\n"
             "  --deleteinput    delete Input_File after encoding         (dflt: off)\n"
             "  --overwrite      overwrite existing Output_File           (dflt: off)\n"
             "  --fade sec       fade in and out with 'sec' duration      (dflt: 0.0)\n"
             "\n"

             "\033[1m\rProfiles and Quality Scale:\033[0m\n"
             "  --thumb          (--quality 3.00)   low/medium quality    (~  90 kbps)\n"
             "  --radio          (--quality 4.00)   medium quality        (~ 130 kbps)\n"
             "  --standard       (--quality 5.00)   high quality  (dflt)  (~ 180 kbps)\n"
             "  --extreme        (--quality 6.00)   excellent quality     (~ 210 kbps)\n"
             "  --insane         (--quality 7.00)   excellent quality     (~ 240 kbps)\n"
             "\n"

             "\033[1m\rExamples:\033[0m\n"
             "  mpcenc inputfile.wav\n"
             "  mpcenc inputfile.wav outputfile.mpc\n"
             "  mpcenc --insane inputfile.wav outputfile.mpc\n"
             "  mpcenc --silent --radio inputfile.wav outputfile.mpc\n"
             "\n"
             "For further information use the --longhelp option.\n" );
}


/*
 *  Wishes for fading:
 *
 *            _____________________
 *           /|                   |\
 *         /  |                   |  \
 *        /   |                   |   \
 *  ____/     |                   |     \______________
 *  |  |      |                   |      |  |
 *  |t1|  t2  |                   |  t4  |t5|
 *  |                 t3                    |
 *     |<-------------- M P C ------------->|
 *  |<-------------------- W A V E ------------------>|
 *
 *   t1: StartTime   (Standard: 0, positive: from beginning of file, negative: from end of file)
 *   t2: FadeInTime  (Standard: 0, positive: Fadetime)
 *   t3: EndTime     (Standard: 0, non-positive: from end of file, positive: from beginning of file)
 *   t4: FadeOutTime (Standard: 0, positive: Fadetime)
 *   t5: PostGapTime (Standard: 0, positive: additional silence)
 *
 * The beginning of phase t4 can also be triggered by the signal SIGINT.
 * With SIGTERM, the current frame is fully decoded and then terminated.
 *
 * Another question is if you can't put t1 before the zero, same with t3 and t5
 * (track-spanning cutting).
 */

float  bump_exp   = 1.f;
float  bump_start = 0.040790618517f;


static void
setbump ( double e )
{
    bump_exp   = e;
    bump_start = 1 - sqrt (1 - 1 / (1 - log(1.e-5) / e));
}


static double
bump ( double x )
{
    x = bump_start + x * (1. - bump_start);
    if ( x <= 0.) return 0.;
    if ( x >= 1.) return 1.;
    x *= (2. - x);
    x  = (x - 1.) / x;
    return exp (x * bump_exp);
}


static void
Fading_In ( PCMDataTyp* data, unsigned int N, const float fs )
{
    float  inv_fs = 1.f / fs;
    float  fadein_pos;
    float  scale;
    int    n;
    int    idx;
    for ( n = 0; n < BLOCK; n++, N++ ) {
        idx           = n + CENTER;
        fadein_pos    = N * inv_fs;
        scale         = fadein_pos / FadeInTime;
        scale         = bump (scale);
        data->L[idx] *= scale;
        data->R[idx] *= scale;
        data->M[idx] *= scale;
        data->S[idx] *= scale;
    }
}


static void
Fading_Out ( PCMDataTyp* data, unsigned int N, const float fs )
{
    float  inv_fs = 1.f / fs;
    float  fadeout_pos;
    float  scale;
    int    n;
    int    idx;
    for ( n = 0; n < BLOCK; n++, N++ ) {
        idx           = n + CENTER;
        fadeout_pos   = (long double)(SamplesInWAVE - N) * inv_fs;
        scale         = fadeout_pos / FadeOutTime;
        scale         = bump (scale);
        data->L[idx] *= scale;
        data->R[idx] *= scale;
        data->M[idx] *= scale;
        data->S[idx] *= scale;
    }
}


static const unsigned char  Penalty [256] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
      0,  2,  5,  9, 15, 23, 36, 54, 79,116,169,246,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

#define P(new,old)  Penalty [128 + (old) - (new)]

static void
SCF_Extraktion ( PsyModel*m, mpc_encoder_t* e, const int MaxBand, SubbandFloatTyp* x )
{
    int    Band;
    int    n;
    int    d01;
    int    d12;
    int    d02;
    int    warnL;
    int    warnR;
    int*   scfL;
    int*   scfR;
    int    comp_L [3];
    int    comp_R [3];
    float  tmp_L  [3];
    float  tmp_R  [3];
    float  facL;
    float  facR;
    float  L;
    float  R;
    float  SL;
    float  SR;

    for ( Band = 0; Band <= MaxBand; Band++ ) {         // Suche nach Maxima
        L  = FABS (x[Band].L[ 0]);
        R  = FABS (x[Band].R[ 0]);
        SL = x[Band].L[ 0] * x[Band].L[ 0];
        SR = x[Band].R[ 0] * x[Band].R[ 0];
        for ( n = 1; n < 12; n++ ) {
            if (L < FABS (x[Band].L[n])) L = FABS (x[Band].L[n]);
            if (R < FABS (x[Band].R[n])) R = FABS (x[Band].R[n]);
            SL += x[Band].L[n] * x[Band].L[n];
            SR += x[Band].R[n] * x[Band].R[n];
        }
        Power_L [Band][0] = SL;
        Power_R [Band][0] = SR;
        tmp_L [0] = L;
        tmp_R [0] = R;

        L  = FABS (x[Band].L[12]);
        R  = FABS (x[Band].R[12]);
        SL = x[Band].L[12] * x[Band].L[12];
        SR = x[Band].R[12] * x[Band].R[12];
        for ( n = 13; n < 24; n++ ) {
            if (L < FABS (x[Band].L[n])) L = FABS (x[Band].L[n]);
            if (R < FABS (x[Band].R[n])) R = FABS (x[Band].R[n]);
            SL += x[Band].L[n] * x[Band].L[n];
            SR += x[Band].R[n] * x[Band].R[n];
        }
        Power_L [Band][1] = SL;
        Power_R [Band][1] = SR;
        tmp_L [1] = L;
        tmp_R [1] = R;

        L  = FABS (x[Band].L[24]);
        R  = FABS (x[Band].R[24]);
        SL = x[Band].L[24] * x[Band].L[24];
        SR = x[Band].R[24] * x[Band].R[24];
        for ( n = 25; n < 36; n++ ) {
            if (L < FABS (x[Band].L[n])) L = FABS (x[Band].L[n]);
            if (R < FABS (x[Band].R[n])) R = FABS (x[Band].R[n]);
            SL += x[Band].L[n] * x[Band].L[n];
            SR += x[Band].R[n] * x[Band].R[n];
        }
        Power_L [Band][2] = SL;
        Power_R [Band][2] = SR;
        tmp_L [2] = L;
        tmp_R [2] = R;

        // calculation of the scalefactor-indexes
        // -12.6f*log10(x)+57.8945021823f = -10*log10(x/32767)*1.26+1
        // normalize maximum of +/- 32767 to prevent quantizer overflow
        // It can stand a maximum of +/- 32768 ...

        // Where is scf{R,L} [0...2] initialized ???
        scfL = e->SCF_Index_L [Band];
		scfR = e->SCF_Index_R [Band];
        if (tmp_L [0] > 0.) scfL [0] = IFLOORF (-12.6f * LOG10 (tmp_L [0]) + 57.8945021823f );
        if (tmp_L [1] > 0.) scfL [1] = IFLOORF (-12.6f * LOG10 (tmp_L [1]) + 57.8945021823f );
        if (tmp_L [2] > 0.) scfL [2] = IFLOORF (-12.6f * LOG10 (tmp_L [2]) + 57.8945021823f );
        if (tmp_R [0] > 0.) scfR [0] = IFLOORF (-12.6f * LOG10 (tmp_R [0]) + 57.8945021823f );
        if (tmp_R [1] > 0.) scfR [1] = IFLOORF (-12.6f * LOG10 (tmp_R [1]) + 57.8945021823f );
        if (tmp_R [2] > 0.) scfR [2] = IFLOORF (-12.6f * LOG10 (tmp_R [2]) + 57.8945021823f );

        // restriction to SCF_Index = -6...121, make note of the internal overflow
        warnL = warnR = 0;
		for( n = 0; n < 3; n++){
			if (scfL[n] < -6) scfL[n] = -6, warnL = 1;
			if (scfL[n] > 121) scfL[n] = 121, warnL = 1;
			if (scfR[n] < -6) scfR[n] = -6, warnR = 1;
			if (scfR[n] > 121) scfR[n] = 121, warnR = 1;
		}

        // save old values for compensation calculation
        comp_L[0] = scfL[0]; comp_L[1] = scfL[1]; comp_L[2] = scfL[2];
        comp_R[0] = scfR[0]; comp_R[1] = scfR[1]; comp_R[2] = scfR[2];

        // determination and replacement of scalefactors of minor differences with the smaller one???
        // a smaller one is quantized more roughly, i.e. the noise gets amplified???

        if ( m->CombPenalities >= 0 ) {
            if      ( P(scfL[0],scfL[1]) + P(scfL[0],scfL[2]) <= m->CombPenalities ) scfL[2] = scfL[1] = scfL[0];
            else if ( P(scfL[1],scfL[0]) + P(scfL[1],scfL[2]) <= m->CombPenalities ) scfL[0] = scfL[2] = scfL[1];
            else if ( P(scfL[2],scfL[0]) + P(scfL[2],scfL[1]) <= m->CombPenalities ) scfL[0] = scfL[1] = scfL[2];
            else if ( P(scfL[0],scfL[1])                      <= m->CombPenalities ) scfL[1] = scfL[0];
            else if ( P(scfL[1],scfL[0])                      <= m->CombPenalities ) scfL[0] = scfL[1];
            else if ( P(scfL[1],scfL[2])                      <= m->CombPenalities ) scfL[2] = scfL[1];
            else if ( P(scfL[2],scfL[1])                      <= m->CombPenalities ) scfL[1] = scfL[2];

            if      ( P(scfR[0],scfR[1]) + P(scfR[0],scfR[2]) <= m->CombPenalities ) scfR[2] = scfR[1] = scfR[0];
            else if ( P(scfR[1],scfR[0]) + P(scfR[1],scfR[2]) <= m->CombPenalities ) scfR[0] = scfR[2] = scfR[1];
            else if ( P(scfR[2],scfR[0]) + P(scfR[2],scfR[1]) <= m->CombPenalities ) scfR[0] = scfR[1] = scfR[2];
            else if ( P(scfR[0],scfR[1])                      <= m->CombPenalities ) scfR[1] = scfR[0];
            else if ( P(scfR[1],scfR[0])                      <= m->CombPenalities ) scfR[0] = scfR[1];
            else if ( P(scfR[1],scfR[2])                      <= m->CombPenalities ) scfR[2] = scfR[1];
            else if ( P(scfR[2],scfR[1])                      <= m->CombPenalities ) scfR[1] = scfR[2];
        }
        else {

            d12  = scfL [2] - scfL [1];
            d01  = scfL [1] - scfL [0];
            d02  = scfL [2] - scfL [0];

            if      ( 0 < d12  &&  d12 < 5 ) scfL [2] = scfL [1];
            else if (-3 < d12  &&  d12 < 0 ) scfL [1] = scfL [2];
            else if ( 0 < d01  &&  d01 < 5 ) scfL [1] = scfL [0];
            else if (-3 < d01  &&  d01 < 0 ) scfL [0] = scfL [1];
            else if ( 0 < d02  &&  d02 < 4 ) scfL [2] = scfL [0];
            else if (-2 < d02  &&  d02 < 0 ) scfL [0] = scfL [2];

            d12  = scfR [2] - scfR [1];
            d01  = scfR [1] - scfR [0];
            d02  = scfR [2] - scfR [0];

            if      ( 0 < d12  &&  d12 < 5 ) scfR [2] = scfR [1];
            else if (-3 < d12  &&  d12 < 0 ) scfR [1] = scfR [2];
            else if ( 0 < d01  &&  d01 < 5 ) scfR [1] = scfR [0];
            else if (-3 < d01  &&  d01 < 0 ) scfR [0] = scfR [1];
            else if ( 0 < d02  &&  d02 < 4 ) scfR [2] = scfR [0];
            else if (-2 < d02  &&  d02 < 0 ) scfR [0] = scfR [2];
        }

        // calculate SNR-compensation
        tmp_L [0]         = invSCF [comp_L[0] - scfL[0]];
        tmp_L [1]         = invSCF [comp_L[1] - scfL[1]];
        tmp_L [2]         = invSCF [comp_L[2] - scfL[2]];
        tmp_R [0]         = invSCF [comp_R[0] - scfR[0]];
        tmp_R [1]         = invSCF [comp_R[1] - scfR[1]];
        tmp_R [2]         = invSCF [comp_R[2] - scfR[2]];
        m->SNR_comp_L [Band] = (tmp_L[0]*tmp_L[0] + tmp_L[1]*tmp_L[1] + tmp_L[2]*tmp_L[2]) * 0.3333333333f;
        m->SNR_comp_R [Band] = (tmp_R[0]*tmp_R[0] + tmp_R[1]*tmp_R[1] + tmp_R[2]*tmp_R[2]) * 0.3333333333f;

        // normalize the subband samples
        facL = invSCF[scfL[0]];
        facR = invSCF[scfR[0]];
        for ( n = 0; n < 12; n++ ) {
            x[Band].L[n] *= facL;
            x[Band].R[n] *= facR;
        }
        facL = invSCF[scfL[1]];
        facR = invSCF[scfR[1]];
        for ( n = 12; n < 24; n++ ) {
            x[Band].L[n] *= facL;
            x[Band].R[n] *= facR;
        }
        facL = invSCF[scfL[2]];
        facR = invSCF[scfR[2]];
        for ( n = 24; n < 36; n++ ) {
            x[Band].L[n] *= facL;
            x[Band].R[n] *= facR;
        }

        // limit to +/-32767 if internal clipping
        if ( warnL )
            for ( n = 0; n < 36; n++ ) {
                if      (x[Band].L[n] > +32767.f) {
                    e->Overflows++;
                    MaxOverFlow = maxf (MaxOverFlow,  x[Band].L[n]);
                    x[Band].L[n] = 32767.f;
                }
                else if (x[Band].L[n] < -32767.f) {
					e->Overflows++;
                    MaxOverFlow = maxf (MaxOverFlow, -x[Band].L[n]);
                    x[Band].L[n] = -32767.f;
                }
            }
        if ( warnR )
            for ( n = 0; n < 36; n++ ) {
                if      (x[Band].R[n] > +32767.f) {
					e->Overflows++;
                    MaxOverFlow = maxf (MaxOverFlow,  x[Band].R[n]);
                    x[Band].R[n] = 32767.f;
                }
                else if (x[Band].R[n] < -32767.f) {
					e->Overflows++;
                    MaxOverFlow = maxf (MaxOverFlow, -x[Band].R[n]);
                    x[Band].R[n] = -32767.f;
                }
            }
    }
    return;
}


static void
Quantisierung ( PsyModel * m,
				const int               MaxBand,
                const int*              resL,
                const int*              resR,
                const SubbandFloatTyp*  subx,
				mpc_quantizer*          subq )
{
    static float  errorL [32] [36 + MAX_NS_ORDER];
    static float  errorR [32] [36 + MAX_NS_ORDER];
    int           Band;

    // quantize Subband- and Subframe-samples
    for ( Band = 0; Band <= MaxBand; Band++, resL++, resR++ ) {

        if ( *resL > 0 ) {
            if ( m->NS_Order_L [Band] > 0 ) {
                QuantizeSubbandWithNoiseShaping ( subq[Band].L, subx[Band].L, *resL, errorL [Band], m->FIR_L [Band] );
                memcpy ( errorL [Band], errorL[Band] + 36, MAX_NS_ORDER * sizeof (**errorL) );
            } else {
				QuantizeSubband                 ( subq[Band].L, subx[Band].L, *resL, errorL [Band], MAX_NS_ORDER );
                memcpy ( errorL [Band], errorL[Band] + 36, MAX_NS_ORDER * sizeof (**errorL) );
            }
        }

        if ( *resR > 0 ) {
            if ( m->NS_Order_R [Band] > 0 ) {
                QuantizeSubbandWithNoiseShaping ( subq[Band].R, subx[Band].R, *resR, errorR [Band], m->FIR_R [Band] );
                memcpy ( errorR [Band], errorR [Band] + 36, MAX_NS_ORDER * sizeof (**errorL) );
            } else {
				QuantizeSubband                 ( subq[Band].R, subx[Band].R, *resR, errorL [Band], MAX_NS_ORDER);
                memcpy ( errorR [Band], errorR [Band] + 36, MAX_NS_ORDER * sizeof (**errorL) );
            }
        }
    }
    return;
}


static int
PNS_SCF ( int* scf, float S0, float S1, float S2 )
{
//    printf ("%7.1f %7.1f %7.1f  ", sqrt(S0/12), sqrt(S1/12), sqrt(S2/12) );

#if 1
    if ( S0 < 0.5 * S1  ||  S1 < 0.5 * S2  ||  S0 < 0.5 * S2 )
        return 0;

    if ( S1 < 0.25 * S0  ||  S2 < 0.25 * S1  ||  S2 < 0.25 * S0 )
        return 0;
#endif


    if ( S0 >= 0.8 * S1 ) {
        if ( S0 >= 0.8 * S2  &&  S1 > 0.8 * S2 )
            S0 = S1 = S2 = 0.33333333333f * (S0 + S1 + S2);
        else
            S0 = S1 = 0.5f * (S0 + S1);
    }
    else {
        if ( S1 >= 0.8 * S2 )
            S1 = S2 = 0.5f * (S1 + S2);
    }

    scf [0] = scf [1] = scf [2] = 63;
    S0 = sqrt (S0/12 * 4/1.2005080577484075047860806747022);
    S1 = sqrt (S1/12 * 4/1.2005080577484075047860806747022);
    S2 = sqrt (S2/12 * 4/1.2005080577484075047860806747022);
    if (S0 > 0.) scf [0] = IFLOORF (-12.6f * LOG10 (S0) + 57.8945021823f );
    if (S1 > 0.) scf [1] = IFLOORF (-12.6f * LOG10 (S1) + 57.8945021823f );
    if (S2 > 0.) scf [2] = IFLOORF (-12.6f * LOG10 (S2) + 57.8945021823f );

    if ( scf[0] & ~63 ) scf[0] = scf[0] > 63 ? 63 : 0;
    if ( scf[1] & ~63 ) scf[1] = scf[1] > 63 ? 63 : 0;
    if ( scf[2] & ~63 ) scf[2] = scf[2] > 63 ? 63 : 0;

    return 1;
}


static void
Allocate ( const int MaxBand, int* res, float* x, int* scf, const float* comp, const float* smr, const SCFTriple* Pow, const int* Transient, const float PNS )
{
    int    Band;
    int    k;
    float  tmpMNR;      // to adjust the scalefactors
    float  save [36];   // to adjust the scalefactors
    float  MNR;         // Mask-to-Noise ratio

    for ( Band = 0; Band <= MaxBand; Band++, res++, comp++, smr++, scf += 3, x += 72 ) {
        // printf ( "%2u: %u\n", Band, Transient[Band] );

        // Find out needed quantization resolution Res to fulfill the calculated MNR
        // This is done by exactly measuring the quantization residuals against the signal itself
        // Starting with Res=1  Res in increased until MNR becomes less than 1.
        if ( Band > 0  &&  res[-1] < 3  &&  *smr >= 1. &&  *smr < Band * PNS  &&
             PNS_SCF ( scf, Pow [Band][0], Pow [Band][1], Pow [Band][2] ) ) {
            *res = -1;
        } else {
            for ( MNR = *smr * 1.; MNR > 1.  &&  *res != 15; )
                MNR = *smr * (Transient[Band] ? ISNR_Schaetzer_Trans : ISNR_Schaetzer) ( x, *comp, ++*res );
        }

        // Fine adapt SCF's (MNR > 0 prevents adaption of zero samples, which is nonsense)
        // only apply to Huffman-coded samples (otherwise no savings in bitrate)
        if ( *res > 0  &&  *res <= LAST_HUFFMAN  &&  MNR < 1.  &&  MNR > 0.  &&  !Transient[Band] ) {
            while ( scf[0] > 0  &&  scf[1] > 0  &&  scf[2] > 0 ) {

                --scf[2]; --scf[1]; --scf[0];                   // adapt scalefactors and samples
                memcpy ( save, x, sizeof save );
                for (k = 0; k < 36; k++ )
                    x[k] *= SCFfac;

                tmpMNR = *smr * (Transient[Band] ? ISNR_Schaetzer_Trans : ISNR_Schaetzer) ( x, *comp, *res );// recalculate MNR

                // FK: if ( tmpMNR > MNR  &&  tmpMNR <= 1 ) {          // check for MNR
                if ( tmpMNR <= 1 ) {                            // check for MNR
                    MNR = tmpMNR;
                }
                else {
                    ++scf[0]; ++scf[1]; ++scf[2];               // restore scalefactors and samples
                    memcpy ( x, save, sizeof save );
                    break;
                }
            }
        }

    }
    return;
}




/* Planned: return the evaluated options, without InputFile and OutputFile, argc implicit instead of explicit */

static int
EvalParameters (PsyModel * m, int argc, char** argv, char** InputFile, char** OutputFile, int onlyfilenames )
{
    int          k;
    size_t       len;
    static char  output [2048];
    static char  errmsg [] = "\n\033[33;41;1mERROR\033[0m: Missing argument for option '--%s'\n\n";
    FILE*        fp;
    char*        p;
    char         buff [32768];

    /********************************* In / Out Files *********************************/
    *InputFile  = argv [argc-1];
    *OutputFile = NULL;

    // search for output file
    if ( argc >= 3 ) {
        len = strlen (argv[argc-1]);

        if ( strcmp (argv[argc-1], "/dev/null") == 0  ||
             strcmp (argv[argc-1], "-")         == 0  ||
             (len >= 4  &&  (0 == strcasecmp (argv [argc-1] + len - 4, ".MPC")  ||
                             0 == strcasecmp (argv [argc-1] + len - 4, ".MPP")  ||
                             0 == strcasecmp (argv [argc-1] + len - 4, ".MP+"))) ) {
            *OutputFile = argv[argc-1];
            *InputFile  = argv[argc-2];
            argc -= 2;
        }
    }

    // if no Output-File is stated, set OutFile to InFile.mpc
    if ( *OutputFile == NULL  ) {
	    char * ext = strrchr(*InputFile, '.'); // search for extension delimiter
        strcpy ( *OutputFile = output, *InputFile );
        len = strlen ( output );
		if (ext != 0 && (len - (ext - *InputFile)) < 7) // extension max size (5 chars)
			len = ext - *InputFile;
        strcpy (output+len, ".mpc");
        argc -= 1;
    }

    if ( onlyfilenames )
        return 0;

    /********************************* In / Out Files *********************************/


    // search for options
    for ( k = 1; k < argc; k++ ) {

        const char*  arg = argv [k];

        if ( arg[0] != '-'  ||  arg[1] != '-' )
            continue;
        arg += 2;

        if      ( 0 == strcmp ( arg, "verbose" ) ) {                                     // verbose
            verbose++;
        }
        else if ( 0 == strcmp ( arg, "telephone" ) ) {                                   // MainQual
            SetQualityParams (m, 2.0);
        }
        else if ( 0 == strcmp ( arg, "thumb" ) ) {                                       // MainQual
            SetQualityParams (m, 3.0);
        }
        else if ( 0 == strcmp ( arg, "radio"   ) ) {
            SetQualityParams (m, 4.0);
        }
        else if ( 0 == strcmp ( arg, "standard")  ||  0 == strcmp ( arg, "normal") ) {
            SetQualityParams (m, 5.0);
        }
        else if ( 0 == strcmp ( arg, "xtreme")  ||  0 == strcmp ( arg, "extreme") ) {
            SetQualityParams (m, 6.0);
        }
        else if ( 0 == strcmp ( arg, "insane") ) {
            SetQualityParams (m, 7.0);
        }
        else if ( 0 == strcmp ( arg, "braindead") ) {
            SetQualityParams (m, 8.0);
        }
        else if ( 0 == strcmp ( arg, "quality") ) {                                      // Quality
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            SetQualityParams (m, atof (argv[k]) );
        }
        else if ( 0 == strcmp ( arg, "neveroverwrite") ) {                              // NeverOverWrite
            WriteMode = MODE_NEVER_OVERWRITE;
        }
        else if ( 0 == strcmp ( arg, "forcewrite")  ||  0 == strcmp ( arg, "overwrite") ) { // ForceWrite
            WriteMode = MODE_OVERWRITE;
        }
        else if ( 0 == strcmp ( arg, "interactive")  ) {                                // Interactive
            WriteMode = MODE_ASK_FOR_OVERWRITE;
        }
        else if ( 0 == strcmp ( arg, "delinput")  ||  0 == strcmp ( arg, "delete")  ||  0 == strcmp ( arg, "deleteinput" ) ) {                                    // DelInput
            DelInput = 0xAFFEDEAD;
        }
        else if ( 0 == strcmp ( arg, "beep")  ) {
            IsEndBeep = MPC_TRUE;
        }
        else if ( 0 == strcmp ( arg, "scale") ) {                                       // ScalingFactor
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            ScalingFactorl = ScalingFactorr = (float) atof (argv[k]);
            if (strchr (argv[k], ','))
                ScalingFactorr = (float) atof (strchr (argv[k], ',') + 1);
            if ( ScalingFactorl == 0.97f  ||  ScalingFactorl == 0.98f ) stderr_printf ("--scale 0.97 or --scale 0.98 is nearly useless to prevent clipping. Use replaygain tool\nto determine EXACT attenuation to avoid clipping. Factor can be between 0.696 and 1.000.\nSee \"http://www.uni-jena.de/~pfk/mpp/clipexample.html\".\n\n" );
        }
        else if ( 0 == strcmp ( arg, "kbd") ) {                                       // ScalingFactor
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
			if ( 2 != sscanf ( argv[k], "%f,%f", &(m->KBD1), &(m->KBD2) ))
                { stderr_printf ( "%s: missing two arguments", arg ); return -1; }
            Init_FFT (m);
        }
        else if ( 0 == strcmp ( arg, "fadein") ) {                                      // FadeInTime
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            FadeInTime = (float) atof (argv[k]);
            if ( FadeInTime < 0.f ) FadeInTime = 0.f;
        }
        else if ( 0 == strcmp ( arg, "fadeout") ) {                                     // FadeOutTime
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            FadeOutTime = (float) atof (argv[k]);
            if ( FadeOutTime < 0.f ) FadeOutTime = 0.f;
        }
        else if ( 0 == strcmp ( arg, "fade") ) {                                        // FadeInTime + FadeOutTime
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            FadeOutTime = (float) atof (argv[k]);
            if ( FadeOutTime < 0.f ) FadeOutTime = 0.f;
            FadeInTime = FadeOutTime;
        }
        else if ( 0 == strcmp ( arg, "fadeshape") ) {                                   // FadeOutTime
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            FadeShape = (float) atof (argv[k]);
            if ( FadeShape < 0.001f  ||  FadeShape > 1000.f ) FadeShape = 1.f;
            setbump ( FadeShape );
        }
        else if ( 0 == strcmp ( arg, "skip")  ||  0 == strcmp ( arg, "start") ) {       // SkipTime
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            SkipTime = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "dur")  ||  0 == strcmp ( arg, "duration") ) {     // maximum Duration
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            Duration = atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "ans") ) {                                         // AdaptiveNoiseShaping
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->NS_Order = atoi (argv[k]);
            m->NS_Order = mini ( m->NS_Order, MAX_NS_ORDER );
        }
        else if ( 0 == strcmp ( arg, "predict") ) {                                     // AdaptiveNoiseShaping
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            PredictionBands = atoi (argv[k]);
            PredictionBands = mini ( PredictionBands, 32 );
        }
        else if ( 0 == strcmp ( arg, "ltq_var")  ||  0 == strcmp ( arg, "ath_var") ) {  // ltq_var
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->varLtq = atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "pns") ) {                                         // pns
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->PNS = atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "minval") ) {                                      // MinValChoice
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->MinValChoice = atoi (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "transdet") ) {                                    // TransDetect
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->TransDetect = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "shortthr") ) {                                    // ShortThr
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->ShortThr = (float) atof (argv[k]);
        }
	    else if ( 0 == strcmp ( arg, "xlevel") || 0 == strcmp ( arg, "noxlevel") ) {
		    stderr_printf ( "\nXlevel coding not needed anymore, --%s ignored.\n", arg );
	    }
        else if ( 0 == strcmp ( arg, "nmt") ) {                                         // NMT
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg );  return -1; }
            m->NMT = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "tmn") ) {                                         // TMN
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg );  return -1; }
            m->TMN = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "cvd") ) {                                         // ClearVoiceDetection
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->CVD_used = atoi (argv[k]);
            if ( m->CVD_used == 0 )
                stderr_printf ( "\nDisabling CVD always reduces quality!\a\n" );
        }
        else if ( 0 == strcmp ( arg, "ms") ) {                                          // Mid/Side Stereo
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->MS_Channelmode = atoi (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "minSMR") ) {                                      // minimum SMR
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            if ( m->minSMR > (float) atof (argv[k]) )
                stderr_printf ( "This option usage may reduces quality!\a\n" );
            m->minSMR = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "tmpMask") ) {                                     // temporal post-masking
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->tmpMask_used = atoi (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "ltq_max")  ||  0 == strcmp ( arg, "ath_max") ) {  // Maximum for threshold in quiet
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg );  return -1; }
			m->Ltq_max = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "ltq_gain")  ||  0 == strcmp ( arg, "ath_gain") ) {// Offset for threshold in quiet
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
			m->Ltq_offset = (float) atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "silent")  ||  0 == strcmp ( arg, "quiet") ) {
            SetStderrSilent (1);
        }
        else if ( 0 == strcmp ( arg, "stderr") ) {                                      // Offset for threshold in quiet
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            freopen ( argv[k], "a", stderr );
        }
        else if ( 0 == strcmp ( arg, "ltq")  ||  0 == strcmp ( arg, "ath") ) {          // threshold in quiet
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->EarModelFlag = atoi (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "noco") ) {
            NoiseInjectionComp ();
        }
        else if ( 0 == strcmp ( arg, "newcomb") ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            m->CombPenalities = atoi (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "ape1") ) {                                     // Mark APE as APE 1.000
            APE_Version = 1000;
        }
        else if ( 0 == strcmp ( arg, "ape2") ) {                                     // Mark APE as APE 2.000
            APE_Version = 2000;
        }
        else if ( 0 == strcmp ( arg, "unicode") ) {                                  // no tag conversion
            NoUnicode = 1 - NoUnicode;
		}
		else if ( 0 == strcmp ( arg, "no_ei") ) {                                  // no encoder info block
			NoEncoderInfo = 1;
		}
		else if ( 0 == strcmp ( arg, "no_st") ) {                                  // no seek table
			NoSeekTable = 1;
		}
		else if ( 0 == strcmp ( arg, "num_frames") ) {
			if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
			FramesBlockPwr = atoi (argv[k]) * 2;
		}
		else if ( 0 == strcmp ( arg, "seek_distance") ) {
			if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
			SeekDistance = atoi (argv[k]);
		}
        else if ( 0 == strcmp ( arg, "lowdelay") ) {
            LowDelay = 1;
        }
        else if ( 0 == strcmp ( arg, "bw")  ||  0 == strcmp ( arg, "lowpass") ) {       // bandwidth
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
			m->BandWidth = atof (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "displayupdatetime") ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            DisplayUpdateTime = atoi (argv[k]);
        }
        else if ( 0 == strcmp ( arg, "artist" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Artist", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "album" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Album", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "debutalbum" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Debut Album", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "publisher" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Publisher", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "conductor" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Conductor", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "title" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Title", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "subtitle" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Subtitle", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "track" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Track", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "comment" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Comment", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "composer" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Composer", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "copyright" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Copyright", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "publicationright" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Publicationright", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "filename" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "File", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "recordlocation" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Record Location", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "recorddate" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Record Date", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "ean/upc" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "EAN/UPC", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "year" )  ||  0 == strcmp ( arg, "releasedate") ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Year", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "genre" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Genre", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "media" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Media", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "index" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Index", 0, p, strlen(p), NoUnicode*3, 0 );
        }
        else if ( 0 == strcmp ( arg, "isrc" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "ISRC", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "abstract" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Abstract", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "bibliography" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Bibliography", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "introplay" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Introplay", 0, p, strlen(p), NoUnicode*3, 0 );
        }
        else if ( 0 == strcmp ( arg, "media" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = argv[k];
            addtag ( "Media", 0, p, strlen(p), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "tag" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = strchr ( argv[k], '=' );
            if ( p == NULL )
                addtag ( argv[k], strlen(argv[k]), "", 0, NoUnicode, 0 );
            else
                addtag ( argv[k], p-argv[k], p+1, strlen(p+1), NoUnicode, 0 );
        }
        else if ( 0 == strcmp ( arg, "tagfile" ) ) {
            if ( ++k >= argc ) { stderr_printf ( errmsg, arg ); return -1; }
            p = strchr ( argv[k], '=' );
            if ( p == NULL ) {
                stderr_printf (" Enter value for tag key '%s': ", argv[k] );
                fgets ( buff, sizeof buff, stdin );
                len = strlen (buff);
                while ( len > 0  &&  (buff [len-1] == '\r'  ||  buff [len-1] == '\n') )
                    len--;
                addtag ( arg, strlen(arg), buff, len, NoUnicode*6, 0 );
            }
            else {
                fp = fopen ( p+1, "rb" );
                if ( fp == NULL ) {
                    fprintf ( stderr, "Can't open file '%s'.\n", p+1 );
                }
                else {
                    addtag ( argv[k], p-argv[k], buff, fread (buff,1,sizeof buff,fp), NoUnicode*2, 3 );
                    fclose (fp);
                }
            }
        }
        else {
            stderr_printf ( "\n\033[33;41;1mERROR\033[0m: unknown option '--%s' !\n", arg );
	        shorthelp();
            return -1;
        }
    }

    TestProfileParams (m);
    return 0;
}


static void
ShowParameters (PsyModel * m, char* inDatei, char* outDatei )
{
    static const char        unk      []       = "???";
    static const char*       EarModel []       = { "ISO (bad!!!)", "Busch", "Filburt", "Klemm", "Klemm/Busch mix", "min(Klemm,Busch)" };
    static const char        th       [ 7] [4] = { "no", "1st", "2nd", "3rd", "4th", "5th", "6th" };
    static const char        able     [ 3] [9] = { "Disabled", "Enabled", "Dual" };
    static const char*       stereo   [16]     = {
        "Simple uncoupled Stereo",
        "Mid/Side Stereo + Intensity Stereo 2 bit",
        "Mid/Side Stereo + Intensity Stereo 4 bit",
        "Mid/Side Stereo, destroyed imaging (unusable)",
        "Mid/Side Stereo, much reduced imaging",
        "Mid/Side Stereo, reduced imaging (-3 dB)",
        "Mid/Side Stereo when superior",
        unk, unk, unk,
        "Mid/Side Stereo when superior + enhanced (1.5/3 dB)",
        "Mid/Side Stereo when superior + enhanced (2/6 dB)",
        "Mid/Side Stereo when superior + enhanced (2.5/9 dB)",
        "Mid/Side Stereo when superior + enhanced (3/12 dB)",
        unk,
        "Mid/Side Stereo when superior + enhanced (3/oo dB)"
    };
    static const char* const Profiles [16]     = {
        "n.a", "Unstable/Experimental", unk, unk, unk, "below Telephone", "below Telephone", "Telephone",
        "Thumb", "Radio", "Standard", "Xtreme", "Insane", "BrainDead", "above BrainDead", "above BrainDead"
    };

    stderr_printf ( "\n"
                    " encoding file '%s'\n"
                    "       to file '%s'\n"
                    "\n"
                    " SV %u, Profile '%s'\n",
                    inDatei, outDatei, 8, Profiles [m->MainQual] );

    if ( verbose > 0 ) {
        stderr_printf ( "\n" );
        if ( FadeInTime != 0.  ||  FadeOutTime != 0.  ||  verbose > 1 )
            stderr_printf ( " PCM fader                : fade-in: %.2f s, fade-out: %.2f s, shape: %g\n", FadeInTime, FadeOutTime, FadeShape );
        if ( ScalingFactorr != 1.  ||  ScalingFactorl != 1.  ||  verbose > 1 )
            stderr_printf ( " Scaling input by         : left %.5f, right: %.5f\n", ScalingFactorl, ScalingFactorr );
        stderr_printf ( " Maximum encoded bandwidth: %4.1f kHz\n", (m->Max_Band+1) * (m->SampleFreq/32./2000.) );
        stderr_printf ( " Adaptive Noise Shaping   : max. %s order\n", th [m->NS_Order] );
        stderr_printf ( " Clear Voice Detection    : %s\n", able [m->CVD_used] );
        stderr_printf ( " Mid/Side Stereo          : %s\n", stereo [m->MS_Channelmode] );
        stderr_printf ( " Threshold of Hearing     : Model %3u: %s, Max ATH: %2.0f dB, Offset: %+1.0f dB, +Offset@20 kHz:%3.0f dB\n",
                        m->EarModelFlag,
						m->EarModelFlag/100 < sizeof(EarModel)/sizeof(*EarModel) ? EarModel [m->EarModelFlag/100] : unk,
                        m->Ltq_max,
                        m->Ltq_offset,
                        -0.6 * (int) (m->EarModelFlag % 100 - 50) );
		if ( m->NMT !=  6.5 || verbose > 1 )
			stderr_printf ( " Noise masks Tone Ratio   : %4.1f dB\n", m->NMT );
		if ( m->TMN != 18.0 || verbose > 1 )
			stderr_printf ( " Tone masks Noise Ratio   : %4.1f dB\n", m->TMN );
        if ( m->PNS > 0 )
            stderr_printf ( " PNS Threshold            : %4.2f\n", m->PNS );
		if ( !m->tmpMask_used )
            stderr_printf ( " No exploitation of temporal post masking\n" );
        else if ( verbose > 1 )
            stderr_printf ( " Exploitation of temporal post masking\n" );
		if ( m->minSMR > 0. )
			stderr_printf ( " Minimum Signal-to-Mask   : %4.1f dB\n", m->minSMR );
        else if ( verbose > 1 )
            stderr_printf ( " No minimum SMR (psycho model controlled filtering)\n" );
        if ( DelInput == 0xAFFEDEAD )
            stderr_printf ( " Deleting input file after (successful) encoding\n" );
        else if ( verbose > 1 )
            stderr_printf ( " No deleting of input file after encoding\n" );
    }
    stderr_printf ( "\n" );
}


/*
 *  Print out the time to stderr with a precision of 10 ms always using
 *  12 characters. Time is represented by the sample count. An additional
 *  prefix character (normally ' ' or '-') is prepended before the first
 *  digit.
 */

static const char*
PrintTime ( PsyModel* m, mpc_uint64_t samples, int sign )
{
    static char  ret [32];
	mpc_uint32_t tmp  = (mpc_uint32_t) ( (long double)(samples) * 100. / m->SampleFreq );
    mpc_uint_t   hour = (mpc_uint_t)  ( tmp / 360000     );
    mpc_uint_t   min  = (mpc_uint_t)  ( tmp / 6000 %  60 );
    mpc_uint_t   sec  = (mpc_uint_t)  ( tmp / 100  %  60 );
    mpc_uint_t   csec = (mpc_uint_t)  ( tmp        % 100 );


	if ( (long double)(samples) >= m->SampleFreq * 360000. )
        return "            ";
    else if ( hour > 9 )
        sprintf ( ret,  "%c%2u:%02u", sign, hour, min );
    else if ( hour > 0 )
        sprintf ( ret, " %c%1u:%02u", sign, hour, min );
    else if ( min  > 9 )
        sprintf ( ret,    "   %c%2u", sign,       min );
    else
        sprintf ( ret,   "    %c%1u", sign,       min );

    sprintf ( ret + 6,   ":%02u.%02u", sec, csec );
    return ret;
}


static void
ShowProgress ( PsyModel* m,
			   mpc_uint64_t  samples,
               mpc_uint64_t  total_samples,
               mpc_uint64_t  databits )
{
    static clock_t  start;
    clock_t         curr;
    float           percent;
    float           kbps;
    float           speed;
    float           total_estim;

    if ( samples == 0 ) {
        if ( DisplayUpdateTime >= 0 ) {
            stderr_printf ("    %%|avg.bitrate| speed|play time (proc/tot)| CPU time (proc/tot)| ETA\n"
                            "  -.-    -.- kbps  -.--x     -:--.-    -:--.-     -:--.-    -:--.-     -:--.-\r" );
        }
        start = clock ();
        return;
    }
    curr    = clock ();
    if ( curr == start )
        return;

    percent     = 100.f    * (long double)(samples) / (long double)(total_samples);
	kbps        =   1.e-3f * (long double)(databits) * m->SampleFreq / (long double)(samples);
	speed       =   1.f    * (long double)(samples) * (CLOCKS_PER_SEC / m->SampleFreq) / (unsigned long)(curr - start) ;
    total_estim =   1.f    * (long double)(total_samples) / (long double)(samples) * (unsigned long)(curr - start);

    // progress percent
	if ( total_samples < mpc_int64_max )
        stderr_printf ("\r%5.1f ", percent );
    else
        stderr_printf ("\r      " );

    // average data rate
    stderr_printf ( "%6.1f kbps ", kbps );

    // encoder speed
    stderr_printf ( "%5.2fx ", speed );

    // 2x duration in WAVE file time (encoded/total)
    stderr_printf ("%10.10s" , PrintTime ( m, samples      , (char)' ')+1 );
    stderr_printf ("%10.10s ", PrintTime ( m, total_samples, (char)' ')+1 );

    // 2x coding time (encoded/total)
	stderr_printf ("%10.10s" , PrintTime ( m, (curr - start) * (m->SampleFreq/CLOCKS_PER_SEC), (char)' ')+1 );
	stderr_printf ("%10.10s ", PrintTime ( m, total_estim    * (m->SampleFreq/CLOCKS_PER_SEC), (char)' ')+1 );

    // ETA
	stderr_printf ( "%10.10s\r", samples < total_samples  ?  PrintTime (m, (total_estim - curr + start) * (m->SampleFreq/CLOCKS_PER_SEC), (char)' ')+1  :  "" );
    fflush ( stderr );

    if ( WIN32_MESSAGES  &&  FrontendPresent )
        SendProgressMessage ( kbps, speed, percent );
}


static int
myfeof ( FILE* fp )
{
    int  ch;

    if ( fp != (FILE*)-1 )
        return feof (fp);

    ch = CheckKeyKeep ();
    if ( ch == 'q'  ||  ch == 'Q' )
        return 1;
    return 0;
}

static void fill_float(float * buffer,float val,unsigned count)
{
	unsigned n;
	for(n=0;n<count;n++) buffer[n] = val;
}


static void OverdriveReport ( mpc_encoder_t * e )
{
	if ( e->Overflows > 0 ) {                                                // report internal clippings
		stderr_printf ( "\n"
				"\033[1m\rWARNING:\n"
				"\033[0m\r  There still occured %u SCF clippings.\n"
				"  Use the '--scale' method to avoid additional distortions. Note that this\n"
				"  file already has annoying distortions due to slovenly CD mastering.\a\n\n", e->Overflows );
	}
}

// FIXME : not sure if it's a good idea
static void Init_FPU ( void )
{
	mpc_uint16_t  cw;

#if   defined __i386__  &&  defined _FPU_GETCW  &&  defined _FPU_SETCW
    _FPU_GETCW ( cw );
    cw  &=  ~0x300;
	_FPU_SETCW ( cw );
#elif defined __i386__  &&  defined  FPU_GETCW  &&  defined  FPU_SETCW
    FPU_GETCW ( cw );
    cw  &=  ~0x300;
	FPU_SETCW ( cw );
#elif defined __MINGW32__
    __asm__ ("fnstcw %0" : "=m" (*&cw));
    cw  &=  ~0x300;
	__asm__ ("fldcw %0" : : "m" (*&cw));
#elif defined(_WIN32) && !defined(_WIN64)
    _asm { fstcw cw };
    cw  &=  ~0x300;
	_asm { fldcw cw };
#endif
}

static FILE * OpenStream(char * OutputName)
{
	FILE * OutputFile = NULL;

	/* open bitstream file */
	if      ( 0 == strcmp ( OutputName, "/dev/null") )
		OutputFile = fopen (DEV_NULL, "wb");
	else if ( 0 == strcmp ( OutputName, "-")  ||  0 == strcmp ( OutputName, "/dev/stdout") )
		OutputFile = SETBINARY_OUT (stdout);
	else
		switch ( WriteMode ) {
			default:
				stderr_printf ( "\033[33;41;1mERROR\033[0m: Invalid Write mode, internal error\n" );
				exit(1);
			case MODE_NEVER_OVERWRITE:
				OutputFile = fopen ( OutputName, "rb" );
				if ( OutputFile != NULL ) {
					fclose ( OutputFile );
					stderr_printf ( "\033[33;41;1mERROR\033[0m: Output file '%s' already exists\n", OutputName );
					exit(1);
				}
				OutputFile = fopen ( OutputName, "w+b" );
				break;
			case MODE_OVERWRITE:
				OutputFile = fopen ( OutputName, "w+b" );
				break;
			case MODE_ASK_FOR_OVERWRITE:
				OutputFile = fopen ( OutputName, "rb" );
				if ( OutputFile != NULL ) {
					char c;
					fclose ( OutputFile );
					stderr_printf ( "\nmpcenc: Output file '%s' already exists, overwrite (Y/n)? ", OutputName );
					c = waitkey ();
					if ( c != 'Y'  &&  c != 'y' ) {
						stderr_printf ( "No!!!\n\n*** Canceled overwrite ***\n" );
						exit(1);
					}
					stderr_printf ( " YES\n" );
				}
				OutputFile = fopen ( OutputName, "w+b" );
				break;
		}

	if ( OutputFile == NULL ) {
		stderr_printf ( "\033[33;41;1mERROR\033[0m: Could not create output file '%s'\n", OutputName );
		exit(1);
	}
	return OutputFile;
}

static int
mainloop ( int argc, char** argv )
{
    SMRTyp           SMR;                       // contains SMRs for the given frame
    PCMDataTyp       Main;                      // contains PCM data for 1600 samples
    SubbandFloatTyp  X [32];                    // Subbandsamples as float()
    wave_t           Wave;                      // contains WAV-files arguments
    mpc_uint64_t        AllSamplesRead   =    0;   // overall read Samples per channel
    unsigned int     CurrentRead      =    0;   // current read Samples per channel
    unsigned int     N;                         // counter for processed frames
    char*            InputName        = NULL;   // Name of WAVE file
    char*            OutputName       = NULL;   // Name of bitstream file
    int              Silence          =    0;
    int              OldSilence       =    0;
    time_t           T;
    int              TransientL [PART_SHORT];   // Flag of transient detection
    int              TransientR [PART_SHORT];   // Flag of transient detection
    int              Transient  [32];           // Flag of transient detection
	PsyModel         m;
	mpc_encoder_t    e;
	mpc_uint_t si_size;

    // initialize tables which must be initialized once and only once

	m.SCF_Index_L = (int*) e.SCF_Index_L;
	m.SCF_Index_R = (int*) e.SCF_Index_R;

	Init_Psychoakustik (&m);
	Init_FPU ();

    // initialize PCM-data
    memset ( &Main, 0, sizeof Main );

    // open WAV file
    if ( EvalParameters (&m,  argc, argv, &InputName, &OutputName, 1 ) < 0 )
        return 1;
    if ( Open_WAV_Header ( &Wave, InputName ) < 0 ) {
        stderr_printf ( "\033[33;41;1mERROR\033[0m: Unable to read or decode: '%s'\n", InputName );
        return 1;
    }
    TitleBar ( InputName );
    CopyTags ( InputName );

    // read WAV-Header
    if ( 0 != Read_WAV_Header (&Wave) ) {
        stderr_printf ( "\033[33;41;1mERROR\033[0m: Invalid file header, not a WAVE file '%s'\n", InputName );
        return 1;
    }

    m.SampleFreq = Wave.SampleFreq;
    SamplesInWAVE = Wave.PCMSamples;

    if ( Wave.SampleFreq != 44100.  &&  Wave.SampleFreq != 48000.  &&  Wave.SampleFreq != 37800.  &&  Wave.SampleFreq != 32000. ) {
        stderr_printf ( "\033[33;41;1mERROR\033[0m: Sampling frequency of %g kHz is not supported!\n\n", (double)(Wave.SampleFreq * 1.e-3) );
        return 1;
    }

    if ( Wave.BitsPerSample < 8  ||  Wave.BitsPerSample > 32 ) {
        stderr_printf ( "\033[33;41;1mERROR\033[0m: %i bits per sample are not supported!\n\n", Wave.BitsPerSample );
        return 1;
    }

    switch ( Wave.Channels ) {
    case  0:
        stderr_printf ( "\033[33;41;1mERROR\033[0m: 0 channels file, this is nonsense\n\n" );
        return 1;
    case  1: case  2:
        break;
    case  3: case  4: case  5: case  6: case  7: case  8:
        stderr_printf ( "WARNING: %i channel(s) file, only first 2 channels are encoded.\n\n", Wave.Channels );
        break;
    default:
        stderr_printf ( "\033[33;41;1mERROR\033[0m: %i channel(s) file, not supported\n\n", Wave.Channels );
        return 1;
    }

    SetQualityParams (&m, 5.0);

    if ( EvalParameters (&m, argc, argv, &InputName, &OutputName, 0 ) < 0 )
        return 1;

    if ( (long double)(SamplesInWAVE) >= Wave.SampleFreq * (SkipTime + Duration) ) {
        SamplesInWAVE = Wave.SampleFreq * (SkipTime + Duration);
    }

	mpc_encoder_init (&e, SamplesInWAVE, FramesBlockPwr, SeekDistance);
    Init_Psychoakustiktabellen (&m);              // must be done AFTER decoding command line parameters

    // check fade-length
    if ( FadeInTime + FadeOutTime > (long double)(SamplesInWAVE) / Wave.SampleFreq ) {
        stderr_printf ( "WARNING: Duration of fade in + out exceeds file length!\n");
        FadeInTime = FadeOutTime = 0.5 * (long double)(SamplesInWAVE) / Wave.SampleFreq;
    }

	e.outputFile = OpenStream(OutputName);

    ShowParameters (&m, InputName, OutputName );
    if ( WIN32_MESSAGES  &&  FrontendPresent )
        SendModeMessage (m.MainQual);

    if ( SkipTime > 0. ) {
        unsigned long  SkipSamples = m.SampleFreq * SkipTime;
        signed long read;

        while ( SkipSamples > 0 ) {
            read          = Read_WAV_Samples ( &Wave, mini(BLOCK, SkipSamples), &Main, CENTER, ScalingFactorl, ScalingFactorr, &Silence );
            if ( read <= 0 )
                break;
            SkipSamples   -= read;
            SamplesInWAVE -= read;
        }
    }

	e.MS_Channelmode = m.MS_Channelmode;
	e.seek_ref = ftell(e.outputFile);
	writeMagic(&e);
	writeStreamInfo ( &e, m.Max_Band, m.MS_Channelmode > 0, SamplesInWAVE, 0,
					   m.SampleFreq, Wave.Channels > 2 ? 2 : Wave.Channels);
	si_size = writeBlock(&e, "SH", MPC_TRUE, 0);
	writeGainInfo ( &e, 0, 0, 0, 0);
	writeBlock(&e, "RG", MPC_FALSE, 0);
	if (NoEncoderInfo == 0) {
		writeEncoderInfo(&e, m.FullQual, m.PNS > 0, MPCENC_MAJOR, MPCENC_MINOR, MPCENC_BUILD);
		writeBlock(&e, "EI", MPC_FALSE, 0);
	}
	if (NoSeekTable == 0) {
		e.seek_ptr = ftell(e.outputFile);
		writeBits (&e, 0, 16);
		writeBits (&e, 0, 24); // jump 40 bits for seek table pointer
		writeBlock(&e, "SO", MPC_FALSE, 0); // reserve space for seek offset
	}


    // initialize timer
    ShowProgress (&m, 0, SamplesInWAVE, e.outputBits );
    T            = time ( NULL );

    // read samples
    CurrentRead     = Read_WAV_Samples ( &Wave, (int)minf(BLOCK, SamplesInWAVE - AllSamplesRead), &Main, CENTER, ScalingFactorl, ScalingFactorr, &Silence );
    AllSamplesRead += CurrentRead;

	if (CurrentRead > 0)
	{
		fill_float( Main.L, Main.L[CENTER], CENTER );
		fill_float( Main.R, Main.R[CENTER], CENTER );
		fill_float( Main.M, Main.M[CENTER], CENTER );
		fill_float( Main.S, Main.S[CENTER], CENTER );
	}

	Analyse_Init ( Main.L[CENTER], Main.R[CENTER], X, m.Max_Band );

    // adapt SamplesInWAVE to the real number of contained samples
    if ( myfeof (Wave.fp) ) {
        stderr_printf ( "WAVE file has incorrect header: header: %.3f s, contents: %.3f s    \n",
						(long double)(SamplesInWAVE) / m.SampleFreq, (long double)(AllSamplesRead) / m.SampleFreq);
    }

    for ( N = 0; (mpc_uint64_t)N * BLOCK < SamplesInWAVE + DECODER_DELAY; N++ ) {

        // setting residual data-fields to zero
        if ( CurrentRead < BLOCK  &&  N > 0 ) {
            fill_float( Main.L + (CENTER + CurrentRead), Main.L[CENTER + CurrentRead - 1], BLOCK - CurrentRead );
            fill_float( Main.R + (CENTER + CurrentRead), Main.R[CENTER + CurrentRead - 1], BLOCK - CurrentRead );
            fill_float( Main.M + (CENTER + CurrentRead), Main.M[CENTER + CurrentRead - 1], BLOCK - CurrentRead );
            fill_float( Main.S + (CENTER + CurrentRead), Main.S[CENTER + CurrentRead - 1], BLOCK - CurrentRead );
        }

        /*********************************************************************************/
        /*                                Fade In and Fade Out                                */
        /*********************************************************************************/
        if ( FadeInTime  > 0. )
            if ( FadeInTime  > (long double)(BLOCK         + (mpc_uint64_t)N*BLOCK) / Wave.SampleFreq )
                Fading_In  ( &Main, N*BLOCK, Wave.SampleFreq );
        if ( FadeOutTime > 0. )
            if ( FadeOutTime > (long double)(SamplesInWAVE - (mpc_uint64_t)N*BLOCK) / Wave.SampleFreq )
                Fading_Out ( &Main, N*BLOCK, Wave.SampleFreq );

        /********************************************************************/
        /*                         Encoder-Core                             */
        /********************************************************************/
        // you only get null samples at the output of the filterbank when the last frame contains zeroes

		memset ( e.Res_L, 0, sizeof e.Res_L );
		memset ( e.Res_R, 0, sizeof e.Res_R );

        if ( !Silence  ||  !OldSilence ) {
            Analyse_Filter ( &Main, X, m.Max_Band );                      // Analysis-Filterbank (Main -> X)
			SMR = Psychoakustisches_Modell (&m, m.Max_Band*0+31, &Main, TransientL, TransientR );    // Psychoacoustics return SMRs for input data 'Main'
            if ( m.minSMR > 0 )
				RaiseSMR (&m, m.Max_Band, &SMR );                            // Minimum-operation on SBRs (full bandwidth)
			if ( m.MS_Channelmode > 0 )
				MS_LR_Entscheidung ( m.Max_Band, e.MS_Flag, &SMR, X );      // Selection of M/S- or L/R-Coding
			SCF_Extraktion (&m, &e, m.Max_Band, X );                             // Extraction of the scalefactors and normalization of the subband samples
            TransientenCalc ( Transient, TransientL, TransientR );
			if ( m.NS_Order > 0 ) {
				NS_Analyse (&m, m.Max_Band, e.MS_Flag, SMR, Transient );                  // calculate possible ANS-Filter and the expected gain
            }

			Allocate ( m.Max_Band, e.Res_L, X[0].L, e.SCF_Index_L[0], m.SNR_comp_L, SMR.L, Power_L, Transient , m.PNS );   // allocate bits for left + right channel
			Allocate ( m.Max_Band, e.Res_R, X[0].R, e.SCF_Index_R[0], m.SNR_comp_R, SMR.R, Power_R, Transient , m.PNS );

			Quantisierung (&m, m.Max_Band, e.Res_L, e.Res_R, X, e.Q );             // quantize samples
        }

        OldSilence      = Silence;
        writeBitstream_SV8 ( &e, m.Max_Band); // write SV8-Bitstream

        if ( (int)(time (NULL) - T) >= 0 ) {                            // output
            T += labs (DisplayUpdateTime);
			ShowProgress (&m, (mpc_uint64_t)(N+1) * BLOCK, SamplesInWAVE, e.outputBits );
        }

        memmove ( Main.L, Main.L + BLOCK, CENTER * sizeof(float) );
        memmove ( Main.R, Main.R + BLOCK, CENTER * sizeof(float) );
        memmove ( Main.M, Main.M + BLOCK, CENTER * sizeof(float) );
        memmove ( Main.S, Main.S + BLOCK, CENTER * sizeof(float) );

        // read samples
        CurrentRead     = Read_WAV_Samples ( &Wave, (int)minf(BLOCK, SamplesInWAVE - AllSamplesRead), &Main, CENTER, ScalingFactorl, ScalingFactorr, &Silence );
        AllSamplesRead += CurrentRead;

        // adapt SamplesInWAV to the real number of contained samples
		if ( myfeof (Wave.fp) ) {
			stderr_printf ( "WAVE file has incorrect header: header: %.3f s, contents: %.3f s    \n",
							(long double)(SamplesInWAVE) / m.SampleFreq, (long double)(AllSamplesRead) / m.SampleFreq);
			SamplesInWAVE = AllSamplesRead;
		}
    }

    // write the last incomplete block
	if (e.framesInBlock != 0) {
		if ((e.block_cnt & ((1 << e.seek_pwr) - 1)) == 0) {
			e.seek_table[e.seek_pos] = ftell(e.outputFile);
			e.seek_pos++;
		}
		e.block_cnt++;
		writeBlock(&e, "AP", MPC_FALSE, 0);
	}
	if (NoSeekTable == 0) {
		writeSeekTable(&e);
		writeBlock(&e, "ST", MPC_FALSE, 0); // write seek table block
	}
	writeBlock(&e, "SE", MPC_FALSE, 0); // write end of stream block

	if (Wave.PCMSamples != AllSamplesRead) {
		fseek(e.outputFile, e.seek_ref + 4, SEEK_SET);
		writeStreamInfo ( &e, m.Max_Band, m.MS_Channelmode > 0, SamplesInWAVE, 0,
						   m.SampleFreq, Wave.Channels > 2 ? 2 : Wave.Channels);
		writeBlock(&e, "SH", MPC_TRUE, si_size);
		fseek(e.outputFile, 0, SEEK_END);
	}

    ShowProgress (&m, SamplesInWAVE, SamplesInWAVE, e.outputBits );

    FinalizeTags ( e.outputFile, APE_Version, 0 );
    fclose ( e.outputFile );
    fclose ( Wave.fp );
	mpc_encoder_exit(&e);

    if ( DelInput == 0xAFFEDEAD  &&  remove (InputName) == -1 )         // delete input file if DelInput is active
        stderr_printf ( "\n\n\033[33;41;1mERROR\033[0m: Could not delete input file '%s'\n", InputName );

    if ( WIN32_MESSAGES  &&  FrontendPresent )
        SendQuitMessage ();

    stderr_printf ( "\n" );

	OverdriveReport (&e);                         // output a report if clipping was necessary

    return 0;
}


/************ The main() function *****************************/
int mpc_cdecl
main ( int argc, char** argv )
{
    int  ret;

#ifdef _OS2
    _wildcard ( &argc, &argv );
#endif

#ifdef LC_CTYPE
	setlocale(LC_CTYPE, "");
	NoUnicode = (strcmp(nl_langinfo(CODESET), "UTF-8") != 0);
#endif

    if ( WIN32_MESSAGES ) {
        FrontendPresent = SearchForFrontend (); // search for presence of Windows Frontend
        if ( FrontendPresent )
            SendStartupMessage ( MPCENC_VERSION, 8);
    }

    // Welcome message
    if ( argc < 2  ||  ( 0 != strcmp (argv[1], "--silent")  &&  0 != strcmp (argv[1], "--quiet")) )
        (void) stderr_printf ("\r\x1B[1m\r%s\n\x1B[0m\r     \r", About );

#ifdef FAST_MATH
    Init_FastMath ();
#endif

    // no arguments or call for help
    if ( argc < 2  ||  0==strcmp (argv[1],"-h")  ||  0==strcmp (argv[1],"-?")  ||  0==strcmp (argv[1],"--help") ) {
		PsyModel m;
        SetQualityParams (&m, 5.0);
        dup2 ( 1, 2 );
        shorthelp ();
        return 1;
    }

    if ( 0==strcmp (argv[1],"--longhelp")  ||  0==strcmp (argv[1],"-??") ) {
		PsyModel m;
        SetQualityParams (&m, 5.0);
        dup2 ( 1, 2 );
        longhelp (&m);
        return 1;
    }

    ret = mainloop ( argc, argv );              // analyze command line and do the requested work

    if(IsEndBeep)
        stderr_printf("\a\a\a");

#ifdef BUGBUG
    reppr ();
#endif
    return ret;
}

/* end of mpcenc.c */
