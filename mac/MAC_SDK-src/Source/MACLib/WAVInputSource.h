#pragma once

#include "IO.h"

namespace APE
{

#pragma pack(push, 1)

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

    // get other properties
    virtual bool GetUnknownLengthPipe() { return false; }

protected:
    // get header / terminating data
    int GetHeaderDataHelper(bool bIsValid, unsigned char * pBuffer, uint32 nHeaderBytes, CIO * pIO);
    int GetTerminatingDataHelper(bool bIsValid, unsigned char * pBuffer, uint32 nTerminatingBytes, CIO * pIO);
    void Convert8BitSignedToUnsigned(unsigned char * pBuffer, int nChannels, int nBlocks);
    void FlipEndian(unsigned char * pBuffer, int nBitsPerSample, int nChannels, int nBlocks);
};

/**************************************************************************************************
CWAVInputSource - wraps working with WAV files
**************************************************************************************************/
class CWAVInputSource : public CInputSource
{
public:
    // construction / destruction
    CWAVInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CWAVInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // get other properties
    bool GetUnknownLengthPipe() APE_OVERRIDE { return m_bUnknownLengthPipe; }

private:
    int AnalyzeSource();

    CSmartPtr<CIO> m_spIO;
    uint32 m_nHeaderBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nDataBytes;
    int64 m_nFileBytes;
    CSmartPtr<char> m_spFullHeader;
    WAVEFORMATEX m_wfeSource;
    bool m_bIsValid;
    bool m_bUnknownLengthPipe;
};

/**************************************************************************************************
CAIFFInputSource - wraps working with AIFF files
**************************************************************************************************/
class CAIFFInputSource : public CInputSource
{
public:
    // construction / destruction
    CAIFFInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CAIFFInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // endian
    bool GetIsBigEndian();

private:
    int AnalyzeSource();
    unsigned long FetchLong(unsigned long * ptr);
    double GetExtendedDouble(uint16_t exponent, uint64_t mantissa);

    CSmartPtr<CIO> m_spIO;
    uint32 m_nHeaderBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nDataBytes;
    int64 m_nFileBytes;
    WAVEFORMATEX m_wfeSource;
    bool m_bIsValid;
    bool m_bLittleEndian;
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
    CW64InputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CW64InputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

private:
    int AnalyzeSource();
    int64 Align(int64 nValue, int nAlignment);

    CSmartPtr<CIO> m_spIO;
    uint32 m_nHeaderBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nDataBytes;
    int64 m_nFileBytes;
    WAVEFORMATEX m_wfeSource;
    bool m_bIsValid;
};

/**************************************************************************************************
CSNDInputSource - wraps working with SND files
**************************************************************************************************/
class CSNDInputSource : public CInputSource
{
public:
    // construction / destruction
    CSNDInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL, int32 * pFlags = APE_NULL);
    ~CSNDInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

private:
    int AnalyzeSource(int32 * pFlags);
    uint32 FlipByteOrder32(uint32 nValue);

    CSmartPtr<CIO> m_spIO;
    uint32 m_nHeaderBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nDataBytes;
    int64 m_nFileBytes;
    WAVEFORMATEX m_wfeSource;
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
    CCAFInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CCAFInputSource();

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // endian
    bool GetIsBigEndian();

private:
    int AnalyzeSource();

    CSmartPtr<CIO> m_spIO;
    uint32 m_nHeaderBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nDataBytes;
    int64 m_nFileBytes;
    WAVEFORMATEX m_wfeSource;
    bool m_bLittleEndian;
    bool m_bIsValid;
};

/**************************************************************************************************
Input souce creation
**************************************************************************************************/
CInputSource * CreateInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int32 * pFlags, int * pErrorCode = APE_NULL);

#pragma pack(pop)

}
