#pragma once

#include "APECompress.h"
#include "BitArray.h"

#ifdef APE_SUPPORT_COMPRESS

namespace APE
{

class CPrepare;
class IPredictorCompress;

#pragma pack(push, 1)

/**************************************************************************************************
CAPECompressCore - manages the core of compression and bitstream output
**************************************************************************************************/
class  CAPECompressCore
{
public:
    CAPECompressCore(CIO * pIO, const WAVEFORMATEX * pwfeInput, int nMaxFrameBlocks, int nCompressionLevel);
    virtual ~CAPECompressCore();

    int EncodeFrame(const void * pInputData, int nInputBytes);

    CBitArray * GetBitArray();
    intn GetPeakLevel();

private:
    int Prepare(const void * pInputData, int nInputBytes, int * pSpecialCodes);

    CSmartPtr<CBitArray> m_spBitArray;
    IPredictorCompress * m_aryPredictors[APE_MAXIMUM_CHANNELS];
    BIT_ARRAY_STATE m_aryBitArrayStates[APE_MAXIMUM_CHANNELS];
    CSmartPtr<int> m_spData;
    CSmartPtr<int> m_spTempData;
    CSmartPtr<CPrepare> m_spPrepare;
    int m_nPeakLevel;
    int m_nMaxFrameBlocks;
    WAVEFORMATEX m_wfeInput;
};

#pragma pack(pop)

}

#endif
