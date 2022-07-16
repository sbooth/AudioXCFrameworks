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

#include "mpcenc.h"


#define MAX_LPC_ORDER       35
#define log2(x)             ( log (x) * (1./M_LN2) )
#define ORDER_PENALTY       0


static int                                     // best prediction order model
CalculateLPCCoeffs ( Int32_t*  buf,            // Samples
                     size_t    nbuf,           // Number of samples
                     Int32_t   offset,         //
                     double*   lpcout,         // quantized prediction coefficients
                     int       nlpc,           // max. prediction order
                     float*    psigbit,        // expected number of bits per original signal sample
                     float*    presbit )       // expected number of bits per residual signal sample
{
    static double*  fbuf  = NULL;
    static int      nflpc = 0;
    static int      nfbuf = 0;
    int             nbit;
    int             i;
    int             j;
    int             bestnbit;
    int             bestnlpc;
    double          e;
    double          bestesize;
    double          ci;
    double          esize;
    double          acf [MAX_LPC_ORDER + 1];
    double          ref [MAX_LPC_ORDER + 1];
    double          lpc [MAX_LPC_ORDER + 1];
    double          tmp [MAX_LPC_ORDER + 1];
    double          escale = 0.5 * M_LN2 * M_LN2 / nbuf;
    double          sum;

    if ( nlpc >= nbuf )                         // if necessary, limit the LPC order to the number of samples available
        nlpc = nbuf - 1;

    if ( nlpc > nflpc  ||  nbuf > nfbuf ) {     // grab some space for a 'zero mean' buffer of floats if needed
        if ( fbuf != NULL )
            free ( fbuf - nflpc );
        fbuf  = nlpc + ((double*) calloc ( nlpc+nbuf, sizeof (*fbuf) ));
        nfbuf = nbuf;
        nflpc = nlpc;
    }

    e = 0.;
    for ( j = 0; j < nbuf; j++ ) {              // zero mean signal and compute energy
        sum = fbuf [j] = buf[j] - (double)offset;
        e  += sum * sum;
    }

    esize     = e > 0.  ?  0.5 * log2 (escale * e)  :  0.;
    *psigbit  = esize;                          // return the expected number of bits per original signal sample

    acf [0]   = e;                              // store the best values so far (the zeroth order predictor)
    bestnlpc  = 0;
    bestnbit  = nbuf * esize;
    bestesize = esize;

    for ( i = 1; i <= nlpc  &&  e > 0.  &&  i < bestnlpc + 4; i++ ) {   // just check two more than bestnlpc

        sum = 0.;
        for ( j = i; j < nbuf; j++ )                                    // compute the jth autocorrelation coefficient
            sum += fbuf [j] * fbuf [j-i];
        acf [i] = sum;

        ci = 0.;                                                        // compute the reflection and LP coeffients for order j predictor
        for ( j = 1; j < i; j++ )
            ci += lpc [j] * acf [i-j];
        lpc [i] = ref [i] = ci = (acf [i] - ci) / e;
        for ( j = 1; j < i; j++ )
            tmp [j] = lpc [j] - ci * lpc [i-j];
        for ( j = 1; j < i; j++ )
            lpc [j] = tmp [j];

        e    *= 1 - ci*ci;                                              // compute the new energy in the prediction residual
        esize = e > 0.  ?  0.5 * log2 (escale * e)  :  0.;

        nbit = nbuf * esize + i * ORDER_PENALTY;
        if ( nbit < bestnbit ) {                                        // store this model if it is the best so far
            bestnlpc  = i;                                              // store best model order
            bestnbit  = nbit;
            bestesize = esize;

            for ( j = 0; j < bestnlpc; j++ )                            // store the quantized LP coefficients
                lpcout [j] = lpc [j+1];
        }
    }

    *presbit = bestesize;                       // return the expected number of bits per residual signal sample
    return bestnlpc;                            // return the best model order
}


static void
Pred ( const unsigned int*  new,
       unsigned int*        old )
{
    static Double  DOUBLE [36];
    Float   org;
    Float   pred;
    int     i;
    int     j;
    int     sum = 18;
    int     order;
    double  oldeff = 0.;
    double  neweff = 0.;

    for ( i = 0; i < 36; i++ )
        sum += old [i];
    sum = (int) floor (sum / 36.);

    order = CalculateLPCCoeffs ( old, 36, sum*0, DOUBLE, 35, &org, &pred );

    printf ("avg: %4u  [%2u]  %.2f  %.2f\n\n", sum, order, org, pred );
    if ( order < 1 )
        return;

    for ( i = 0; i < order; i++ )
        printf ("%f ", DOUBLE[i] );
    printf ("\n");

    for ( i = 0; i < 36; i++ ) {
        double  sum = 0.;
        for ( j = 1; j <= order; j++ ) {
            sum += (i-j < 0 ? old[i-j+36] : new [i-j]) * DOUBLE [j-1];
        }
        printf ("%2u: %6.2f %3d\n", i, sum, new [i] );
        oldeff += new[i]       * new[i];
        neweff += (sum-new[i]) * (sum-new[i]);
    }
    printf ("%6.2f %6.2f\n", sqrt(oldeff), sqrt(neweff) );
}


void
Predicate ( int Channel, int Band, unsigned int* x, int* scf )
{
    static Int32_t  OLD [2] [32] [36];
    int    i;

    printf ("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
    for ( i = 0; i < 36; i++ )
        printf ("%2d ", OLD [Channel][Band][i] );
    printf ("\n");
    for ( i = 0; i < 36; i++ )
        printf ("%2d ", x[i] );
    printf ("\n");
    printf ("%2u-%2u-%2u  ", scf[0], scf[1], scf[2] );
    Pred ( x, OLD [Channel][Band] );
    for ( i = 0; i < 36; i++ )
        OLD [Channel][Band][i] = x[i];
}

/* end of predict.c */
