#pragma once

#include "All.h"

namespace APE
{

#include "RollBuffer.h"
#define NN_WINDOW_ELEMENTS    4096
class IPredictorDecompress;

template <class INTTYPE> class CNNFilter
{
public:
    CNNFilter(int nOrder, int nShift, int nVersion);
    ~CNNFilter();

    INTTYPE Compress(INTTYPE nInput);
    INTTYPE Decompress(INTTYPE nInput);
    void Flush();

    void SetLegacyDecode(bool bLegacyDecode) { m_bLegacyDecode = bLegacyDecode; }

private:
    int m_nOrder;
    int m_nShift;
    int m_nOneShiftedByShift;
    int m_nVersion;
    INTTYPE m_nRunningAverage;
    APE::CRollBuffer<short> m_rbInput16;
    APE::CRollBuffer<short> m_rbDeltaM16;
    APE::CRollBuffer<int> m_rbInput32;
    APE::CRollBuffer<int> m_rbDeltaM32;
    short * m_paryM16;
    int * m_paryM32;
    bool m_bLegacyDecode;
    bool m_bSSEAvailable;
    bool m_bAVX2Available;
    bool useSSE2() const { return m_bSSEAvailable; }
    bool useAVX2() const { return m_bAVX2Available; }

    __forceinline short GetSaturatedShortFromInt(INTTYPE nValue) const
    {
        short sValue = short(nValue);
        if (sValue != nValue)
            sValue = (nValue >> (8 * sizeof(INTTYPE) - 1)) ^ 0x7FFF;
        return sValue;
    }
};

}
