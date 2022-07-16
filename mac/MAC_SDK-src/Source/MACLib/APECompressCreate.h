#pragma once

#ifdef APE_SUPPORT_COMPRESS

#include "APECompress.h"

namespace APE
{
class CAPECompressCore;

class CAPECompressCreate
{
public:
    CAPECompressCreate();
    virtual ~CAPECompressCreate();
    
    int InitializeFile(CIO * pIO, const WAVEFORMATEX * pwfeInput, intn nMaxFrames, intn nCompressionLevel, const void * pHeaderData, int64 nHeaderBytes, int32 nFlags);
    int FinalizeFile(CIO * pIO, int nNumberOfFrames, int nFinalFrameBlocks, const void * pTerminatingData, int64 nTerminatingBytes, int64 nWAVTerminatingBytes);
    
    int SetSeekByte(int nFrame, int64 nByteOffset);

    int Start(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int64 nMaxAudioBytes, int nCompressionLevel = MAC_COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = NULL, int64 nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION, int32 nFlags = 0);
        
    intn GetFullFrameBytes();
    int EncodeFrame(const void * pInputData, int nInputBytes);

    int Finish(const void * pTerminatingData, int nTerminatingBytes, int nWAVTerminatingBytes);

    bool GetTooMuchData() { return m_bTooMuchData; }
    
private:    
    CSmartPtr<uint32> m_spSeekTable;
    intn m_nMaxFrames;

    CSmartPtr<CIO> m_spIO;
    CSmartPtr<CAPECompressCore> m_spAPECompressCore;
    
    WAVEFORMATEX m_wfeInput;
    int m_nCompressionLevel;
    int m_nBlocksPerFrame;
    int m_nFrameIndex;
    int m_nLastFrameBlocks;
    bool m_bTooMuchData;
};

}

#endif