#include "NNFilter.h"
#include "NNFilterCommon.h"
#include "CPUFeatures.h"

#if defined(__AVX2__) || (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)) && !defined(_M_ARM64EC))
    #define APE_USE_AVX2_INTRINSICS
#endif

#ifdef APE_USE_AVX2_INTRINSICS
    #include <immintrin.h> // AVX2
#endif

namespace APE
{

bool GetAVX2Available()
{
#ifdef APE_USE_AVX2_INTRINSICS
    return true;
#else
    return false;
#endif
}

#ifdef APE_USE_AVX2_INTRINSICS

#define ADAPT_AVX2_SIMD_SHORT_ADD                                                                   \
{                                                                                                   \
    const __m256i avxM = _mm256_load_si256(reinterpret_cast<__m256i *>(&pM[z + n]));                \
    const __m256i avxAdapt = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&pAdapt[z + n])); \
    const __m256i avxNew = _mm256_add_epi16(avxM, avxAdapt);                                        \
    _mm256_store_si256(reinterpret_cast<__m256i *>(&pM[z + n]), avxNew);                            \
}

#define ADAPT_AVX2_SIMD_SHORT_SUB                                                                   \
{                                                                                                   \
    const __m256i avxM = _mm256_load_si256(reinterpret_cast<__m256i *>(&pM[z + n]));                \
    const __m256i avxAdapt = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&pAdapt[z + n])); \
    const __m256i avxNew = _mm256_sub_epi16(avxM, avxAdapt);                                        \
    _mm256_store_si256(reinterpret_cast<__m256i *>(&pM[z + n]), avxNew);                            \
}

static void AdaptAVX2(short * pM, const short * pAdapt, int32 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((reinterpret_cast<size_t>(pM) % 32) == 0);

    // we're working up to 64 elements at a time
    ASSERT(nOrder == 16 || nOrder == 32 || (nOrder % 64) == 0);

    int z = 0, n = 0;
    if (nDirection < 0)
    {
        switch (nOrder)
        {
        case 16:
            ADAPT_AVX2_SIMD_SHORT_ADD
            break;
        case 32:
            EXPAND_SIMD_2(n, 16, ADAPT_AVX2_SIMD_SHORT_ADD)
            break;
        default:
            for (z = 0; z < nOrder; z += 64)
                EXPAND_SIMD_4(n, 16, ADAPT_AVX2_SIMD_SHORT_ADD)
            break;
        }
    }
    else if (nDirection > 0)
    {
        switch (nOrder)
        {
        case 16:
            ADAPT_AVX2_SIMD_SHORT_SUB
            break;
        case 32:
            EXPAND_SIMD_2(n, 16, ADAPT_AVX2_SIMD_SHORT_SUB)
            break;
        default:
            for (z = 0; z < nOrder; z += 64)
                EXPAND_SIMD_4(n, 16, ADAPT_AVX2_SIMD_SHORT_SUB)
            break;
        }
    }
}

#define ADAPT_AVX2_SIMD_INT_ADD                                                                     \
{                                                                                                   \
    const __m256i avxM = _mm256_load_si256(reinterpret_cast<__m256i *>(&pM[z + n]));                \
    const __m256i avxAdapt = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&pAdapt[z + n])); \
    const __m256i avxNew = _mm256_add_epi32(avxM, avxAdapt);                                        \
    _mm256_store_si256(reinterpret_cast<__m256i *>(&pM[z + n]), avxNew);                            \
}

#define ADAPT_AVX2_SIMD_INT_SUB                                                                     \
{                                                                                                   \
    const __m256i avxM = _mm256_load_si256(reinterpret_cast<__m256i *>(&pM[z + n]));                \
    const __m256i avxAdapt = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&pAdapt[z + n])); \
    const __m256i avxNew = _mm256_sub_epi32(avxM, avxAdapt);                                        \
    _mm256_store_si256(reinterpret_cast<__m256i *>(&pM[z + n]), avxNew);                            \
}

static void AdaptAVX2(int * pM, const int * pAdapt, int64 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((reinterpret_cast<size_t>(pM) % 32) == 0);

    // we're working up to 32 elements at a time
    ASSERT(nOrder == 16 || (nOrder % 32) == 0);

    int z = 0, n = 0;
    if (nDirection < 0)
    {
        switch (nOrder)
        {
        case 16:
            EXPAND_SIMD_2(n, 8, ADAPT_AVX2_SIMD_INT_ADD)
            break;
        default:
            for (z = 0; z < nOrder; z += 32)
                EXPAND_SIMD_4(n, 8, ADAPT_AVX2_SIMD_INT_ADD)
            break;
        }
    }
    else if (nDirection > 0)
    {
        switch (nOrder)
        {
        case 16:
            EXPAND_SIMD_2(n, 8, ADAPT_AVX2_SIMD_INT_SUB)
            break;
        default:
            for (z = 0; z < nOrder; z += 32)
                EXPAND_SIMD_4(n, 8, ADAPT_AVX2_SIMD_INT_SUB)
            break;
        }
    }
}

static int32 CalculateDotProductAVX2(const short * pA, const short * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((reinterpret_cast<size_t>(pB) % 32) == 0);

    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    // loop
    __m256i avxSum = _mm256_setzero_si256();
    for (int z = 0; z < nOrder; z += 16)
    {
        const __m256i avxA = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&pA[z]));
        const __m256i avxB = _mm256_load_si256(reinterpret_cast<const __m256i *>(&pB[z]));

        const __m256i avxDotProduct = _mm256_madd_epi16(avxA, avxB);

        avxSum = _mm256_add_epi32(avxSum, avxDotProduct);
    }

    // build output
    const __m128i lo128 = _mm256_castsi256_si128(avxSum);
    const __m128i hi128 = _mm256_extracti128_si256(avxSum, 0x1);

    __m128i sseSum = _mm_add_epi32(lo128, hi128);
    __m128i sseShift = _mm_srli_si128(sseSum, 0x8);

    sseSum = _mm_add_epi32(sseSum, sseShift);
    sseShift = _mm_srli_si128(sseSum, 0x4);
    sseSum = _mm_add_epi32(sseSum, sseShift);

    return _mm_cvtsi128_si32(sseSum);
}

static int64 CalculateDotProductAVX2(const int * pA, const int * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((reinterpret_cast<size_t>(pB) % 32) == 0);

    // we're working 8 elements at a time
    ASSERT((nOrder % 8) == 0);

    // loop
    __m256i avxSumLo = _mm256_setzero_si256();
    __m256i avxSumHi = _mm256_setzero_si256();
    for (int z = 0; z < nOrder; z += 8)
    {
        const __m256i avxA = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&pA[z]));
        const __m256i avxB = _mm256_load_si256(reinterpret_cast<const __m256i *>(&pB[z]));

        const __m256i avxProduct = _mm256_mullo_epi32(avxA, avxB);

        const __m256i avxProductLo = _mm256_cvtepi32_epi64(_mm256_castsi256_si128(avxProduct));
        const __m256i avxProductHi = _mm256_cvtepi32_epi64(_mm256_extracti128_si256(avxProduct, 0x1));

        avxSumLo = _mm256_add_epi64(avxSumLo, avxProductLo);
        avxSumHi = _mm256_add_epi64(avxSumHi, avxProductHi);
    }

    // build output
    const __m256i avxSum = _mm256_add_epi64(avxSumLo, avxSumHi);

    const __m128i lo128 = _mm256_castsi256_si128(avxSum);
    const __m128i hi128 = _mm256_extracti128_si256(avxSum, 0x1);

    __m128i sseSum = _mm_add_epi64(lo128, hi128);
    const __m128i sseShift = _mm_srli_si128(sseSum, 0x8);

    sseSum = _mm_add_epi64(sseSum, sseShift);

#if !defined(__x86_64__) && !defined(_M_X64)
    return static_cast<int64>(_mm_extract_epi32(sseSum, 1)) << 32 | static_cast<uint32>(_mm_cvtsi128_si32(sseSum));
#else
    return _mm_cvtsi128_si64(sseSum);
#endif
}
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::CompressAVX2(INTTYPE nInput)
{
#ifdef APE_USE_AVX2_INTRINSICS
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductAVX2(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput = static_cast<INTTYPE>(nInput - ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptAVX2(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);

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

template int CNNFilter<int, short>::CompressAVX2(int nInput);
template int64 CNNFilter<int64, int>::CompressAVX2(int64 nInput);

template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::DecompressAVX2(INTTYPE nInput)
{
#ifdef APE_USE_AVX2_INTRINSICS
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductAVX2(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput;
    if (m_bInterimMode)
        nOutput = static_cast<INTTYPE>(nInput + ((static_cast<int64>(nDotProduct) + m_nOneShiftedByShift) >> m_nShift));
    else
        nOutput = static_cast<INTTYPE>(nInput + ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptAVX2(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);

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

template int CNNFilter<int, short>::DecompressAVX2(int nInput);
template int64 CNNFilter<int64, int>::DecompressAVX2(int64 nInput);
#endif

}
