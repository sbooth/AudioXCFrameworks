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
    // input source creation
    static CInputSource * CreateInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int32 * pFlags, int * pErrorCode = APE_NULL);

    // construction / destruction
    virtual ~CInputSource() { }

    // get data
    virtual int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) = 0;

    // get header / terminating data
    virtual int GetHeaderData(unsigned char * pBuffer) = 0;
    virtual int GetTerminatingData(unsigned char * pBuffer) = 0;

    // get other properties
    virtual bool GetUnknownLengthFile() { return false; }
    virtual bool GetFloat() { return false; }

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
    // test header
    static bool GetHeaderMatches(BYTE aryHeader[64]);

    // construction / destruction
    CWAVInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CWAVInputSource() APE_OVERRIDE;

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // get other properties
    bool GetUnknownLengthFile() APE_OVERRIDE { return m_bUnknownLengthFile; }
    bool GetFloat() APE_OVERRIDE { return m_bFloat; }

private:
    int AnalyzeSource();

    CSmartPtr<CIO> m_spIO;
    uint32 m_nHeaderBytes;
    uint32 m_nTerminatingBytes;
    int64 m_nDataBytes;
    int64 m_nFileBytes;
    WAVEFORMATEX m_wfeSource;
    bool m_bIsValid;
    bool m_bUnknownLengthFile;
    bool m_bFloat;
};

/**************************************************************************************************
CAIFFInputSource - wraps working with AIFF files
**************************************************************************************************/
class CAIFFInputSource : public CInputSource
{
public:
    // test header
    static bool GetHeaderMatches(BYTE aryHeader[64]);

    // construction / destruction
    CAIFFInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CAIFFInputSource() APE_OVERRIDE;

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // endian
    bool GetIsBigEndian() const;

    // get other properties
    bool GetFloat() APE_OVERRIDE { return m_bFloat; }

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
    bool m_bFloat;
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
    // test header
    static bool GetHeaderMatches(BYTE aryHeader[64]);

    // construction / destruction
    CW64InputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CW64InputSource() APE_OVERRIDE;

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // get other properties
    bool GetFloat() APE_OVERRIDE { return m_bFloat; }

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
    bool m_bFloat;
};

/**************************************************************************************************
CSNDInputSource - wraps working with SND files
**************************************************************************************************/
class CSNDInputSource : public CInputSource
{
public:
    // test header
    static bool GetHeaderMatches(BYTE aryHeader[64]);

    // construction / destruction
    CSNDInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL, int32 * pFlags = APE_NULL);
    ~CSNDInputSource() APE_OVERRIDE;

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // get other properties
    bool GetFloat() APE_OVERRIDE { return (m_wfeSource.wFormatTag == WAVE_FORMAT_IEEE_FLOAT); }

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
    // test header
    static bool GetHeaderMatches(BYTE aryHeader[64]);

    // construction / destruction
    CCAFInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode = APE_NULL);
    ~CCAFInputSource() APE_OVERRIDE;

    // get data
    int GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved) APE_OVERRIDE;

    // get header / terminating data
    int GetHeaderData(unsigned char * pBuffer) APE_OVERRIDE;
    int GetTerminatingData(unsigned char * pBuffer) APE_OVERRIDE;

    // get other properties
    bool GetFloat() APE_OVERRIDE { return (m_wfeSource.wFormatTag == WAVE_FORMAT_IEEE_FLOAT); }

    // endian
    bool GetIsBigEndian() const;

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

#pragma pack(pop)

}
