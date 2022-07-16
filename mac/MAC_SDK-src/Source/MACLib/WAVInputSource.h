#pragma once

#include "IO.h"

namespace APE
{

/**************************************************************************************************
CInputSource - base input format class (allows multiple format support)
**************************************************************************************************/
class CInputSource
{
public:
    // construction / destruction
    virtual ~CInputSource() { }
    
    // get data
    virtual int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) = 0;
    
    // get header / terminating data
    virtual int GetHeaderData(unsigned char * pBuffer) = 0;
    virtual int GetTerminatingData(unsigned char * pBuffer) = 0;
    virtual bool GetUnknownLengthPipe() { return false; }

protected:
    // get header / terminating data
    int GetHeaderDataHelper(bool bIsValid, unsigned char * pBuffer, uint32 nHeaderBytes, CIO * pIO);
    int GetTerminatingDataHelper(bool bIsValid, unsigned char * pBuffer, uint32 nTerminatingBytes, CIO * pIO);
};

/**************************************************************************************************
CWAVInputSource - wraps working with WAV files
**************************************************************************************************/
class CWAVInputSource : public CInputSource
{
public:
    // construction / destruction
    CWAVInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = NULL);
    ~CWAVInputSource();
    
    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved);
    
    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer);
    int GetTerminatingData(unsigned char * pBuffer);
    bool GetUnknownLengthPipe() { return m_bUnknownLengthPipe; }

private:
    int AnalyzeSource();

    CSmartPtr<CIO> m_spIO;    
    WAVEFORMATEX m_wfeSource;
    uint32 m_nHeaderBytes;
    int64 m_nDataBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nFileBytes;
    bool m_bIsValid;
    CSmartPtr<char> m_spFullHeader;
    bool m_bUnknownLengthPipe;
};

/**************************************************************************************************
CAIFFInputSource - wraps working with AIFF files
**************************************************************************************************/
class CAIFFInputSource : public CInputSource
{
public:
    // construction / destruction
    CAIFFInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = NULL);
    ~CAIFFInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved);

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer);
    int GetTerminatingData(unsigned char * pBuffer);

private:
    int AnalyzeSource();
    unsigned long FetchLong(unsigned long * ptr);
    uint32 IEEE754ExtendedFloatToUINT32(unsigned char* buffer);

    CSmartPtr<CIO> m_spIO;
    WAVEFORMATEX m_wfeSource;
    uint32 m_nHeaderBytes;
    int64 m_nDataBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nFileBytes;
    bool m_bIsValid;
};

/**************************************************************************************************
CW64InputSource - wraps working with W64 files
**************************************************************************************************/
struct W64ChunkHeader
{
    GUID guidIdentifier; // the identifier of the chunk
    uint64 nBytes; // the size of the chunk
};

struct WAVFormatChunkData
{
    uint16            nFormatTag;                // the format of the WAV...should equal 1 for a PCM file
    uint16            nChannels;                // the number of channels
    uint32            nSamplesPerSecond;        // the number of samples per second
    uint32            nAverageBytesPerSecond; // the bytes per second
    uint16            nBlockAlign;            // block alignment
    uint16            nBitsPerSample;            // the number of bits per sample
};

class CW64InputSource : public CInputSource
{
public:
    // construction / destruction
    CW64InputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = NULL);
    ~CW64InputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved);

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer);
    int GetTerminatingData(unsigned char * pBuffer);

private:
    int AnalyzeSource();
    
    static inline int64 Align(int64 nValue, int nAlignment)
    {
        ASSERT(nAlignment > 0 && ((nAlignment & (nAlignment - 1)) == 0));
        return (nValue + nAlignment - 1) & ~((int64)(nAlignment - 1));
    }

    CSmartPtr<CIO> m_spIO;
    WAVEFORMATEX m_wfeSource;
    uint32 m_nHeaderBytes;
    int64 m_nDataBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nFileBytes;
    bool m_bIsValid;
};

/**************************************************************************************************
CSNDInputSource - wraps working with SND files
**************************************************************************************************/
class CSNDInputSource : public CInputSource
{
public:
    // construction / destruction
    CSNDInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = NULL, int32 * pFlags = NULL);
    ~CSNDInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved);

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer);
    int GetTerminatingData(unsigned char * pBuffer);

private:
    int AnalyzeSource(int32 * pFlags);

    static inline int32 FlipByteOrder32(int32 nValue)
    {
        return (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
    }
    static inline uint32 FlipByteOrder32(uint32 nValue)
    {
        return (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
    }
    
    CSmartPtr<CIO> m_spIO;
    WAVEFORMATEX m_wfeSource;
    uint32 m_nHeaderBytes;
    int64 m_nDataBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nFileBytes;
    bool m_bIsValid;
    bool m_bBigEndian;
};

/**************************************************************************************************
CCAFInputSource - wraps working with CAF files
**************************************************************************************************/
class CCAFInputSource : public CInputSource
{
public:
    // construction / destruction
    CCAFInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = NULL);
    ~CCAFInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved);

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer);
    int GetTerminatingData(unsigned char * pBuffer);

private:
    int AnalyzeSource();

    CSmartPtr<CIO> m_spIO;
    WAVEFORMATEX m_wfeSource;
    uint32 m_nHeaderBytes;
    int64 m_nDataBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nFileBytes;
    bool m_bIsValid;
};

/**************************************************************************************************
Input souce creation
**************************************************************************************************/
CInputSource * CreateInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int32 * pFlags, int * pErrorCode = NULL);

}
