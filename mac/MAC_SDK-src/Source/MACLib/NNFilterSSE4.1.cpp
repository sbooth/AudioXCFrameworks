#include "NNFilter.h"
#include "NNFilterCommon.h"
#include "CPUFeatures.h"

#if defined(__SSE4_1__) || (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)))
    #define APE_USE_SSE41_INTRINSICS
#endif

#ifdef APE_USE_SSE41_INTRINSICS
    #include <smmintrin.h> // SSE4.1
#endif

namespace APE
{

void AdaptSSE2(short * pM, const short * pAdapt, int32 nDirection, int nOrder);
void AdaptSSE2(int * pM, const int * pAdapt, int64 nDirection, int nOrder);

int32 CalculateDotProductSSE2(const short * pA, const short * pB, int nOrder);

bool GetSSE41Available()
{
#ifdef APE_USE_SSE41_INTRINSICS
    return true;
#else
    return false;
#endif
}

#ifdef APE_USE_SSE41_INTRINSICS
static int32 CalculateDotProductSSE41(const short * pA, const short * pB, int nOrder)
{
    return CalculateDotProductSSE2(pA, pB, nOrder);
}

static int64 CalculateDotProductSSE41(const int * pA, const int * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((reinterpret_cast<size_t>(pB) % 16) == 0);

    // we're working 8 elements at a time
    ASSERT((nOrder % 8) == 0);

    // loop
    __m128i sseSum1Lo = _mm_setzero_si128();
    __m128i sseSum1Hi = _mm_setzero_si128();
    __m128i sseSum2Lo = _mm_setzero_si128();
    __m128i sseSum2Hi = _mm_setzero_si128();
    for (int z = 0; z < nOrder; z += 8)
    {
        const __m128i sseA1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pA[z]));
        const __m128i sseB1 = _mm_load_si128(reinterpret_cast<const __m128i *>(&pB[z]));
        const __m128i sseA2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pA[z + 4]));
        const __m128i sseB2 = _mm_load_si128(reinterpret_cast<const __m128i *>(&pB[z + 4]));

        const __m128i sseProduct1 = _mm_mullo_epi32(sseA1, sseB1);
        const __m128i sseProduct2 = _mm_mullo_epi32(sseA2, sseB2);

        const __m128i sseProduct1Lo = _mm_cvtepi32_epi64(sseProduct1);
        const __m128i sseProduct1Hi = _mm_cvtepi32_epi64(_mm_srli_si128(sseProduct1, 0x8));
        const __m128i sseProduct2Lo = _mm_cvtepi32_epi64(sseProduct2);
        const __m128i sseProduct2Hi = _mm_cvtepi32_epi64(_mm_srli_si128(sseProduct2, 0x8));

        sseSum1Lo = _mm_add_epi64(sseSum1Lo, sseProduct1Lo);
        sseSum1Hi = _mm_add_epi64(sseSum1Hi, sseProduct1Hi);
        sseSum2Lo = _mm_add_epi64(sseSum2Lo, sseProduct2Lo);
        sseSum2Hi = _mm_add_epi64(sseSum2Hi, sseProduct2Hi);
    }

    // build output
    __m128i sseSum = _mm_add_epi64(_mm_add_epi64(sseSum1Lo, sseSum1Hi), _mm_add_epi64(sseSum2Lo, sseSum2Hi));
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
template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::CompressSSE41(INTTYPE nInput)
{
#ifdef APE_USE_SSE41_INTRINSICS
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductSSE41(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput = static_cast<INTTYPE>(nInput - ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptSSE2(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);

    // update delta
    UPDATE_DELTA_NEW(nInput)

    // convert the input to a short and store it
    m_rbInput[0] = GetSaturatedShortFromInt(nInput);

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    return nOutput;
#else
    (void) nInput;
    return 0;
#endif
}

template int CNNFilter<int, short>::CompressSSE41(int nInput);
template int64 CNNFilter<int64, int>::CompressSSE41(int64 nInput);

template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::DecompressSSE41(INTTYPE nInput)
{
#ifdef APE_USE_SSE41_INTRINSICS
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductSSE41(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput;
    if (m_bInterimMode)
        nOutput = static_cast<INTTYPE>(nInput + ((static_cast<int64>(nDotProduct) + m_nOneShiftedByShift) >> m_nShift));
    else
        nOutput = static_cast<INTTYPE>(nInput + ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptSSE2(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);

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

    return nOutput;
#else
    (void) nInput;
    return 0;
#endif
}

template int CNNFilter<int, short>::DecompressSSE41(int nInput);
template int64 CNNFilter<int64, int>::DecompressSSE41(int64 nInput);
#endif

}
