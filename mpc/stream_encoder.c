/*
 * Musepack audio compression
 * Copyright (c) 2005-2009, The Musepack Development Team
 * Copyright (C) 1999-2004 Buschmann/Klemm/Piecha/Wolf
 * Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
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

#import "stream_encoder.h"

#import <mpc/libmpcpsy.h>
#import <mpc/mpcmath.h>
#import <mpc/datatypes.h>
#import <mpc/minimax.h>

#import <stdio.h>

#pragma mark - config.h

/* parsed values from file "version" */

#define MPCENC_MAJOR 1
#define MPCENC_MINOR 30
#define MPCENC_BUILD 1

/* end of config.h */

#pragma mark - mpcenc.h

//// constants /////////////////////////////////////////////////////
#define DECODER_DELAY    (512 - 32 + 1)
#define BLK_SIZE         (36 * 32)


//// procedures/functions //////////////////////////////////////////
#define SCFfac              0.832980664785f     // = SCF[n-1]/SCF[n]

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
void   QuantizeSubband                 ( mpc_int16_t* qu_output, const float* input, const int res, float* errors, const int maxNsOrder );
void   QuantizeSubbandWithNoiseShaping ( mpc_int16_t* qu_output, const float* input, const int res, float* errors, const float* FIR );

void   NoiseInjectionComp ( void );

// regress.c
void    Regression       ( float* const _r, float* const _b, const float* p, const float* q );


// tags.c
//void    Init_Tags        ( void );
//int     FinalizeTags     ( FILE* fp, unsigned int Version, unsigned int flags );
//int     addtag           ( const char* key, size_t keylen, const char* value, size_t valuelen, int converttoutf8, int flags );
//int     gettag           ( const char* key, char* dst, size_t len );
//int     CopyTags         ( const char* filename );


#define MPPENC_DENORMAL_FIX_BASE ( 32. * 1024. /* normalized sample value range */ / ( (float) (1 << 24 /* first bit below 32-bit PCM range */ ) ) )
#define MPPENC_DENORMAL_FIX_LEFT ( MPPENC_DENORMAL_FIX_BASE )
#define MPPENC_DENORMAL_FIX_RIGHT ( MPPENC_DENORMAL_FIX_BASE * 0.5f )

#ifndef LAST_HUFFMAN
# define LAST_HUFFMAN    7
#endif

/* end of mpcenc.h */

#pragma mark - mpcenc.c

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
SCF_Extraktion ( PsyModel*m, mpc_encoder_t* e, const int MaxBand, SubbandFloatTyp* x, float Power_L[32][3], float Power_R[32][3], float *MaxOverFlow )
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
			if (scfL[n] < -6) { scfL[n] = -6; warnL = 1; }
			if (scfL[n] > 121) { scfL[n] = 121; warnL = 1; }
			if (scfR[n] < -6) { scfR[n] = -6; warnR = 1; }
			if (scfR[n] > 121) { scfR[n] = 121; warnR = 1; }
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
					*MaxOverFlow = maxf (*MaxOverFlow,  x[Band].L[n]);
					x[Band].L[n] = 32767.f;
				}
				else if (x[Band].L[n] < -32767.f) {
					e->Overflows++;
					*MaxOverFlow = maxf (*MaxOverFlow, -x[Band].L[n]);
					x[Band].L[n] = -32767.f;
				}
			}
		if ( warnR )
			for ( n = 0; n < 36; n++ ) {
				if      (x[Band].R[n] > +32767.f) {
					e->Overflows++;
					*MaxOverFlow = maxf (*MaxOverFlow,  x[Band].R[n]);
					x[Band].R[n] = 32767.f;
				}
				else if (x[Band].R[n] < -32767.f) {
					e->Overflows++;
					*MaxOverFlow = maxf (*MaxOverFlow, -x[Band].R[n]);
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
	S0 = (float)sqrt (S0/12 * 4/1.2005080577484075047860806747022);
	S1 = (float)sqrt (S1/12 * 4/1.2005080577484075047860806747022);
	S2 = (float)sqrt (S2/12 * 4/1.2005080577484075047860806747022);
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
			for ( MNR = *smr * 1.f; MNR > 1.f  &&  *res != 15; )
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

static void fill_float(float *buffer, float val, size_t count)
{
	while(count--)
		*buffer++ = val;
}

#pragma mark - mpc_stream_encoder

struct mpc_stream_encoder_t {
	float				Power_L [32][3];
	float				Power_R [32][3];
	float				MaxOverFlow;				// maximum overflow
	unsigned int 		FramesBlockPwr;				// must be even : frames_per_block = 1 << FramesBlockPwr
	unsigned int		SeekDistance;				// keep a seek table entry every 2^SeekDistance block
	mpc_uint64_t		SamplesInWAVE;				// number of samples per channel in the WAV file
	SMRTyp				SMR;						// contains SMRs for the given frame
	PCMDataTyp			Main;						// contains PCM data for 1600 samples
	SubbandFloatTyp		X [32];						// Subbandsamples as float()
	int					OldSilence;
	int					TransientL [PART_SHORT];	// Flag of transient detection
	int					TransientR [PART_SHORT];	// Flag of transient detection
	int					Transient [32];				// Flag of transient detection
	PsyModel			m;
	mpc_encoder_t		e;
	mpc_uint_t			si_size;

	float				samplerate;					// Hz
	unsigned int		channels;					// Number of channels (1 or 2)
	mpc_int16_t			*buf;						// Interleaved
	unsigned int		framelen;					// Frames in buf

	unsigned int		blocks;						// Number of blocks encoded
	mpc_uint64_t		frames;						// Count of frames encoded
};

mpc_stream_encoder * mpc_stream_encoder_create()
{
	mpc_stream_encoder *enc = calloc(1, sizeof(mpc_stream_encoder));
	if(!enc)
		return NULL;

	enc->m.SCF_Index_L = (int *)enc->e.SCF_Index_L;
	enc->m.SCF_Index_R = (int *)enc->e.SCF_Index_R;

	Init_Psychoakustik (&enc->m);

	// initialize PCM-data
	memset ( &enc->Main, 0, sizeof(enc->Main) );

	enc->FramesBlockPwr = 6;
	enc->SeekDistance = 1;

	return enc;
}

void mpc_stream_encoder_destroy(mpc_stream_encoder *enc)
{
	if(enc) {
		mpc_encoder_exit(&enc->e);
		free(enc->buf);
		enc->buf = NULL;
		free(enc);
	}
}

mpc_status mpc_stream_encoder_set_quality(mpc_stream_encoder *enc, float quality)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;
	SetQualityParams (&enc->m, quality);
	return MPC_STATUS_OK;
}

mpc_status mpc_stream_encoder_set_estimated_total_frames(mpc_stream_encoder *enc, mpc_uint64_t frames)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;
	if(frames >= 0)
		enc->SamplesInWAVE = frames;
	return MPC_STATUS_OK;
}

mpc_status mpc_stream_encoder_set_frames_block_power(mpc_stream_encoder *enc, unsigned int frames_block_power)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;
	if(frames_block_power % 2 == 0)
		enc->FramesBlockPwr = frames_block_power;
	return MPC_STATUS_OK;
}

mpc_status mpc_stream_encoder_set_seek_distance(mpc_stream_encoder *enc, unsigned int seek_distance)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;
	if(seek_distance > 0)
		enc->SeekDistance = seek_distance;
	return MPC_STATUS_OK;
}

mpc_status mpc_stream_encoder_init(mpc_stream_encoder *enc, float samplerate, int channels, mpc_write_callback_t write_callback, mpc_seek_callback_t seek_callback, mpc_tell_callback_t tell_callback, void *vio_context)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;

	if(channels < 1 || channels > 2)
		return MPC_STATUS_FAIL;

	if(samplerate != 44100 && samplerate != 48000 && samplerate != 37800 && samplerate != 32000)
		return MPC_STATUS_FAIL;

	if(write_callback == NULL || seek_callback == NULL || tell_callback == NULL)
		return MPC_STATUS_FAIL;

	enc->samplerate = samplerate;
	enc->channels = (unsigned int)channels;

	enc->buf = (mpc_int16_t *)calloc(BLOCK * enc->channels, sizeof(mpc_int16_t));
	if(enc->buf == NULL)
		return MPC_STATUS_FAIL;

	enc->m.SampleFreq = enc->samplerate;

	SetQualityParams (&enc->m, 5.0);

	mpc_encoder_init (&enc->e, enc->SamplesInWAVE, enc->FramesBlockPwr, enc->SeekDistance);
	Init_Psychoakustiktabellen (&enc->m);              // must be done AFTER decoding command line parameters

	enc->e.vio_context = vio_context;
	enc->e.vio_write = write_callback;
	enc->e.vio_seek = seek_callback;
	enc->e.vio_tell = tell_callback;

	enc->e.MS_Channelmode = enc->m.MS_Channelmode;
	enc->e.seek_ref = (mpc_uint32_t)enc->e.vio_tell(enc->e.vio_context);
	writeMagic(&enc->e);
	writeStreamInfo ( &enc->e, (unsigned int)enc->m.Max_Band, enc->m.MS_Channelmode > 0, (unsigned int)enc->SamplesInWAVE, 0, (unsigned int)enc->samplerate, enc->channels);
	enc->si_size = writeBlock(&enc->e, "SH", MPC_TRUE, 0);
	writeGainInfo ( &enc->e, 0, 0, 0, 0);
	writeBlock(&enc->e, "RG", MPC_FALSE, 0);

	writeEncoderInfo(&enc->e, enc->m.FullQual, enc->m.PNS > 0, MPCENC_MAJOR, MPCENC_MINOR, MPCENC_BUILD);
	writeBlock(&enc->e, "EI", MPC_FALSE, 0);

	enc->e.seek_ptr = (mpc_uint32_t)enc->e.vio_tell(enc->e.vio_context);
	writeBits (&enc->e, 0, 16);
	writeBits (&enc->e, 0, 24); // jump 40 bits for seek table pointer
	writeBlock(&enc->e, "SO", MPC_FALSE, 0); // reserve space for seek offset

	return MPC_STATUS_OK;
}

static int is_digital_silence(const mpc_int16_t *buf, unsigned int frames, unsigned int channels)
{
	for(unsigned int i = 0; i < frames; ++i) {
		for(unsigned int j = 0; j < channels; ++j) {
			if(*buf != 0)
				return 0;
			++buf;
		}
	}
	return 1;
}

static mpc_status encode_block(mpc_stream_encoder *enc)
{
	// Check if signal is silence
	int isSilence = is_digital_silence(enc->buf, enc->framelen, enc->channels);

	// Calculate mid/side
	const int16_t *buf = enc->buf;
	if(enc->channels == 1) {
		for(uint i = CENTER; i < CENTER + enc->framelen; ++i) {
			enc->Main.L[i] = (float)(*buf   + MPPENC_DENORMAL_FIX_LEFT);
			enc->Main.R[i] = (float)(*buf++ + MPPENC_DENORMAL_FIX_RIGHT);
			enc->Main.M[i] = (enc->Main.L[i] + enc->Main.R[i]) * 0.5f;
			enc->Main.S[i] = (enc->Main.L[i] - enc->Main.R[i]) * 0.5f;
		}
	}
	else {
		for(uint i = CENTER; i < CENTER + enc->framelen; ++i) {
			enc->Main.L[i] = (float)(*buf++ + MPPENC_DENORMAL_FIX_LEFT);
			enc->Main.R[i] = (float)(*buf++ + MPPENC_DENORMAL_FIX_RIGHT);
			enc->Main.M[i] = (enc->Main.L[i] + enc->Main.R[i]) * 0.5f;
			enc->Main.S[i] = (enc->Main.L[i] - enc->Main.R[i]) * 0.5f;
		}
	}

	unsigned int framelen = enc->framelen;
	unsigned int framesOfSilenceAdded = 0;
	if(enc->framelen < BLOCK) {
		framesOfSilenceAdded = BLOCK - framelen;
		memset(enc->buf + (framelen * enc->channels), 0, framesOfSilenceAdded * enc->channels * sizeof(mpc_int16_t));
		enc->framelen += framesOfSilenceAdded;
	}

	// Just starting
	if(enc->blocks == 0) {
		fill_float(enc->Main.L, enc->Main.L[CENTER], CENTER);
		fill_float(enc->Main.R, enc->Main.R[CENTER], CENTER);
		fill_float(enc->Main.M, enc->Main.M[CENTER], CENTER);
		fill_float(enc->Main.S, enc->Main.S[CENTER], CENTER);

		Analyse_Init(enc->Main.L[CENTER], enc->Main.R[CENTER], enc->X, enc->m.Max_Band);
	}

	// setting residual data-fields to zero
	if(framesOfSilenceAdded > 0 && enc->blocks > 0) {
		fill_float(enc->Main.L + (CENTER + framelen), enc->Main.L[CENTER + framelen - 1], framesOfSilenceAdded);
		fill_float(enc->Main.R + (CENTER + framelen), enc->Main.R[CENTER + framelen - 1], framesOfSilenceAdded);
		fill_float(enc->Main.M + (CENTER + framelen), enc->Main.M[CENTER + framelen - 1], framesOfSilenceAdded);
		fill_float(enc->Main.S + (CENTER + framelen), enc->Main.S[CENTER + framelen - 1], framesOfSilenceAdded);
	}

	/********************************************************************/
	/*                         Encoder-Core                             */
	/********************************************************************/
	// you only get null samples at the output of the filterbank when the last frame contains zeroes

	memset(enc->e.Res_L, 0, sizeof(enc->e.Res_L));
	memset(enc->e.Res_R, 0, sizeof(enc->e.Res_R));

	if ( !isSilence  ||  !enc->OldSilence ) {
		Analyse_Filter ( &enc->Main, enc->X, enc->m.Max_Band );                      // Analysis-Filterbank (Main -> X)
		enc->SMR = Psychoakustisches_Modell (&enc->m, enc->m.Max_Band*0+31, &enc->Main, enc->TransientL, enc->TransientR );    // Psychoacoustics return SMRs for input data 'Main'
		if ( enc->m.minSMR > 0 )
			RaiseSMR (&enc->m, enc->m.Max_Band, &enc->SMR );                            // Minimum-operation on SBRs (full bandwidth)
		if ( enc->m.MS_Channelmode > 0 )
			MS_LR_Entscheidung ( enc->m.Max_Band, enc->e.MS_Flag, &enc->SMR, enc->X );      // Selection of M/S- or L/R-Coding
		SCF_Extraktion (&enc->m, &enc->e, enc->m.Max_Band, enc->X, enc->Power_L, enc->Power_R, &enc->MaxOverFlow );                             // Extraction of the scalefactors and normalization of the subband samples
		TransientenCalc ( enc->Transient, enc->TransientL, enc->TransientR );
		if ( enc->m.NS_Order > 0 ) {
			NS_Analyse (&enc->m, enc->m.Max_Band, enc->e.MS_Flag, enc->SMR, enc->Transient );                  // calculate possible ANS-Filter and the expected gain
		}

		Allocate ( enc->m.Max_Band, enc->e.Res_L, enc->X[0].L, enc->e.SCF_Index_L[0], enc->m.SNR_comp_L, enc->SMR.L, enc->Power_L, enc->Transient , enc->m.PNS );   // allocate bits for left + right channel
		Allocate ( enc->m.Max_Band, enc->e.Res_R, enc->X[0].R, enc->e.SCF_Index_R[0], enc->m.SNR_comp_R, enc->SMR.R, enc->Power_R, enc->Transient , enc->m.PNS );

		Quantisierung (&enc->m, enc->m.Max_Band, enc->e.Res_L, enc->e.Res_R, enc->X, enc->e.Q );             // quantize samples
	}

	enc->OldSilence = isSilence;
	writeBitstream_SV8(&enc->e, enc->m.Max_Band); // write SV8-Bitstream

	memmove(enc->Main.L, enc->Main.L + BLOCK, CENTER * sizeof(float));
	memmove(enc->Main.R, enc->Main.R + BLOCK, CENTER * sizeof(float));
	memmove(enc->Main.M, enc->Main.M + BLOCK, CENTER * sizeof(float));
	memmove(enc->Main.S, enc->Main.S + BLOCK, CENTER * sizeof(float));

	enc->blocks += 1;
	enc->frames += framelen;

	return MPC_STATUS_OK;
}

mpc_status mpc_stream_encoder_encode(mpc_stream_encoder *enc, const mpc_int16_t *data, unsigned int frames)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;

	if(data == NULL)
		return MPC_STATUS_FAIL;

	// Split data into block-sized chunks
	unsigned int framesProcessed = 0;
	unsigned int framesRemaining = frames;

	for(;;) {
		unsigned int avail = BLOCK - enc->framelen;
		unsigned int framesToCopy = framesRemaining > avail ? avail : framesRemaining;
		memcpy(enc->buf + (enc->framelen * enc->channels), data + (framesProcessed * enc->channels), framesToCopy * enc->channels * sizeof(mpc_int16_t));
		enc->framelen += framesToCopy;
		framesRemaining -= framesToCopy;
		framesProcessed += framesToCopy;

		// Encode the next MPC block
		if(enc->framelen == BLOCK) {
			if(encode_block(enc) != MPC_STATUS_OK)
				return MPC_STATUS_FAIL;
			enc->framelen = 0;
		}

		if(framesProcessed == frames)
			break;
	}

	return MPC_STATUS_OK;
}

mpc_status mpc_stream_encoder_finish(mpc_stream_encoder *enc)
{
	if(enc == NULL)
		return MPC_STATUS_FAIL;

	// Encode any remaining audio, ensuring at least DECODER_DELAY samples are added
	unsigned int framesRemaining = DECODER_DELAY;

	for(;;) {
		unsigned int avail = BLOCK - enc->framelen;
		unsigned int framesToZero = framesRemaining > avail ? avail : framesRemaining;
		memset(enc->buf + (enc->framelen * enc->channels), 0, framesToZero * enc->channels * sizeof(mpc_int16_t));
		enc->framelen += framesToZero;
		framesRemaining -= framesToZero;

		// Encode the next block
		if(enc->framelen == BLOCK) {
			if(encode_block(enc) != MPC_STATUS_OK)
				return MPC_STATUS_FAIL;
			enc->framelen = 0;
		}

		// All complete frames were processed
		if(framesRemaining == 0)
			break;
	}

	if(enc->framelen > 0) {
		if(encode_block(enc) != MPC_STATUS_OK)
			return MPC_STATUS_FAIL;
		enc->framelen = 0;
	}

	// adjust for delay
	enc->frames -= DECODER_DELAY;

	// write the last incomplete block
	if (enc->e.framesInBlock != 0) {
		if ((enc->e.block_cnt & ((1 << enc->e.seek_pwr) - 1)) == 0) {
			enc->e.seek_table[enc->e.seek_pos] = (mpc_uint32_t)enc->e.vio_tell(enc->e.vio_context);
			enc->e.seek_pos++;
		}
		enc->e.block_cnt++;
		writeBlock(&enc->e, "AP", MPC_FALSE, 0);
	}

	writeSeekTable(&enc->e);
	writeBlock(&enc->e, "ST", MPC_FALSE, 0); // write seek table block

	writeBlock(&enc->e, "SE", MPC_FALSE, 0); // write end of stream block

	if(enc->SamplesInWAVE != enc->frames) {
		enc->e.vio_seek(enc->e.vio_context, enc->e.seek_ref + 4, SEEK_SET);
		writeStreamInfo ( &enc->e, (unsigned int)enc->m.Max_Band, (unsigned int)enc->m.MS_Channelmode > 0, (unsigned int)enc->frames, 0, (unsigned int)enc->samplerate, enc->channels);
		writeBlock(&enc->e, "SH", MPC_TRUE, enc->si_size);
		enc->e.vio_seek(enc->e.vio_context, 0, SEEK_END);
	}

//	FinalizeTags ( e.outputFile, APE_Version, 0 );

	return MPC_STATUS_OK;
}
