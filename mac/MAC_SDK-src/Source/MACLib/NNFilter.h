#pragma once

#include "All.h"
#include "RollBuffer.h"

#define NN_WINDOW_ELEMENTS 512

namespace APE
{

class IPredictorDecompress;

#pragma pack(push, 1)

/**************************************************************************************************
CNNFilter
**************************************************************************************************/
template <class INTTYPE, class DATATYPE> class CNNFilter
{
public:
    CNNFilter(int nOrder, int nShift, int nVersion = -1);
    virtual ~CNNFilter();

    INTTYPE Compress(INTTYPE nInput) { return (this->*CompressImpl)(nInput); }
    INTTYPE Decompress(INTTYPE nInput) { return (this->*DecompressImpl)(nInput); }
    void Flush();

    void SetInterimMode(bool bInterimMode) { m_bInterimMode = bInterimMode; }

private:
    INTTYPE (CNNFilter<INTTYPE, DATATYPE>::*CompressImpl)(INTTYPE nInput);
    INTTYPE (CNNFilter<INTTYPE, DATATYPE>::*DecompressImpl)(INTTYPE nInput);

    INTTYPE CompressGeneric(INTTYPE nInput);
    INTTYPE DecompressGeneric(INTTYPE nInput);

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    INTTYPE CompressSSE2(INTTYPE nInput);
    INTTYPE DecompressSSE2(INTTYPE nInput);

    INTTYPE CompressSSE41(INTTYPE nInput);
    INTTYPE DecompressSSE41(INTTYPE nInput);

    INTTYPE CompressAVX2(INTTYPE nInput);
    INTTYPE DecompressAVX2(INTTYPE nInput);

    INTTYPE CompressAVX512(INTTYPE nInput);
    INTTYPE DecompressAVX512(INTTYPE nInput);
#endif

#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
    INTTYPE CompressNeon(INTTYPE nInput);
    INTTYPE DecompressNeon(INTTYPE nInput);
#endif

    const int m_nOrder;
    const int m_nShift;
    const int m_nOneShiftedByShift;
    const int m_nVersion;
    DATATYPE * m_paryM;
    APE::CRollBuffer<DATATYPE, NN_WINDOW_ELEMENTS> m_rbInput;
    APE::CRollBuffer<DATATYPE, NN_WINDOW_ELEMENTS> m_rbDeltaM;
    bool m_bInterimMode;
    INTTYPE m_nRunningAverage;

private:
    // silence warning about implicitly deleted assignment operator
    CNNFilter<INTTYPE, DATATYPE> & operator=(const CNNFilter<INTTYPE, DATATYPE> & Copy) { }
};

// forward declare the CNNFilter classes because it helps with a Clang warning
#ifdef _MSC_VER
extern template class CNNFilter<int, short>;
extern template class CNNFilter<int64, int>;
#endif

#pragma pack(pop)

}
