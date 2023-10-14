#pragma once

#include "All.h"

#ifdef _MSC_VER
    #pragma warning(disable: 4514) // unreferenced inline function
#endif

/**************************************************************************************************
Macros for SIMD expansion
**************************************************************************************************/

#define EXPAND_SIMD_2(OFFSET, INCREMENT, CODE) {{ OFFSET = 0; CODE } { OFFSET = 1 * INCREMENT; CODE }}
#define EXPAND_SIMD_4(OFFSET, INCREMENT, CODE) {{ OFFSET = 0; CODE } { OFFSET = 1 * INCREMENT; CODE } { OFFSET = 2 * INCREMENT; CODE } { OFFSET = 3 * INCREMENT; CODE }}

/**************************************************************************************************
Macros to avoid code duplication
**************************************************************************************************/

#define UPDATE_DELTA_NEW(VALUE)                                 \
{                                                               \
    INTTYPE nTempABS = Abs(VALUE);                              \
                                                                \
    if (nTempABS > (m_nRunningAverage * 3))                     \
        m_rbDeltaM[0] = ((VALUE >> 25) & 64) - 32;              \
    else if (nTempABS > (m_nRunningAverage * 4) / 3)            \
        m_rbDeltaM[0] = ((VALUE >> 26) & 32) - 16;              \
    else if (nTempABS > 0)                                      \
        m_rbDeltaM[0] = ((VALUE >> 27) & 16) - 8;               \
    else                                                        \
        m_rbDeltaM[0] = 0;                                      \
                                                                \
    m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;   \
                                                                \
    m_rbDeltaM[-1] >>= 1;                                       \
    m_rbDeltaM[-2] >>= 1;                                       \
    m_rbDeltaM[-8] >>= 1;                                       \
}

#define UPDATE_DELTA_OLD(VALUE)                                 \
{                                                               \
    m_rbDeltaM[0] = (VALUE == 0) ? 0 : ((VALUE >> 28) & 8) - 4; \
    m_rbDeltaM[-4] >>= 1;                                       \
    m_rbDeltaM[-8] >>= 1;                                       \
}

namespace APE
{

/**************************************************************************************************
Helper functions
**************************************************************************************************/

__forceinline short GetSaturatedShortFromInt(int32 nValue)
{
    short sValue = static_cast<short>(nValue);
    if (sValue != nValue)
        sValue = (nValue >> (8 * sizeof(int32) - 1)) ^ 0x7FFF;
    return sValue;
}

__forceinline short GetSaturatedShortFromInt(int64 nValue)
{
    short sValue = static_cast<short>(nValue);
    if (sValue != nValue)
        sValue = (nValue >> (8 * sizeof(int64) - 1)) ^ 0x7FFF;
    return sValue;
}

/*************************************************************************************************/

__forceinline int Abs(int32 nValue)
{ return abs(nValue); }

__forceinline int64 Abs(int64 nValue)
{ return llabs(nValue); }

/*************************************************************************************************/

__forceinline void Adapt(short * pM, const short * pAdapt, int32 nDirection, int nOrder)
{
    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    nOrder >>= 4;

    if (nDirection < 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ += *pAdapt++;)
        }
    }
    else if (nDirection > 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
        }
    }
}

__forceinline void Adapt(int * pM, const int * pAdapt, int64 nDirection, int nOrder)
{
    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    nOrder >>= 4;

    if (nDirection < 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ += *pAdapt++;)
        }
    }
    else if (nDirection > 0)
    {
        while (nOrder--)
        {
            EXPAND_16_TIMES(*pM++ -= *pAdapt++;)
        }
    }
}

/*************************************************************************************************/

__forceinline int32 CalculateDotProduct(const short * pA, const short * pB, int nOrder)
{
    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    int32 nDotProduct = 0;
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
    }

    return nDotProduct;
}

__forceinline int64 CalculateDotProduct(const int * pA, const int * pB, int nOrder)
{
    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    int64 nDotProduct = 0;
    int32 nTemporary = 0;
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nTemporary = *pA++ * *pB++; nDotProduct += nTemporary;)
    }

    return nDotProduct;
}

}
