#pragma once

namespace APE
{

#pragma pack(push, 1)

class CAPEDecompressCoreOld
{
public:
    CAPEDecompressCoreOld(IAPEDecompress * pAPEDecompress);
    ~CAPEDecompressCoreOld();

    void GenerateDecodedArrays(intn nBlocks, intn nSpecialCodes, intn nFrameIndex);
    void GenerateDecodedArray(int * pInputArray, int nNumberElements, intn nFrameIndex, CAntiPredictor * pAntiPredictor);

    int * GetDataX();
    int * GetDataY();

#ifdef APE_DECOMPRESS_CORE_GET_UNBITARRAY
    __forceinline CUnBitArrayBase * GetUnBitArrray() { return m_spUnBitArray; }
#endif

    CSmartPtr<int> m_spTempData;
    CSmartPtr<int> m_spDataX;
    CSmartPtr<int> m_spDataY;

    CSmartPtr<CAntiPredictor> m_spAntiPredictorX;
    CSmartPtr<CAntiPredictor> m_spAntiPredictorY;

    CSmartPtr<CUnBitArrayBase> m_spUnBitArray;
    BIT_ARRAY_STATE m_BitArrayStateX;
    BIT_ARRAY_STATE m_BitArrayStateY;

    IAPEDecompress * m_pAPEDecompress;

    int m_nBlocksProcessed;
};

#pragma pack(pop)

}
