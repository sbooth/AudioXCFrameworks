#pragma once

#include "UnBitArrayBase.h"
#include "MACLib.h"
#include "Prepare.h"
#include "CircleBuffer.h"

namespace APE
{

class CUnBitArray;
class CPrepare;
class CAPEInfo;
class IPredictorDecompress;

#pragma pack(push, 1)

class CAPEDecompress : public IAPEDecompress
{
public:
    CAPEDecompress(int * pErrorCode, CAPEInfo * pAPEInfo, int64 nStartBlock = -1, int64 nFinishBlock = -1);
    ~CAPEDecompress();

    int GetData(unsigned char * pBuffer, int64 nBlocks, int64 * pBlocksRetrieved, APE_GET_DATA_PROCESSING * pProcessing = APE_NULL) APE_OVERRIDE;
    int Seek(int64 nBlockOffset) APE_OVERRIDE;

    int64 GetInfo(IAPEDecompress::APE_DECOMPRESS_FIELDS Field, int64 nParam1 = 0, int64 nParam2 = 0) APE_OVERRIDE;

protected:
    // file info
    int m_nBlockAlign;
    int64 m_nCurrentFrame;

    // start / finish information
    int64 m_nStartBlock;
    int64 m_nFinishBlock;
    int64 m_nCurrentBlock;
    bool m_bIsRanged;
    bool m_bDecompressorInitialized;

    // decoding tools
    unsigned int m_nCRC;
    unsigned int m_nStoredCRC;
    int m_nSpecialCodes;
    CSmartPtr<int> m_sparyChannelData;
    CPrepare m_Prepare;
    WAVEFORMATEX m_wfeInput;

    int SeekToFrame(int64 nFrameIndex);
    void DecodeBlocksToFrameBuffer(int64 nBlocks);
    int FillFrameBuffer();
    void StartFrame();
    void EndFrame();
    int InitializeDecompressor();

    // more decoding components
    CSmartPtr<CAPEInfo> m_spAPEInfo;
    CSmartPtr<CUnBitArrayBase> m_spUnBitArray;
    UNBIT_ARRAY_STATE m_aryBitArrayStates[APE_MAXIMUM_CHANNELS];
    IPredictorDecompress * m_aryPredictor[APE_MAXIMUM_CHANNELS];
    int m_nLastX;

    // decoding buffer
    int64 m_nErrorDecodingCurrentFrameOutputSilenceBlocks;
    int64 m_nCurrentFrameBufferBlock;
    int64 m_nFrameBufferFinishedBlocks;
    CCircleBuffer m_cbFrameBuffer;
    bool m_bErrorDecodingCurrentFrame;
    bool m_bErrorDecodingLastFrame;
    bool m_bInterimMode;
};

#pragma pack(pop)

}
