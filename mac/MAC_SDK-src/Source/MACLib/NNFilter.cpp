#include "All.h"
#include "GlobalFunctions.h"
#include "NNFilter.h"

#ifdef ENABLE_SSE_ASSEMBLY
    #include <emmintrin.h> // SSE 2
#endif

#ifdef ENABLE_AVX_ASSEMBLY
    #include <immintrin.h> // AVX
#endif

namespace APE
{

static __forceinline void Adapt(int * pM, int * pAdapt, int64 nDirection, int nOrder);
static __forceinline void Adaptx16(short * pM, short * pAdapt, int64 nDirection, int nOrder);
static __forceinline int64 CalculateDotProduct(int * pA, int * pB, int nOrder);
static __forceinline int64 CalculateDotProductx16(short * pA, short * pB, int nOrder);
    
#ifdef ENABLE_SSE_ASSEMBLY
    #if !defined(_MSC_VER)
        typedef union __attribute__ ((aligned (16))) __sseWord {
            __m128i m128i;
            int32_t m128i_i32[4];
        } __sseWord;
    #endif

    static __forceinline void AdaptSSE(int * pM, int * pAdapt, int64 nDirection, int nOrder);
    static __forceinline void AdaptSSEx16(short * pM, short * pAdapt, int64 nDirection, int nOrder);
    static __forceinline int64 CalculateDotProductSSEx16(short * pA, short * pB, int nOrder);
#endif

#ifdef ENABLE_AVX_ASSEMBLY
    #if !defined(_MSC_VER)
        typedef union __attribute__ ((aligned (32))) __avxWord {
            __m256i m256i;
            int32_t m256i_i32[8];
        } __avxWord;
    #endif

    static __forceinline void AdaptAVX(int * pM, int * pAdapt, int64 nDirection, int nOrder);
    static __forceinline void AdaptAVXx16(short * pM, short * pAdapt, int64 nDirection, int nOrder);
    static __forceinline int64 CalculateDotProductAVXx16(short * pA, short * pB, int nOrder);
#endif

template <class INTTYPE> CNNFilter<INTTYPE>::CNNFilter(int nOrder, int nShift, int nVersion)
{
    if (nOrder <= 0)
        throw(1);
    else if ((nOrder != 16) && ((nOrder % 32) != 0)) 
        throw(1);

    m_nOrder = nOrder;
    m_nShift = nShift;
    m_nOneShiftedByShift = int(1 << (m_nShift - 1));
    m_nVersion = nVersion;
    m_bLegacyDecode = false;
    m_bSSEAvailable = GetSSEAvailable(false);
    m_bAVX2Available = GetAVX2Available();
    m_nRunningAverage = 0;

    m_rbInput16.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_rbDeltaM16.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_rbInput32.Create(NN_WINDOW_ELEMENTS, m_nOrder);
    m_rbDeltaM32.Create(NN_WINDOW_ELEMENTS, m_nOrder);

    m_paryM16 = (short *) AllocateAligned(intn(sizeof(short)) * m_nOrder, 32); // align for possible SSE usage
    m_paryM32 = (int *) AllocateAligned(intn(sizeof(int)) * m_nOrder, 32); // align for possible SSE usage
}

template <class INTTYPE> CNNFilter<INTTYPE>::~CNNFilter()
{
    if (m_paryM16 != NULL)
    {
        FreeAligned(m_paryM16);
        m_paryM16 = NULL;
    }
    if (m_paryM32 != NULL)
    {
        FreeAligned(m_paryM32);
        m_paryM32 = NULL;
    }
}

template <class INTTYPE> void CNNFilter<INTTYPE>::Flush()
{
    memset(&m_paryM16[0], 0, m_nOrder * sizeof(short));
    memset(&m_paryM32[0], 0, m_nOrder * sizeof(int));
    m_rbInput16.Flush();
    m_rbDeltaM16.Flush();
    m_rbInput32.Flush();
    m_rbDeltaM32.Flush();
    m_nRunningAverage = 0;
}

template <class INTTYPE> INTTYPE CNNFilter<INTTYPE>::Compress(INTTYPE nInput)
{
    switch (sizeof(INTTYPE))
    {
    case 4: // 4 byte (int)
    {
        // convert the input to a short and store it
        m_rbInput16[0] = GetSaturatedShortFromInt(nInput);

        // figure a dot product
        int64 nDotProduct;
        #ifdef ENABLE_AVX_ASSEMBLY
            if (this->useAVX2())
                nDotProduct = CalculateDotProductAVXx16(&m_rbInput16[-m_nOrder], &m_paryM16[0], m_nOrder);
            else
        #endif
        #ifdef ENABLE_SSE_ASSEMBLY
            if (this->useSSE2())
                nDotProduct = CalculateDotProductSSEx16(&m_rbInput16[-m_nOrder], &m_paryM16[0], m_nOrder);
            else
        #endif
                nDotProduct = CalculateDotProductx16(&m_rbInput16[-m_nOrder], &m_paryM16[0], m_nOrder);

        // calculate the output
        #ifdef LEGACY_ENCODE
            INTTYPE nOutput = INTTYPE(nInput - ((int(nDotProduct) + m_nOneShiftedByShift) >> m_nShift));
        #else
            INTTYPE nOutput = INTTYPE(nInput - ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));
        #endif
        
        // adapt
        #ifdef ENABLE_AVX_ASSEMBLY
            if (this->useAVX2())
                AdaptAVXx16(&m_paryM16[0], &m_rbDeltaM16[-m_nOrder], nOutput, m_nOrder);
            else
        #endif
        #ifdef ENABLE_SSE_ASSEMBLY
            if (this->useSSE2())
                AdaptSSEx16(&m_paryM16[0], &m_rbDeltaM16[-m_nOrder], nOutput, m_nOrder);
            else
        #endif
                Adaptx16(&m_paryM16[0], &m_rbDeltaM16[-m_nOrder], nOutput, m_nOrder);

        INTTYPE nTempABS = labs((int) nInput);

        if (nTempABS > (m_nRunningAverage * 3))
            m_rbDeltaM16[0] = ((nInput >> 25) & 64) - 32;
        else if (nTempABS > (m_nRunningAverage * 4) / 3)
            m_rbDeltaM16[0] = ((nInput >> 26) & 32) - 16;
        else if (nTempABS > 0)
            m_rbDeltaM16[0] = ((nInput >> 27) & 16) - 8;
        else
            m_rbDeltaM16[0] = 0;

        m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

        m_rbDeltaM16[-1] >>= 1;
        m_rbDeltaM16[-2] >>= 1;
        m_rbDeltaM16[-8] >>= 1;

        // increment and roll if necessary
        m_rbInput16.IncrementSafe();
        m_rbDeltaM16.IncrementSafe();

        return nOutput;
    }
    default: // 8 byte (int64)
    {
        // convert the input to a short and store it
        m_rbInput32[0] = GetSaturatedShortFromInt(nInput);

        // figure a dot product
        int64 nDotProduct = CalculateDotProduct(&m_rbInput32[-m_nOrder], &m_paryM32[0], m_nOrder);

        // calculate the output
        INTTYPE nOutput = INTTYPE(nInput - ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

        // adapt
        #ifdef ENABLE_AVX_ASSEMBLY
            if (this->useAVX2())
                AdaptAVX(&m_paryM32[0], &m_rbDeltaM32[-m_nOrder], nOutput, m_nOrder);
            else
        #endif
        #ifdef ENABLE_SSE_ASSEMBLY
            if (this->useSSE2())
                AdaptSSE(&m_paryM32[0], &m_rbDeltaM32[-m_nOrder], nOutput, m_nOrder);
            else
        #endif
                Adapt(&m_paryM32[0], &m_rbDeltaM32[-m_nOrder], nOutput, m_nOrder);

        INTTYPE nTempABS = (INTTYPE) llabs(nInput);

        if (nTempABS > (m_nRunningAverage * 3))
            m_rbDeltaM32[0] = ((nInput >> 25) & 64) - 32;
        else if (nTempABS > (m_nRunningAverage * 4) / 3)
            m_rbDeltaM32[0] = ((nInput >> 26) & 32) - 16;
        else if (nTempABS > 0)
            m_rbDeltaM32[0] = ((nInput >> 27) & 16) - 8;
        else
            m_rbDeltaM32[0] = 0;

        m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

        m_rbDeltaM32[-1] >>= 1;
        m_rbDeltaM32[-2] >>= 1;
        m_rbDeltaM32[-8] >>= 1;

        // increment and roll if necessary
        m_rbInput32.IncrementSafe();
        m_rbDeltaM32.IncrementSafe();

        return nOutput;
    }
    }
}

template <class INTTYPE> INTTYPE CNNFilter<INTTYPE>::Decompress(INTTYPE nInput)
{
    switch (sizeof(INTTYPE))
    {
    case 4: // 4 byte (int)
    {
        // figure a dot product
        int64 nDotProduct;
        #ifdef ENABLE_AVX_ASSEMBLY
            if (this->useAVX2())
                nDotProduct = CalculateDotProductAVXx16(&m_rbInput16[-m_nOrder], &m_paryM16[0], m_nOrder);
            else
        #endif
        #ifdef ENABLE_SSE_ASSEMBLY
            if (this->useSSE2())
                nDotProduct = CalculateDotProductSSEx16(&m_rbInput16[-m_nOrder], &m_paryM16[0], m_nOrder);
            else
        #endif
                nDotProduct = CalculateDotProductx16(&m_rbInput16[-m_nOrder], &m_paryM16[0], m_nOrder);

        // adapt
        #ifdef ENABLE_AVX_ASSEMBLY
            if (this->useAVX2())
                AdaptAVXx16(&m_paryM16[0], &m_rbDeltaM16[-m_nOrder], nInput, m_nOrder);
            else
        #endif
        #ifdef ENABLE_SSE_ASSEMBLY
            if (this->useSSE2())
                AdaptSSEx16(&m_paryM16[0], &m_rbDeltaM16[-m_nOrder], nInput, m_nOrder);
            else
        #endif
                Adaptx16(&m_paryM16[0], &m_rbDeltaM16[-m_nOrder], nInput, m_nOrder);

        // store the output value
        INTTYPE nOutput;
        if (m_bLegacyDecode)
            nOutput = INTTYPE(nInput + ((int(nDotProduct) + m_nOneShiftedByShift) >> m_nShift));
        else
            nOutput = INTTYPE(nInput + ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

        // update the input buffer
        m_rbInput16[0] = GetSaturatedShortFromInt(nOutput);

        if (m_nVersion >= 3980)
        {
            INTTYPE nTempABS = labs((int) nOutput);

            if (nTempABS > (m_nRunningAverage * 3))
                m_rbDeltaM16[0] = ((nOutput >> 25) & 64) - 32;
            else if (nTempABS > (m_nRunningAverage * 4) / 3)
                m_rbDeltaM16[0] = ((nOutput >> 26) & 32) - 16;
            else if (nTempABS > 0)
                m_rbDeltaM16[0] = ((nOutput >> 27) & 16) - 8;
            else
                m_rbDeltaM16[0] = 0;

            m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

            m_rbDeltaM16[-1] >>= 1;
            m_rbDeltaM16[-2] >>= 1;
            m_rbDeltaM16[-8] >>= 1;
        }
        else
        {
            m_rbDeltaM16[0] = (nOutput == 0) ? 0 : ((nOutput >> 28) & 8) - 4;
            m_rbDeltaM16[-4] >>= 1;
            m_rbDeltaM16[-8] >>= 1;
        }

        // increment and roll if necessary
        m_rbInput16.IncrementSafe();
        m_rbDeltaM16.IncrementSafe();

        return nOutput;
    }
    default: // 8 byte (int64)
    {
        // figure a dot product
        int64 nDotProduct = CalculateDotProduct(&m_rbInput32[-m_nOrder], &m_paryM32[0], m_nOrder);

        // adapt
        #ifdef ENABLE_AVX_ASSEMBLY
            if (this->useAVX2())
                AdaptAVX(&m_paryM32[0], &m_rbDeltaM32[-m_nOrder], nInput, m_nOrder);
            else
        #endif
        #ifdef ENABLE_SSE_ASSEMBLY
            if (this->useSSE2())
                AdaptSSE(&m_paryM32[0], &m_rbDeltaM32[-m_nOrder], nInput, m_nOrder);
            else
        #endif
                Adapt(&m_paryM32[0], &m_rbDeltaM32[-m_nOrder], nInput, m_nOrder);

        // store the output value
        INTTYPE nOutput = INTTYPE(nInput + ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));
    
        // update the input buffer
        m_rbInput32[0] = GetSaturatedShortFromInt(nOutput);

        if (m_nVersion >= 3980)
        {
            INTTYPE nTempABS = (INTTYPE) llabs(nOutput);

            if (nTempABS > (m_nRunningAverage * 3))
                m_rbDeltaM32[0] = ((nOutput >> 25) & 64) - 32;
            else if (nTempABS > (m_nRunningAverage * 4) / 3)
                m_rbDeltaM32[0] = ((nOutput >> 26) & 32) - 16;
            else if (nTempABS > 0)
                m_rbDeltaM32[0] = ((nOutput >> 27) & 16) - 8;
            else
                m_rbDeltaM32[0] = 0;

            m_nRunningAverage += (nTempABS - m_nRunningAverage) / 16;

            m_rbDeltaM32[-1] >>= 1;
            m_rbDeltaM32[-2] >>= 1;
            m_rbDeltaM32[-8] >>= 1;
        }
        else
        {
            m_rbDeltaM32[0] = (nOutput == 0) ? 0 : ((nOutput >> 28) & 8) - 4;
            m_rbDeltaM32[-4] >>= 1;
            m_rbDeltaM32[-8] >>= 1;
        }

        // increment and roll if necessary
        m_rbInput32.IncrementSafe();
        m_rbDeltaM32.IncrementSafe();
    
        return nOutput;
    }
    }
}

void Adapt(int * pM, int * pAdapt, int64 nDirection, int nOrder)
{
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

void Adaptx16(short * pM, short * pAdapt, int64 nDirection, int nOrder)
{
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

int64 CalculateDotProduct(int * pA, int * pB, int nOrder)
{
    int64 nDotProduct = 0;
    nOrder >>= 4;

    while (nOrder--)
    {
        EXPAND_16_TIMES(nDotProduct += *pA++ * *pB++;)
    }
    
    return nDotProduct;
}

int64 CalculateDotProductx16(short * pA, short * pB, int nOrder)
{
    // calculate the same was as the SSE and AVX paths so overflows are handled the same way
    int64 nDotProduct = 0;
    int nTotal[4] = { 0 };
    while (nOrder)
    {
        nTotal[0] += int(*pA++) * int(*pB++);
        nTotal[1] += int(*pA++) * int(*pB++);
        nTotal[2] += int(*pA++) * int(*pB++);
        nTotal[3] += int(*pA++) * int(*pB++);
        nOrder -= 4;
    }
    nDotProduct = nTotal[0] + nTotal[1] + nTotal[2] + nTotal[3];
    return nDotProduct;
}

#ifdef ENABLE_SSE_ASSEMBLY

void AdaptSSE(int * pM, int * pAdapt, int64 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((size_t(pM) % 16) == 0);

    if (nDirection < 0)
    {
        for (int z = 0; z < nOrder; z += 4)
        {
            __m128i sseM = _mm_load_si128((__m128i *) &pM[z]);
            __m128i sseAdapt = _mm_loadu_si128((__m128i *) &pAdapt[z]);
            __m128i sseNew = _mm_add_epi32(sseM, sseAdapt);
            _mm_store_si128((__m128i *) &pM[z], sseNew);
        }
    }
    else if (nDirection > 0)
    {
        for (int z = 0; z < nOrder; z += 4)
        {
            __m128i sseM = _mm_load_si128((__m128i *) &pM[z]);
            __m128i sseAdapt = _mm_loadu_si128((__m128i *) &pAdapt[z]);
            __m128i sseNew = _mm_sub_epi32(sseM, sseAdapt);
            _mm_store_si128((__m128i *) &pM[z], sseNew);
        }
    }
}

void AdaptSSEx16(short * pM, short * pAdapt, int64 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((size_t(pM) % 16) == 0);

    if (nDirection < 0)
    {
        for (int z = 0; z < nOrder; z += 8)
        {
            __m128i sseM = _mm_load_si128((__m128i *) &pM[z]);
            __m128i sseAdapt = _mm_loadu_si128((__m128i *) &pAdapt[z]);
            __m128i sseNew = _mm_add_epi16(sseM, sseAdapt);
            _mm_store_si128((__m128i *) &pM[z], sseNew);
        }
    }
    else if (nDirection > 0)
    {
        for (int z = 0; z < nOrder; z += 8)
        {
            __m128i sseM = _mm_load_si128((__m128i *) &pM[z]);
            __m128i sseAdapt = _mm_loadu_si128((__m128i *) &pAdapt[z]);
            __m128i sseNew = _mm_sub_epi16(sseM, sseAdapt);
            _mm_store_si128((__m128i *) &pM[z], sseNew);
        }
    }
}

int64 CalculateDotProductSSEx16(short * pA, short * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((size_t(pB) % 16) == 0);

    // loop
    __m128i sseSum = _mm_setzero_si128();
    for (int z = 0; z < nOrder; z += 8)
    {
        __m128i sseA = _mm_loadu_si128((__m128i *) &pA[z]);
        __m128i sseB = _mm_load_si128((__m128i *) &pB[z]);
        __m128i sseDotProduct = _mm_madd_epi16(sseA, sseB);
        sseSum = _mm_add_epi32(sseSum, sseDotProduct);
    }

#ifndef _MSC_VER
    __sseWord sseWord = { sseSum };
#else
    __m128i sseWord = sseSum;
#endif

    // build output
    int64 nDotProduct = sseWord.m128i_i32[0] + sseWord.m128i_i32[1] + sseWord.m128i_i32[2] + sseWord.m128i_i32[3];

    // TODO: SSE4 instructions might help performance of the horizontal add, for example:
    //int nDotProduct = _mm_extract_epi32(sseSum, 0) + _mm_extract_epi32(sseSum, 1) + _mm_extract_epi32(sseSum, 2) + _mm_extract_epi32(sseSum, 3);

    return nDotProduct;
}

#endif

#ifdef ENABLE_AVX_ASSEMBLY
void AdaptAVX(int * pM, int * pAdapt, int64 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((size_t(pM) % 32) == 0);
    ASSERT((size_t(nOrder) % 8) == 0);

    if (nDirection < 0)
    {
        for (int z = 0; z < nOrder; z += 8)
        {
            __m256i avxM = _mm256_load_si256((__m256i *) &pM[z]);
            __m256i avxAdapt = _mm256_loadu_si256((__m256i *) &pAdapt[z]);
            __m256i avxNew = _mm256_add_epi32(avxM, avxAdapt);
            _mm256_store_si256((__m256i *) & pM[z], avxNew);
        }
    }
    else if (nDirection > 0)
    {
        for (int z = 0; z < nOrder; z += 8)
        {
            __m256i avxM = _mm256_load_si256((__m256i *) &pM[z]);
            __m256i avxAdapt = _mm256_loadu_si256((__m256i *) &pAdapt[z]);
            __m256i avxNew = _mm256_sub_epi32(avxM, avxAdapt);
            _mm256_store_si256((__m256i *) &pM[z], avxNew);
        }
    }

    _mm256_zeroupper();
}

void AdaptAVXx16(short * pM, short * pAdapt, int64 nDirection, int nOrder)
{
    // we require that pM is aligned, allowing faster loads and stores
    ASSERT((size_t(pM) % 32) == 0);
    
    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    if (nDirection < 0)
    {
        for (int z = 0; z < nOrder; z += 16)
        {
            __m256i avxM = _mm256_load_si256((__m256i *) &pM[z]);
            __m256i avxAdapt = _mm256_loadu_si256((__m256i *) &pAdapt[z]);
            __m256i avxNew = _mm256_add_epi16(avxM, avxAdapt);
            _mm256_store_si256((__m256i *) &pM[z], avxNew);
        }
    }
    else if (nDirection > 0)
    {
        for (int z = 0; z < nOrder; z += 16)
        {
            __m256i avxM = _mm256_load_si256((__m256i *) &pM[z]);
            __m256i avxAdapt = _mm256_loadu_si256((__m256i *) &pAdapt[z]);
            __m256i avxNew = _mm256_sub_epi16(avxM, avxAdapt);
            _mm256_store_si256((__m256i *) &pM[z], avxNew);
        }
    }

    _mm256_zeroupper();
}

int64 CalculateDotProductAVXx16(short * pA, short * pB, int nOrder)
{
    // we require that pB is aligned, allowing faster loads
    ASSERT((size_t(pB) % 32) == 0);
    
    // we're working 16 elements at a time
    ASSERT((nOrder % 16) == 0);

    // loop
    __m256i avxSum = _mm256_setzero_si256();
    for (int z = 0; z < nOrder; z += 16)
    {
        __m256i avxA = _mm256_loadu_si256((__m256i *) &pA[z]);
        __m256i avxB = _mm256_load_si256((__m256i *) &pB[z]);
        __m256i avxDotProduct = _mm256_madd_epi16(avxA, avxB);
        avxSum = _mm256_add_epi32(avxSum, avxDotProduct);
    }

#ifndef _MSC_VER
    __avxWord avxWord = { avxSum };
#else
    __m256i avxWord = avxSum;
#endif

    // build output
    int64 nDotProduct = avxWord.m256i_i32[0] + avxWord.m256i_i32[1] + avxWord.m256i_i32[2] + avxWord.m256i_i32[3] +
        avxWord.m256i_i32[4] + avxWord.m256i_i32[5] + avxWord.m256i_i32[6] + avxWord.m256i_i32[7];

    _mm256_zeroupper();

    return nDotProduct;
}

#endif

template class CNNFilter<int>;
template class CNNFilter<int64>;

}