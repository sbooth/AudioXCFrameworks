#pragma once

#ifdef APE_SUPPORT_COMPRESS

#include "APECompress.h"

namespace APE
{
class CAPECompressCore;

#pragma pack(push, 1)

class CAPECompressCreate
{
public:
    CAPECompressCreate();
    virtual ~CAPECompressCreate();

    int InitializeFile(CIO * pIO, const WAVEFORMATEX * pwfeInput, intn nMaxFrames, intn nCompressionLevel, const void * pHeaderData, int64 nHeaderBytes, int32 nFlags);
    int FinalizeFile(CIO * pIO, int nNumberOfFrames, int nFinalFrameBlocks, const void * pTerminatingData, int64 nTerminatingBytes, int64 nWAVTerminatingBytes);

    int SetSeekByte(int nFrame, int64 nByteOffset);

    int Start(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int64 nMaxAudioBytes, int nCompressionLevel = APE_COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = APE_NULL, int64 nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION, int32 nFlags = 0);

    intn GetFullFrameBytes();
    int EncodeFrame(const void * pInputData, int nInputBytes);

    int Finish(const void * pTerminatingData, int64 nTerminatingBytes, int64 nWAVTerminatingBytes);

    bool GetTooMuchData();

private:
    CSmartPtr<uint32> m_spSeekTable;
    intn m_nMaxFrames;

    CSmartPtr<CIO> m_spIO;
    CSmartPtr<CAPECompressCore> m_spAPECompressCore;

    int m_nCompressionLevel;
    int m_nBlocksPerFrame;
    int m_nFrameIndex;
    int m_nLastFrameBlocks;
    WAVEFORMATEX m_wfeInput;
    bool m_bTooMuchData;
};

#pragma pack(pop)

}

#endif
