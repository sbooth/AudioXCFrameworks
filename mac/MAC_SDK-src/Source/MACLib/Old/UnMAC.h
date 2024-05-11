#pragma once

#include "BitArray.h"
#include "UnBitArrayBase.h"

namespace APE
{

class CAntiPredictor;
class CPrepare;
class CAPEDecompressCore;
class CPredictorBase;
class IPredictorDecompress;
class IAPEDecompress;

/**************************************************************************************************
CUnMAC class... a class that allows decoding on a frame-by-frame basis
**************************************************************************************************/

#pragma pack(push, 1)

class CUnMAC
{
public:
    // construction / destruction
    CUnMAC();
    ~CUnMAC();

    // functions
    int Initialize(IAPEDecompress * pAPEDecompress);
    int Uninitialize();
    intn DecompressFrame(unsigned char * pOutputData, int32 nFrameIndex, int * pErrorCode);

    int SeekToFrame(intn FrameIndex);

private:
    // data members
    CSmartPtr<IAPEDecompress> m_spAPEDecompress;
    CSmartPtr<CPrepare> m_spPrepare;
    CSmartPtr<CAPEDecompressCore> m_spAPEDecompressCore;
    int m_LastDecodedFrameIndex;

    // functions
    void GenerateDecodedArrays(intn nBlocks, intn nSpecialCodes, intn nFrameIndex);
    void GenerateDecodedArray(int * pInput_Array, uint32 Number_of_Elements, intn Frame_Index, CAntiPredictor * pAntiPredictor);

    int CreateAntiPredictors(int nCompressionLevel, int nVersion);

    intn DecompressFrameOld(unsigned char * pOutputData, int32 FrameIndex, int * pErrorCode);
    uint32 CalculateOldChecksum(const int * pDataX, const int * pDataY, intn nChannels, intn nBlocks);

public:
    int m_nBlocksProcessed;
    unsigned int m_nCRC;
    unsigned int m_nStoredCRC;
    WAVEFORMATEX m_wfeInput;

private:
    bool m_bInitialized;
};

#pragma pack(pop)

}
