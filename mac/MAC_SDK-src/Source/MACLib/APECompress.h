#pragma once

#ifdef APE_SUPPORT_COMPRESS

#include "MACLib.h"

namespace APE
{
class CAPECompressCreate;

#pragma pack(push, 1)

/**************************************************************************************************
CAPECompress - uses the CAPECompressHub to provide a simpler compression interface (with buffering, etc)
**************************************************************************************************/
class CAPECompress : public IAPECompress
{
public:
    CAPECompress();
    ~CAPECompress();

    // start encoding
    int Start(const wchar_t * pOutputFilename, const WAVEFORMATEX * pwfeInput, int64 nMaxAudioBytes, int nCompressionLevel = APE_COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = APE_NULL, int64 nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION, int nFlags = 0) APE_OVERRIDE;
    int StartEx(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int64 nMaxAudioBytes, int nCompressionLevel = APE_COMPRESSION_LEVEL_NORMAL, const void * pHeaderData = APE_NULL, int64 nHeaderBytes = CREATE_WAV_HEADER_ON_DECOMPRESSION) APE_OVERRIDE;

    // add data / compress data

    // allows linear, immediate access to the buffer (fast)
    int64 GetBufferBytesAvailable() APE_OVERRIDE;
    int64 UnlockBuffer(int64 nBytesAdded, bool bProcess = true) APE_OVERRIDE;
    unsigned char * LockBuffer(int64 * pBytesAvailable) APE_OVERRIDE;

    // slower, but easier than locking and unlocking (copies data)
    int64 AddData(unsigned char * pData, int64 nBytes) APE_OVERRIDE;

    // use a CIO (input source) to add data
    int64 AddDataFromInputSource(CInputSource * pInputSource, int64 nMaxBytes = 0, int64 * pBytesAdded = APE_NULL) APE_OVERRIDE;

    // finish / kill
    int Finish(unsigned char * pTerminatingData, int64 nTerminatingBytes, int64 nWAVTerminatingBytes) APE_OVERRIDE;
    int Kill() APE_OVERRIDE;

private:
    int ProcessBuffer(bool bFinalize = false);

    CSmartPtr<CAPECompressCreate> m_spAPECompressCreate;
    int64 m_nBufferHead;
    int64 m_nBufferTail;
    int64 m_nBufferSize;
    CSmartPtr<unsigned char> m_spBuffer;
    CIO * m_pioOutput;
    bool m_bOwnsOutputIO;
    bool m_bBufferLocked;
    WAVEFORMATEX m_wfeInput;
};

#pragma pack(pop)

}

#endif
