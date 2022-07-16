#pragma once

namespace APE
{

class CAPEDecompressCore
{
public:
    CAPEDecompressCore(IAPEDecompress * pAPEDecompress);
    ~CAPEDecompressCore();
    
    void GenerateDecodedArrays(intn nBlocks, intn nSpecialCodes, intn nFrameIndex);
    void GenerateDecodedArray(int * Input_Array, uint32 Number_of_Elements, intn Frame_Index, CAntiPredictor * pAntiPredictor);
    
    int * GetDataX() { return m_spDataX; }
    int * GetDataY() { return m_spDataY; }
    
    CUnBitArrayBase * GetUnBitArrray() { return m_spUnBitArray; }
    
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

}
