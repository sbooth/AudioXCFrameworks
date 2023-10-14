#include "NNFilter.h"
#include "NNFilterCommon.h"
#include "CPUFeatures.h"

#if (defined(__AVX512DQ__) && defined(__AVX512BW__)) || (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)) && !defined(_M_ARM64EC))
    #define APE_USE_AVX512_INTRINSICS
#endif

#ifdef APE_USE_AVX512_INTRINSICS
    #include <immintrin.h> // AVX-512
#endif

namespace APE
{

bool GetAVX512Available()
{
#ifdef APE_USE_AVX512_INTRINSICS
    return true;
#else
    return false;
#endif
}

#ifdef APE_USE_AVX512_INTRINSICS

#define ADAPT_AVX512_SIMD_SHORT_ADD                              \
{                                                                \
    const __m512i avxM = _mm512_load_si512(&pM[z + n]);          \
    const __m512i avxAdapt = _mm512_loadu_si512(&pAdapt[z + n]); \
    const __m512i avxNew = _mm512_add_epi16(avxM, avxAdapt);     \
    _mm512_store_si512(&pM[z + n], avxNew);                      \
}

#define ADAPT_AVX512_SIMD_SHORT_SUB                              \
{                                                                \
    const __m512i avxM = _mm512_load_si512(&pM[z + n]);          \
    const __m512i avxAdapt = _mm512_loadu_si512(&pAdapt[z + n]); \
    const __m512i avxNew = _mm512_sub_epi16(avxM, avxAdapt);     \
    _mm512_store_si512(&pM[z + n], avxNew);                      \
}

static void AdaptAVX512(short * pM, const short * pAdapt, int32 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((reinterpret_cast<size_t>(pM) % 64) == 0);

    // we're working up to 64 elements at a time
    ASSERT(nOrder == 32 || (nOrder % 64) == 0);

    int z = 0, n = 0;
    if (nDirection < 0)
    {
        switch (nOrder)
        {
        case 32:
            ADAPT_AVX512_SIMD_SHORT_ADD
            break;
        default:
            for (z = 0; z < nOrder; z += 64)
                EXPAND_SIMD_2(n, 32, ADAPT_AVX512_SIMD_SHORT_ADD)
            break;
        }
    }
    else if (nDirection > 0)
    {
        switch (nOrder)
        {
        case 32:
            ADAPT_AVX512_SIMD_SHORT_SUB
            break;
        default:
            for (z = 0; z < nOrder; z += 64)
                EXPAND_SIMD_2(n, 32, ADAPT_AVX512_SIMD_SHORT_SUB)
            break;
        }
    }
}

#define ADAPT_AVX512_SIMD_INT_ADD                                \
{                                                                \
    const __m512i avxM = _mm512_load_si512(&pM[z + n]);          \
    const __m512i avxAdapt = _mm512_loadu_si512(&pAdapt[z + n]); \
    const __m512i avxNew = _mm512_add_epi32(avxM, avxAdapt);     \
    _mm512_store_si512(&pM[z + n], avxNew);                      \
}

#define ADAPT_AVX512_SIMD_INT_SUB                                \
{                                                                \
    const __m512i avxM = _mm512_load_si512(&pM[z + n]);          \
    const __m512i avxAdapt = _mm512_loadu_si512(&pAdapt[z + n]); \
    const __m512i avxNew = _mm512_sub_epi32(avxM, avxAdapt);     \
    _mm512_store_si512(&pM[z + n], avxNew);                      \
}

static void AdaptAVX512(int * pM, const int * pAdapt, int64 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((reinterpret_cast<size_t>(pM) % 64) == 0);

    // we're working up to 32 elements at a time
    ASSERT(nOrder == 16 || (nOrder % 32) == 0);

    int z = 0, n = 0;
    if (nDirection < 0)
    {
        switch (nOrder)
        {
        case 16:
            ADAPT_AVX512_SIMD_INT_ADD
            break;
        default:
            for (z = 0; z < nOrder; z += 32)
                EXPAND_SIMD_2(n, 16, ADAPT_AVX512_SIMD_INT_ADD)
            break;
        }
    }
    else if (nDirection > 0)
    {
        switch (nOrder)
        {
        case 16:
            ADAPT_AVX512_SIMD_INT_SUB
            break;
        default:
            for (z = 0; z < nOrder; z += 32)
                EXPAND_SIMD_2(n, 16, ADAPT_AVX512_SIMD_INT_SUB)
            break;
        }
    }
}

static int32 CalculateDotProductAVX512(const short * pA, const short * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((reinterpret_cast<size_t>(pB) % 64) == 0);

    // we're working 32 elements at a time
    ASSERT((nOrder % 32) == 0);

    // loop
    __m512i avxSum = _mm512_setzero_si512();
    for (int z = 0; z < nOrder; z += 32)
    {
        const __m512i avxA = _mm512_loadu_si512(&pA[z]);
        const __m512i avxB = _mm512_load_si512(&pB[z]);

        const __m512i avxDotProduct = _mm512_madd_epi16(avxA, avxB);

        avxSum = _mm512_add_epi32(avxSum, avxDotProduct);
    }

    // build output
    return _mm512_reduce_add_epi32(avxSum);
}

static int64 CalculateDotProductAVX512(const int * pA, const int * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((reinterpret_cast<size_t>(pB) % 64) == 0);

    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    // loop
    __m512i avxSumLo = _mm512_setzero_si512();
    __m512i avxSumHi = _mm512_setzero_si512();
    for (int z = 0; z < nOrder; z += 16)
    {
        const __m512i avxA = _mm512_loadu_si512(&pA[z]);
        const __m512i avxB = _mm512_load_si512(&pB[z]);

        const __m512i avxProduct = _mm512_mullo_epi32(avxA, avxB);

        const __m512i avxProductLo = _mm512_cvtepi32_epi64(_mm512_castsi512_si256(avxProduct));
        const __m512i avxProductHi = _mm512_cvtepi32_epi64(_mm512_extracti32x8_epi32(avxProduct, 0x1));

        avxSumLo = _mm512_add_epi64(avxSumLo, avxProductLo);
        avxSumHi = _mm512_add_epi64(avxSumHi, avxProductHi);
    }

    // build output
    const __m512i avxSum = _mm512_add_epi64(avxSumLo, avxSumHi);

    return _mm512_reduce_add_epi64(avxSum);
}
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::CompressAVX512(INTTYPE nInput)
{
#ifdef APE_USE_AVX512_INTRINSICS
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductAVX512(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput = static_cast<INTTYPE>(nInput - ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptAVX512(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);

    // update delta
    UPDATE_DELTA_NEW(nInput)

    // convert the input to a short and store it
    m_rbInput[0] = GetSaturatedShortFromInt(nInput);

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    _mm256_zeroupper();

    return nOutput;
#else
    (void) nInput;
    return 0;
#endif
}

template int CNNFilter<int, short>::CompressAVX512(int nInput);
template int64 CNNFilter<int64, int>::CompressAVX512(int64 nInput);

template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::DecompressAVX512(INTTYPE nInput)
{
#ifdef APE_USE_AVX512_INTRINSICS
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductAVX512(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput;
    if (m_bInterimMode)
        nOutput = static_cast<INTTYPE>(nInput + ((static_cast<int64>(nDotProduct) + m_nOneShiftedByShift) >> m_nShift));
    else
        nOutput = static_cast<INTTYPE>(nInput + ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptAVX512(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);

    // update delta
    if ((m_nVersion == -1) || (m_nVersion >= 3980))
        UPDATE_DELTA_NEW(nOutput)
    else
        UPDATE_DELTA_OLD(nOutput)

    // update the input buffer
    m_rbInput[0] = GetSaturatedShortFromInt(nOutput);

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    _mm256_zeroupper();

    return nOutput;
#else
    (void) nInput;
    return 0;
#endif
}

template int CNNFilter<int, short>::DecompressAVX512(int nInput);
template int64 CNNFilter<int64, int>::DecompressAVX512(int64 nInput);
#endif

}
