#include <stdint.h>
#include "All.h"
#include "WAVInputSource.h"
#include "IO.h"
#include "BufferIO.h"
#include "MACLib.h"
#include "GlobalFunctions.h"
#include "FloatTransform.h"

namespace APE
{

#define Swap2Bytes(val) \
((((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00))

#define Swap4Bytes(val) \
((((val) >> 24) & 0x000000FF) | (((val) >> 8) & 0x0000FF00) | \
(((val) << 8) & 0x00FF0000) | (((val) << 24) & 0xFF000000))

#define Swap8Bytes(val) \
((((val) >> 56) & 0x00000000000000FFULL) | (((val) >> 40) & 0x000000000000FF00ULL) | \
(((val) >> 24) & 0x0000000000FF0000ULL) | (((val) >> 8) & 0x00000000FF000000ULL) | \
(((val) << 8) & 0x000000FF00000000ULL) | (((val) << 24) & 0x0000FF0000000000ULL) | \
(((val) << 40) & 0x00FF000000000000ULL) | (((val) << 56) & 0xFF00000000000000ULL))

struct RIFF_HEADER
{
    char cRIFF[4];            // the characters 'RIFF' indicating that it's a RIFF file
    uint32_t nBytes;          // the number of bytes following this header
};

struct DATA_TYPE_ID_HEADER
{
    char cDataTypeID[4];      // should equal 'WAVE' for a WAV file
};

struct WAV_FORMAT_HEADER
{
    uint16 nFormatTag;            // the format of the WAV...should equal 1 for a PCM file
    uint16 nChannels;             // the number of channels
    uint32_t nSamplesPerSecond;   // the number of samples per second
    uint32_t nBytesPerSecond;     // the bytes per second
    uint16 nBlockAlign;           // block alignment
    uint16 nBitsPerSample;        // the number of bits per sample
};

struct RIFF_CHUNK_HEADER
{
    char cChunkLabel[4];        // should equal "data" indicating the data chunk
    uint32_t nChunkBytes;       // the bytes of the chunk
};

/**************************************************************************************************
Input source creation
**************************************************************************************************/
CInputSource * CInputSource::CreateInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int32 * pFlags, int * pErrorCode)
{
    // error check the parameters
    if ((pSourceName == APE_NULL) || (wcslen(pSourceName) == 0))
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return APE_NULL;
    }

    // get the extension
    const wchar_t * pExtension = &pSourceName[wcslen(pSourceName)];
    while ((pExtension > pSourceName) && (*pExtension != '.'))
        pExtension--;

    // open the file
    CSmartPtr<CIO> spIO(CreateCIO());
    if (spIO->Open(pSourceName, true) != ERROR_SUCCESS)
    {
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return APE_NULL;
    }

    // read header
    BYTE aryHeader[64];
    CSmartPtr<CHeaderIO> spHeaderIO(new CHeaderIO(spIO));
    if (spHeaderIO->ReadHeader(aryHeader) == false)
    {
        *pErrorCode = ERROR_IO_READ;
        return APE_NULL;
    }

    // set as reader
    spHeaderIO.SetDelete(false);
    spIO.SetDelete(false);
    spIO.Assign(spHeaderIO);
    spIO.SetDelete(true); // this is redundant because Assign sets it, but it's here for clarity

    // read header
    if (CWAVInputSource::GetHeaderMatches(aryHeader))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        CInputSource * pWAV = new CWAVInputSource(spIO, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
        spIO.SetDelete(false);
        if (pWAV->GetFloat())
            *pFlags |= APE_FORMAT_FLAG_FLOATING_POINT;
        return pWAV;
    }
    else if (CAIFFInputSource::GetHeaderMatches(aryHeader))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        *pFlags |= APE_FORMAT_FLAG_AIFF;
        CAIFFInputSource * pAIFF = new CAIFFInputSource(spIO, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
        spIO.SetDelete(false);
        if (pAIFF->GetIsBigEndian())
            *pFlags |= APE_FORMAT_FLAG_BIG_ENDIAN;
        if (pwfeSource->wBitsPerSample == 8)
            *pFlags |= APE_FORMAT_FLAG_SIGNED_8_BIT;
        if (pwfeSource->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            *pFlags |= APE_FORMAT_FLAG_FLOATING_POINT;
        return pAIFF;
    }
    else if (CW64InputSource::GetHeaderMatches(aryHeader))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        *pFlags |= APE_FORMAT_FLAG_W64;
        CW64InputSource * pW64 = new CW64InputSource(spIO, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
        spIO.SetDelete(false);
        if (pwfeSource->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            *pFlags |= APE_FORMAT_FLAG_FLOATING_POINT;
        return pW64;
    }
    else if (CSNDInputSource::GetHeaderMatches(aryHeader))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        CSNDInputSource * pSND = new CSNDInputSource(spIO, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode, pFlags);
        spIO.SetDelete(false);
        if (pwfeSource->wBitsPerSample == 8)
            *pFlags |= APE_FORMAT_FLAG_SIGNED_8_BIT;
        if (pwfeSource->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            *pFlags |= APE_FORMAT_FLAG_FLOATING_POINT;
        return pSND;
    }
    else if (CCAFInputSource::GetHeaderMatches(aryHeader))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        CCAFInputSource * pCAF = new CCAFInputSource(spIO, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
        spIO.SetDelete(false);
        *pFlags |= APE_FORMAT_FLAG_CAF;
        if (pCAF->GetIsBigEndian())
            *pFlags |= APE_FORMAT_FLAG_BIG_ENDIAN;
        if (pwfeSource->wBitsPerSample == 8)
            *pFlags |= APE_FORMAT_FLAG_SIGNED_8_BIT;
        if (pwfeSource->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            *pFlags |= APE_FORMAT_FLAG_FLOATING_POINT;
        return pCAF;
    }
    else
    {
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return APE_NULL;
    }
}

/**************************************************************************************************
CInputSource - base input format class (allows multiple format support)
**************************************************************************************************/
int CInputSource::GetHeaderDataHelper(bool bIsValid, unsigned char * pBuffer, uint32_t nHeaderBytes, CIO * pIO)
{
    if (!bIsValid) return ERROR_UNDEFINED;

    int nResult = ERROR_SUCCESS;

    if (nHeaderBytes > 0)
    {
        const int64 nOriginalFileLocation = pIO->GetPosition();

        if (nOriginalFileLocation != 0)
        {
            pIO->Seek(0, SeekFileBegin);
        }

        unsigned int nBytesRead = 0;
        const int nReadRetVal = pIO->Read(pBuffer, nHeaderBytes, &nBytesRead);

        if ((nReadRetVal != ERROR_SUCCESS) || (nHeaderBytes != nBytesRead))
        {
            nResult = ERROR_UNDEFINED;
        }

        pIO->Seek(nOriginalFileLocation, SeekFileBegin);
    }

    return nResult;
}

int CInputSource::GetTerminatingDataHelper(bool bIsValid, unsigned char * pBuffer, uint32_t nTerminatingBytes, CIO * pIO)
{
    if (!bIsValid) return ERROR_UNDEFINED;

    int nResult = ERROR_SUCCESS;

    if (nTerminatingBytes > 0)
    {
        const int64 nOriginalFileLocation = pIO->GetPosition();

        pIO->Seek(-static_cast<int64>(nTerminatingBytes), SeekFileEnd);

        unsigned int nBytesRead = 0;
        const int nReadRetVal = pIO->Read(pBuffer, nTerminatingBytes, &nBytesRead);

        if ((nReadRetVal != ERROR_SUCCESS) || (nTerminatingBytes != nBytesRead))
        {
            nResult = ERROR_UNDEFINED;
        }

        pIO->Seek(nOriginalFileLocation, SeekFileBegin);
    }

    return nResult;
}

void CInputSource::Convert8BitSignedToUnsigned(unsigned char * pBuffer, int nChannels, int nBlocks)
{
    for (int nSample = 0; nSample < nBlocks * nChannels; nSample++)
    {
        const char cTemp = static_cast<char>(pBuffer[nSample]);
        const unsigned char cConvert = static_cast<unsigned char>(static_cast<int>(cTemp) + 128);
        pBuffer[nSample] = cConvert;
    }
}

void CInputSource::FlipEndian(unsigned char * pBuffer, int nBitsPerSample, int nChannels, int nBlocks)
{
    if (nBitsPerSample == 16)
    {
        for (int nSample = 0; nSample < nBlocks * nChannels; nSample++)
        {
            const unsigned char cTemp = pBuffer[(nSample * 2) + 0];
            pBuffer[(nSample * 2) + 0] = pBuffer[(nSample * 2) + 1];
            pBuffer[(nSample * 2) + 1] = cTemp;
        }
    }
    else if (nBitsPerSample == 24)
    {
        for (int nSample = 0; nSample < nBlocks * nChannels; nSample++)
        {
            const unsigned char cTemp = pBuffer[(nSample * 3) + 0];
            pBuffer[(nSample * 3) + 0] = pBuffer[(nSample * 3) + 2];
            pBuffer[(nSample * 3) + 2] = cTemp;
        }
    }
    else if (nBitsPerSample == 32)
    {
        uint32_t * pBufferUINT32 = reinterpret_cast<uint32_t *>(pBuffer);
        for (int nSample = 0; nSample < nBlocks * nChannels; nSample++)
        {
            const uint32_t nValue = pBufferUINT32[nSample];
            const uint32_t nFlippedValue = (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
            pBufferUINT32[nSample] = nFlippedValue;
        }
    }
}

/**************************************************************************************************
CWAVInputSource - wraps working with WAV files
**************************************************************************************************/
/*static*/ bool CWAVInputSource::GetHeaderMatches(BYTE aryHeader[64])
{
    if (!(aryHeader[0] == 'R' && aryHeader[1] == 'I' && aryHeader[2] == 'F' && aryHeader[3] == 'F') &&
        !(aryHeader[0] == 'R' && aryHeader[1] == 'F' && aryHeader[2] == '6' && aryHeader[3] == '4') &&
        !(aryHeader[0] == 'B' && aryHeader[1] == 'W' && aryHeader[2] == '6' && aryHeader[3] == '4'))
    {
        return false;
    }

    return true;
}

CWAVInputSource::CWAVInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nTerminatingBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_bFloat = false; // we need a boolean instead of just checking WAVE_FORMAT_IEEE_FLOAT since it can be extensible with the format float
    APE_CLEAR(m_wfeSource);

    m_bUnknownLengthFile = false;

    if (pIO == APE_NULL || pwfeSource == APE_NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    // store the reader
    m_spIO.Assign(pIO);

    // read to a buffer so pipes work (that way we don't have to seek back to get the header)
    m_spIO.SetDelete(false);
    m_spIO.Assign(new CBufferIO(m_spIO, APE_BYTES_IN_KILOBYTE * 256));
    m_spIO.SetDelete(true);

    // analyze source
    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / static_cast<int64>(m_wfeSource.nBlockAlign);
        if (pHeaderBytes) *pHeaderBytes = m_nHeaderBytes;
        if (pTerminatingBytes) *pTerminatingBytes = m_nTerminatingBytes;

        m_bIsValid = true;
    }

    if (pErrorCode) *pErrorCode = nResult;
}

CWAVInputSource::~CWAVInputSource()
{
}

int CWAVInputSource::AnalyzeSource()
{
    // get the file size
    m_nFileBytes = m_spIO->GetSize();

    // get the RIFF header
    RIFF_HEADER RIFFHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFHeader, sizeof(RIFFHeader)))

    // make sure the RIFF header is valid
    if (!(RIFFHeader.cRIFF[0] == 'R' && RIFFHeader.cRIFF[1] == 'I' && RIFFHeader.cRIFF[2] == 'F' && RIFFHeader.cRIFF[3] == 'F') &&
        !(RIFFHeader.cRIFF[0] == 'R' && RIFFHeader.cRIFF[1] == 'F' && RIFFHeader.cRIFF[2] == '6' && RIFFHeader.cRIFF[3] == '4') &&
        !(RIFFHeader.cRIFF[0] == 'B' && RIFFHeader.cRIFF[1] == 'W' && RIFFHeader.cRIFF[2] == '6' && RIFFHeader.cRIFF[3] == '4'))
    {
        return ERROR_INVALID_INPUT_FILE;
    }

    // sanity check (a file gotten from David Bryant was tripping this for example)
    if (m_nFileBytes == APE_FILE_SIZE_UNDEFINED)
        RIFFHeader.nBytes = static_cast<uint32_t>(-1);
    else if (RIFFHeader.nBytes > m_nFileBytes)
        RIFFHeader.nBytes = static_cast<uint32_t>(-1);

    // get the file size from RIFF header in case we're a pipe and use the maximum
    if (RIFFHeader.nBytes != static_cast<uint32_t>(-1))
    {
        // use maximum size between header and file size
        const int64 nHeaderBytes = static_cast<int64>(RIFFHeader.nBytes) + static_cast<int64>(sizeof(RIFF_HEADER));
        m_nFileBytes = ape_max(m_nFileBytes, nHeaderBytes);
    }
    else if (m_nFileBytes == APE_FILE_SIZE_UNDEFINED)
    {
        // mark that we need to read the pipe with unknown length
        m_bUnknownLengthFile = true;
        m_nFileBytes = -1;
    }

    // read the data type header
    DATA_TYPE_ID_HEADER DataTypeIDHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &DataTypeIDHeader, sizeof(DataTypeIDHeader)))

    // make sure it's the right data type
    if (!(DataTypeIDHeader.cDataTypeID[0] == 'W' && DataTypeIDHeader.cDataTypeID[1] == 'A' && DataTypeIDHeader.cDataTypeID[2] == 'V' && DataTypeIDHeader.cDataTypeID[3] == 'E'))
        return ERROR_INVALID_INPUT_FILE;

    // find the 'fmt ' chunk
    RIFF_CHUNK_HEADER RIFFChunkHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader)))

    while (!(RIFFChunkHeader.cChunkLabel[0] == 'f' && RIFFChunkHeader.cChunkLabel[1] == 'm' && RIFFChunkHeader.cChunkLabel[2] == 't' && RIFFChunkHeader.cChunkLabel[3] == ' '))
    {
        // check if the header stretches past the end of the file (then we're not valid)
        if (m_nFileBytes != APE_FILE_SIZE_UNDEFINED)
        {
            if (RIFFChunkHeader.nChunkBytes > (m_spIO->GetSize() - m_spIO->GetPosition()))
            {
                return ERROR_INVALID_INPUT_FILE;
            }
        }

        // move the file pointer to the end of this chunk
        CSmartPtr<unsigned char> spExtraChunk(new unsigned char [RIFFChunkHeader.nChunkBytes], true);
        RETURN_ON_ERROR(ReadSafe(m_spIO, spExtraChunk, static_cast<int>(RIFFChunkHeader.nChunkBytes)))

        // check again for the data chunk
        RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader)))
    }

    // read the format info
    WAV_FORMAT_HEADER WAVFormatHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &WAVFormatHeader, sizeof(WAVFormatHeader)))

    // error check the header to see if we support it
    if ((WAVFormatHeader.nFormatTag != WAVE_FORMAT_PCM) && (WAVFormatHeader.nFormatTag != WAVE_FORMAT_EXTENSIBLE) && (WAVFormatHeader.nFormatTag != WAVE_FORMAT_IEEE_FLOAT))
        return ERROR_INVALID_INPUT_FILE;

    #ifndef APE_SUPPORT_FLOAT_COMPRESSION
        if (WAVFormatHeader.nFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            return ERROR_INVALID_INPUT_FILE;
    #endif

    // if the format is an odd bits per sample, just update to a known number -- decoding stores the header so will still be correct (and the block align is that size anyway)
    const int nSampleBits = 8 * WAVFormatHeader.nBlockAlign / ape_max(1, WAVFormatHeader.nChannels);
    if (nSampleBits > 0)
        WAVFormatHeader.nBitsPerSample = static_cast<uint16>(((WAVFormatHeader.nBitsPerSample + (nSampleBits - 1)) / nSampleBits) * nSampleBits);

    // copy the format information to the WAVEFORMATEX passed in
    FillWaveFormatEx(&m_wfeSource, static_cast<int>(WAVFormatHeader.nFormatTag), static_cast<int>(WAVFormatHeader.nSamplesPerSecond), static_cast<int>(WAVFormatHeader.nBitsPerSample), static_cast<int>(WAVFormatHeader.nChannels));

    // see if we're float
    if (WAVFormatHeader.nFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        m_bFloat = true;

    // skip over any extra data in the header
    if (RIFFChunkHeader.nChunkBytes != static_cast<uint32_t>(-1))
    {
        const int64 nWAVFormatHeaderExtra = static_cast<int64>(RIFFChunkHeader.nChunkBytes) - static_cast<int64>(sizeof(WAVFormatHeader));
        if (nWAVFormatHeaderExtra < 0)
        {
            return ERROR_INVALID_INPUT_FILE;
        }
        else if ((nWAVFormatHeaderExtra > 0) && (nWAVFormatHeaderExtra < APE_BYTES_IN_MEGABYTE))
        {
            // read the extra
            CSmartPtr<unsigned char> spWAVFormatHeaderExtra(new unsigned char [static_cast<size_t>(nWAVFormatHeaderExtra)], true);
            RETURN_ON_ERROR(ReadSafe(m_spIO, spWAVFormatHeaderExtra, static_cast<int>(nWAVFormatHeaderExtra)))

            // the extra specifies the format and it might not be PCM, so check
            #pragma pack(push, 1)
            struct CWAVFormatExtra
            {
                uint16 cbSize;
                uint16 nValidBitsPerSample;
                uint32_t nChannelMask;
                BYTE guidSubFormat[16];
            };
            #pragma pack(pop)

            if (nWAVFormatHeaderExtra >= static_cast<APE::int64>(sizeof(CWAVFormatExtra)))
            {
                const CWAVFormatExtra * pExtra = reinterpret_cast<CWAVFormatExtra *>(spWAVFormatHeaderExtra.GetPtr());

                const BYTE guidPCM[16] = { 1, 0, 0, 0, 0, 0, 16, 0, 128, 0, 0, 170, 0, 56, 155, 113 }; // KSDATAFORMAT_SUBTYPE_PCM but that isn't cross-platform
                const BYTE guidFloat[16] = { 3, 0, 0, 0, 0, 0, 16, 0, 128, 0, 0, 170, 0, 56, 155, 113 }; // KSDATAFORMAT_SUBTYPE_IEEE_FLOAT but that isn't cross-platform
                const BYTE guidZero[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                if ((memcmp(&pExtra->guidSubFormat, &guidPCM, 16) != 0) &&
                    (memcmp(&pExtra->guidSubFormat, &guidFloat, 16) != 0) &&
                    (memcmp(&pExtra->guidSubFormat, &guidZero, 16) != 0))
                {
                    // we're not PCM or float, so error
                    return ERROR_INVALID_INPUT_FILE;
                }

                // switch the format to float if we're the float subtype
                if (memcmp(&pExtra->guidSubFormat, &guidFloat, 16) == 0)
                {
                    m_bFloat = true;
                }
            }
        }
    }

    // if we're float, force the size to 32-bit (we found some files where this wasn't true)
    if (m_bFloat)
        FillWaveFormatEx(&m_wfeSource, static_cast<int>(m_wfeSource.wFormatTag), static_cast<int>(m_wfeSource.nSamplesPerSec), static_cast<int>(32), static_cast<int>(m_wfeSource.nChannels));

    // find the data chunk
    RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader)))

    while (!(RIFFChunkHeader.cChunkLabel[0] == 'd' && RIFFChunkHeader.cChunkLabel[1] == 'a' && RIFFChunkHeader.cChunkLabel[2] == 't' && RIFFChunkHeader.cChunkLabel[3] == 'a'))
    {
        // check for headers that go past the end of the file
        if (m_nFileBytes != APE_FILE_SIZE_UNDEFINED)
        {
            if (RIFFChunkHeader.nChunkBytes > (m_spIO->GetSize() - m_spIO->GetPosition()))
                return ERROR_INVALID_INPUT_FILE;
        }

        // move the file pointer to the end of this chunk
        CSmartPtr<unsigned char> spRIFFChunk(new unsigned char [RIFFChunkHeader.nChunkBytes], true);
        RETURN_ON_ERROR(ReadSafe(m_spIO, spRIFFChunk, static_cast<int>(RIFFChunkHeader.nChunkBytes)))

        // check again for the data chunk
        RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader)))
    }

    // we're at the data block
    m_nHeaderBytes = static_cast<uint32_t>(m_spIO->GetPosition());
    m_nDataBytes = (RIFFChunkHeader.nChunkBytes == static_cast<uint32_t>(-1)) ? static_cast<int64>(-1) : RIFFChunkHeader.nChunkBytes;
    if (m_nDataBytes == -1)
    {
        if (m_nFileBytes == -1)
        {
            m_nDataBytes = -1;
        }
        else
        {
            m_nDataBytes = m_nFileBytes - m_nHeaderBytes;
            m_nDataBytes = (m_nDataBytes / m_wfeSource.nBlockAlign) * m_wfeSource.nBlockAlign; // block align
        }
    }
    else if (m_nDataBytes > (m_nFileBytes - m_nHeaderBytes))
    {
        m_nDataBytes = m_nFileBytes - m_nHeaderBytes;
        m_nDataBytes = (m_nDataBytes / m_wfeSource.nBlockAlign) * m_wfeSource.nBlockAlign; // block align
    }

    // make sure the data bytes is a whole number of blocks
    if ((m_nDataBytes != -1) && ((m_nDataBytes % m_wfeSource.nBlockAlign) != 0))
        return ERROR_INVALID_INPUT_FILE;

    // calculate the terminating bytes
    m_nTerminatingBytes = static_cast<uint32_t>(m_nFileBytes - m_nDataBytes - m_nHeaderBytes);

    // no terminating data if we're unknown length (like a pipe) since seeking to read it would fail
    if (m_bUnknownLengthFile)
        m_nTerminatingBytes = 0;

    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CWAVInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    const int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    const int nReadResult = m_spIO->Read(pBuffer, static_cast<unsigned int>(nBytes), &nBytesRead);
    if (nReadResult != ERROR_SUCCESS)
        return nReadResult;

    if (pBlocksRetrieved) *pBlocksRetrieved = static_cast<int>(nBytesRead / m_wfeSource.nBlockAlign);

    return ERROR_SUCCESS;
}

int CWAVInputSource::GetHeaderData(unsigned char * pBuffer)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nResult = ERROR_SUCCESS;

    if (m_nHeaderBytes > 0)
    {
        int nFileBufferBytes = static_cast<int>(m_nHeaderBytes);
        const unsigned char * pFileBuffer = m_spIO->GetBuffer(&nFileBufferBytes);
        if (pFileBuffer != APE_NULL)
        {
            // we have the data already cached, so no need to seek and read
            memcpy(pBuffer, pFileBuffer, ape_min(static_cast<size_t>(m_nHeaderBytes), static_cast<size_t>(nFileBufferBytes)));
        }
        else
        {
            // use the base class
            nResult = GetHeaderDataHelper(m_bIsValid, pBuffer, m_nHeaderBytes, m_spIO);
        }
    }

    return nResult;
}

int CWAVInputSource::GetTerminatingData(unsigned char * pBuffer)
{
    return GetTerminatingDataHelper(m_bIsValid, pBuffer, m_nTerminatingBytes, m_spIO);
}

/**************************************************************************************************
CAIFFInputSource - wraps working with AIFF files
**************************************************************************************************/
/*static*/ bool CAIFFInputSource::GetHeaderMatches(BYTE aryHeader[64])
{
    bool bMatch = (aryHeader[0] == 'F' && aryHeader[1] == 'O' && aryHeader[2] == 'R' && aryHeader[3] == 'M');
    if (bMatch)
    {
        if ((aryHeader[8] == 'A') && (aryHeader[9] == 'I') && (aryHeader[10] == 'F') && (aryHeader[11] == 'F'))
        {
            // AIFF
        }
        else if ((aryHeader[8] == 'A') && (aryHeader[9] == 'I') && (aryHeader[10] == 'F') && (aryHeader[11] == 'C'))
        {
            // AIFC
        }
        else
        {
            // unknown
            bMatch = false;
        }
    }
    return bMatch;
}

CAIFFInputSource::CAIFFInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    m_bLittleEndian = false;
    m_bFloat = false;
    APE_CLEAR(m_wfeSource);

    if (pIO == APE_NULL || pwfeSource == APE_NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    m_spIO.Assign(pIO);

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / static_cast<int64>(m_wfeSource.nBlockAlign);
        if (pHeaderBytes) *pHeaderBytes = m_nHeaderBytes;
        if (pTerminatingBytes) *pTerminatingBytes = m_nTerminatingBytes;

        m_bIsValid = true;
    }

    if (pErrorCode) *pErrorCode = nResult;
}

CAIFFInputSource::~CAIFFInputSource()
{
}

int CAIFFInputSource::AnalyzeSource()
{
    // analyze AIFF header
    //
    // header has 54 bytes
    //    FORM                        - 4 bytes        "FORM"
    //      Size                        - 4                size of all data, excluding the top 8 bytes
    //      AIFF                        - 4                "AIFF"
    //        COMM                    - 4                "COMM"
    //          size                    - 4                size of COMM chunk excluding the 8 bytes for "COMM" and size, should be 18
    //            Channels            - 2                number of channels
    //            sampleFrames        - 4                number of frames
    //            sampleSize            - 2                size of each sample
    //            sampleRate            - 10            samples per second
    //        SSND                    - 4                "SSND"
    //          size                    - 4                size of all data in the chunk, excluding "SSND" and size field
    //            BlockAlign            - 4                normally set to 0
    //            Offset                - 4                normally set to 0
    //            Audio data follows

    // get the file size
    m_nFileBytes = m_spIO->GetSize();

    // get the RIFF header
    RIFF_HEADER RIFFHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFHeader, sizeof(RIFFHeader)))
    RIFFHeader.nBytes = Swap4Bytes(RIFFHeader.nBytes);

    // make sure the RIFF header is valid
    if (!(RIFFHeader.cRIFF[0] == 'F' && RIFFHeader.cRIFF[1] == 'O' && RIFFHeader.cRIFF[2] == 'R' && RIFFHeader.cRIFF[3] == 'M'))
        return ERROR_INVALID_INPUT_FILE;
    if (static_cast<int64>(RIFFHeader.nBytes) != (m_nFileBytes - static_cast<int64>(sizeof(RIFF_HEADER))))
        return ERROR_INVALID_INPUT_FILE;

    // read the AIFF header
    #pragma pack(push, 2)
    struct COMM_HEADER
    {
        int16 nChannels;
        uint32_t nFrames;
        int16 nSampleSize;
        uint16_t nSampleRateExponent;
        uint64_t nSampleRateMantissa;
    };
    #pragma pack(pop)

    // read AIFF header and only support AIFF
    char cAIFF[4] = { 0, 0, 0, 0 };
    RETURN_ON_ERROR(ReadSafe(m_spIO, &cAIFF[0], sizeof(cAIFF)))
    if ((cAIFF[0] == 'A') && (cAIFF[1] == 'I') && (cAIFF[2] == 'F') && (cAIFF[3] == 'F'))
    {
        // AIFF
    }
    else if ((cAIFF[0] == 'A') && (cAIFF[1] == 'I') && (cAIFF[2] == 'F') && (cAIFF[3] == 'C'))
    {
        // AIFC
    }
    else
    {
        // unknown type
        return ERROR_INVALID_INPUT_FILE;
    }

    // read chunks
    #pragma pack(push, 1)
    struct CHUNKS
    {
        char cChunkName[4];
        uint32_t nChunkBytes;
    };
    #pragma pack(pop)
    COMM_HEADER Common; APE_CLEAR(Common);
    while (true)
    {
        CHUNKS Chunk; APE_CLEAR(Chunk);
        RETURN_ON_ERROR(ReadSafe(m_spIO, &Chunk, sizeof(Chunk)))
        Chunk.nChunkBytes = Swap4Bytes(Chunk.nChunkBytes);
        Chunk.nChunkBytes = (Chunk.nChunkBytes + 1) & static_cast<uint32_t>(~1L);
        bool bSeekToNextChunk = true;

        if ((Chunk.cChunkName[0] == 'C') && (Chunk.cChunkName[1] == 'O') && (Chunk.cChunkName[2] == 'M') && (Chunk.cChunkName[3] == 'M'))
        {
            // read the common chunk

            // check the size
            if (sizeof(Common) > Chunk.nChunkBytes)
                return ERROR_INVALID_INPUT_FILE;
            RETURN_ON_ERROR(ReadSafe(m_spIO, &Common, sizeof(Common)))
            bSeekToNextChunk = false; // don't seek since we already read

            Common.nChannels = static_cast<int16>(Swap2Bytes(Common.nChannels));
            Common.nFrames = static_cast<uint32_t>(static_cast<uint32_t>(Swap4Bytes(Common.nFrames)));
            Common.nSampleSize = static_cast<int16>(Swap2Bytes(Common.nSampleSize));
            Common.nSampleRateExponent = static_cast<uint16>(Swap2Bytes(Common.nSampleRateExponent));
            Common.nSampleRateMantissa = Swap8Bytes(Common.nSampleRateMantissa);
            const double dSampleRate = GetExtendedDouble(Common.nSampleRateExponent, Common.nSampleRateMantissa);
            const uint32_t nSampleRate = static_cast<uint32_t>(dSampleRate);
            m_bFloat = false;

            // skip rest of header
            if (Chunk.nChunkBytes > sizeof(Common))
            {
                const int nExtraBytes = static_cast<int>(Chunk.nChunkBytes) - static_cast<int>(sizeof(Common));

                CSmartPtr<BYTE> spBuffer(new BYTE [static_cast<size_t>(nExtraBytes)], true);
                RETURN_ON_ERROR(ReadSafe(m_spIO, spBuffer, nExtraBytes))

                // COMM chunks can optionally have a compression type after the last cExtra parameter and in this case "sowt" mean we're little endian (reversed from normal AIFF)
                m_bLittleEndian = false;
                if (nExtraBytes >= 4)
                {
                    if ((spBuffer[0] == 'N') &&
                        (spBuffer[1] == 'O') &&
                        (spBuffer[2] == 'N') &&
                        (spBuffer[3] == 'E'))
                    {
                        // this means we're a supported file
                    }
                    else if ((spBuffer[0] == 's') &&
                        (spBuffer[1] == 'o') &&
                        (spBuffer[2] == 'w') &&
                        (spBuffer[3] == 't'))
                    {
                        m_bLittleEndian = true;
                    }
                    else if ((spBuffer[0] == 'f') &&
                        (spBuffer[1] == 'l') &&
                        (spBuffer[2] == '3') &&
                        (spBuffer[3] == '2'))
                    {
                        m_bFloat = true;
                        #ifndef APE_SUPPORT_FLOAT_COMPRESSION
                            // 32-bit floating point data (not supported)
                            return ERROR_INVALID_INPUT_FILE;
                        #endif
                    }
                    else
                    {
                        // unknown encoding, so we'll error out
                        return ERROR_INVALID_INPUT_FILE;
                    }
                }
            }

            // copy the format information to the WAVEFORMATEX passed in
            FillWaveFormatEx(&m_wfeSource, m_bFloat ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM, static_cast<int>(nSampleRate), static_cast<int>(Common.nSampleSize), static_cast<int>(Common.nChannels));
        }
        else if ((Chunk.cChunkName[0] == 'S') && (Chunk.cChunkName[1] == 'S') && (Chunk.cChunkName[2] == 'N') && (Chunk.cChunkName[3] == 'D'))
        {
            // read the SSND header
            struct SSNDHeader
            {
                uint32_t offset;
                uint32_t blocksize;
            };
            SSNDHeader Header;
            RETURN_ON_ERROR(ReadSafe(m_spIO, &Header, sizeof(Header)))
            m_nDataBytes = static_cast<int64>(Chunk.nChunkBytes) - 8;

            // check the size
            if ((Common.nFrames > 0) && (static_cast<int64>(m_nDataBytes / Common.nFrames) != static_cast<int64>(Common.nSampleSize * Common.nChannels / 8)))
                return ERROR_INVALID_INPUT_FILE;

            break;
        }

        if (bSeekToNextChunk)
        {
            const int nNextChunkBytes = static_cast<int>(Chunk.nChunkBytes);
            m_spIO->Seek(nNextChunkBytes, SeekFileCurrent);
        }
    }

    // make sure we found the SSND header
    if (m_nDataBytes <= 0)
        return ERROR_INVALID_INPUT_FILE;

    // calculate the header and terminating data
    m_nHeaderBytes = static_cast<uint32_t>(m_spIO->GetPosition());
    m_nTerminatingBytes = static_cast<uint32_t>(m_nFileBytes - (m_nHeaderBytes + m_nDataBytes));

    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CAIFFInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    const int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, static_cast<unsigned int>(nBytes), &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (m_wfeSource.wBitsPerSample == 8)
        Convert8BitSignedToUnsigned(pBuffer, m_wfeSource.nChannels, nBlocks);
    else if (!m_bLittleEndian)
        FlipEndian(pBuffer, m_wfeSource.wBitsPerSample, m_wfeSource.nChannels, nBlocks);

    if (pBlocksRetrieved) *pBlocksRetrieved = static_cast<int>(nBytesRead / m_wfeSource.nBlockAlign);

    return ERROR_SUCCESS;
}

int CAIFFInputSource::GetHeaderData(unsigned char * pBuffer)
{
    return GetHeaderDataHelper(m_bIsValid, pBuffer, m_nHeaderBytes, m_spIO);
}

int CAIFFInputSource::GetTerminatingData(unsigned char * pBuffer)
{
    return GetTerminatingDataHelper(m_bIsValid, pBuffer, m_nTerminatingBytes, m_spIO);
}

double CAIFFInputSource::GetExtendedDouble(uint16_t exponent, uint64_t mantissa)
{
    // this code is borrowed from David Bryant's WavPack
    // he said it derives from this:
    // https://en.wikipedia.org/wiki/Extended_precision#x86_extended_precision_format
    // there's also code here:
    // https://stackoverflow.com/questions/2963055/convert-extended-precision-float-80-bit-to-double-64-bit-in-msvc

    const double sign = (exponent & 0x8000) ? -1.0 : 1.0, value = static_cast<double>(mantissa);
    const double scaler = pow(2.0, static_cast<double>(exponent & 0x7fff) - 16446);
    const double result = value * scaler * sign;
    return result;
}

bool CAIFFInputSource::GetIsBigEndian() const
{
    return !m_bLittleEndian;
}

/**************************************************************************************************
CW64InputSource - wraps working with W64 files
**************************************************************************************************/
/*static*/ bool CW64InputSource::GetHeaderMatches(BYTE aryHeader[64])
{
    static const GUID guidRIFF = { 0x66666972, 0x912E, 0x11CF, { 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00 } };
    static const GUID guidWAVE = { 0x65766177, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A } };
    bool bW64 = (memcmp(aryHeader, &guidRIFF, sizeof(guidRIFF)) == 0);
    if (bW64)
    {
        if (memcmp(&aryHeader[24], &guidWAVE, sizeof(GUID)) != 0)
        {
            bW64 = false;
        }
    }

    return bW64;
}

CW64InputSource::CW64InputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_bFloat = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    APE_CLEAR(m_wfeSource);

    if (pIO == APE_NULL || pwfeSource == APE_NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    m_spIO.Assign(pIO);

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / static_cast<int64>(m_wfeSource.nBlockAlign);
        if (pHeaderBytes) *pHeaderBytes = m_nHeaderBytes;
        if (pTerminatingBytes) *pTerminatingBytes = m_nTerminatingBytes;

        m_bIsValid = true;
    }

    if (pErrorCode) *pErrorCode = nResult;
}

CW64InputSource::~CW64InputSource()
{
}

int CW64InputSource::AnalyzeSource()
{
    // chunk identifiers
    static const GUID guidRIFF = { 0x66666972, 0x912E, 0x11CF, { 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00 } };
    static const GUID guidWAVE = { 0x65766177, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A } };
    static const GUID guidDATA = { 0x61746164, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A } };
    static const GUID guidFMT = { 0x20746D66, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A } };
    const bool bReadMetadataChunks = false;

    // read the riff header
    bool bDataChunkRead = false;
    bool bFormatChunkRead = false;
    W64ChunkHeader RIFFHeader;
    unsigned int nBytesRead = 0;
    m_nFileBytes = m_spIO->GetSize();

    m_spIO->Read(&RIFFHeader, sizeof(RIFFHeader), &nBytesRead);
    if ((memcmp(&RIFFHeader.guidIdentifier, &guidRIFF, sizeof(GUID)) == 0) && (RIFFHeader.nBytes == static_cast<uint64>(m_nFileBytes)))
    {
        // read and verify the wave data type header
        GUID DataHeader;
        const unsigned int nDataHeaderSize = static_cast<unsigned int>(sizeof(DataHeader));
        m_spIO->Read(&DataHeader, nDataHeaderSize, &nBytesRead);
        if (memcmp(&DataHeader, &guidWAVE, sizeof(GUID)) == 0)
        {
            // for now, we only need to process these two chunks besides 'fmt ' chunk above -
            // "data", and "id3 "/"tag "
            while (1)
            {
                // read chunks one by one
                W64ChunkHeader Header;
                m_spIO->Read(&Header, sizeof(Header), &nBytesRead);

                // perhaps we have reached EOF
                if (nBytesRead < sizeof(Header))
                    break;

                // get / check chunk size
                const int64 nChunkRemainingBytes = static_cast<int64>(Header.nBytes) - static_cast<int64>(sizeof(Header));
                if ((m_spIO->GetPosition() + nChunkRemainingBytes) > m_nFileBytes)
                    break;

                // switched based on the chunk type
                if ((memcmp(&Header.guidIdentifier, &guidFMT, sizeof(GUID)) == 0) &&
                    (nChunkRemainingBytes >= static_cast<APE::int64>(sizeof(WAVFormatChunkData))))
                {
                    // read data
                    WAVFormatChunkData Data;
                    m_spIO->Read(&Data, sizeof(Data), &nBytesRead);
                    if (nBytesRead != sizeof(Data))
                        break;

                    // skip the rest
                    m_spIO->Seek(Align(nChunkRemainingBytes, 8) - static_cast<int64>(sizeof(Data)), SeekFileCurrent);

                    // verify the format (must be WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE)
                    m_bFloat = false;
                    if (Data.nFormatTag == WAVE_FORMAT_IEEE_FLOAT)
                    {
                        #ifndef APE_SUPPORT_FLOAT_COMPRESSION
                            break;
                        #endif
                        m_bFloat = true;
                    }
                    else if ((Data.nFormatTag != WAVE_FORMAT_PCM) && (Data.nFormatTag != WAVE_FORMAT_EXTENSIBLE))
                    {
                        break;
                    }

                    // copy information over for internal storage
                    // may want to error check this header (bad avg bytes per sec, bad format, bad block align, etc...)
                    FillWaveFormatEx(&m_wfeSource, m_bFloat ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM, static_cast<int>(Data.nSamplesPerSecond), static_cast<int>(Data.nBitsPerSample), static_cast<int>(Data.nChannels));

                    m_wfeSource.nAvgBytesPerSec = Data.nAverageBytesPerSecond;
                    m_wfeSource.nBlockAlign = Data.nBlockAlign;

                    bFormatChunkRead = true;

                    // short circuit if we don't need metadata
                    if (!bReadMetadataChunks && (bFormatChunkRead && bDataChunkRead))
                        break;
                }
                else if (memcmp(&Header.guidIdentifier, &guidDATA, sizeof(GUID)) == 0)
                {
                    // 'data' chunk

                    // fill in the data bytes (the length of the 'data' chunk)
                    m_nDataBytes = nChunkRemainingBytes;
                    m_nHeaderBytes = static_cast<uint32_t>(m_spIO->GetPosition());

                    bDataChunkRead = true;

                    // short circuit if we don't need metadata
                    if (!bReadMetadataChunks && (bFormatChunkRead && bDataChunkRead))
                        break;

                    // move to the end of WAVEFORM data, so we can read other chunks behind it (if necessary)
                    m_spIO->Seek(Align(nChunkRemainingBytes, 8), SeekFileCurrent);
                }
                else
                {
                    m_spIO->Seek(Align(nChunkRemainingBytes, 8), SeekFileCurrent);
                }
            }
        }
    }

    // we must read both the data and format chunks
    if (bDataChunkRead && bFormatChunkRead)
    {
        // should error check this maybe
        m_nDataBytes = ape_min(m_nDataBytes, m_nFileBytes - m_nHeaderBytes);

        // get terminating bytes
        m_nTerminatingBytes = static_cast<uint32_t>(m_nFileBytes - m_nDataBytes - m_nHeaderBytes);

        // we're valid if we make it this far
        m_bIsValid = true;
    }

    // we made it this far, everything must be cool
    return m_bIsValid ? ERROR_SUCCESS : ERROR_INVALID_INPUT_FILE;
}

int CW64InputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    const unsigned int nBytes = static_cast<unsigned int>(m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (pBlocksRetrieved) *pBlocksRetrieved = static_cast<int>(nBytesRead / m_wfeSource.nBlockAlign);

    return ERROR_SUCCESS;
}

int CW64InputSource::GetHeaderData(unsigned char * pBuffer)
{
    return GetHeaderDataHelper(m_bIsValid, pBuffer, m_nHeaderBytes, m_spIO);
}

int CW64InputSource::GetTerminatingData(unsigned char * pBuffer)
{
    return GetTerminatingDataHelper(m_bIsValid, pBuffer, m_nTerminatingBytes, m_spIO);
}

int64 CW64InputSource::Align(int64 nValue, int nAlignment)
{
    ASSERT(nAlignment > 0 && ((nAlignment & (nAlignment - 1)) == 0));
    return (nValue + nAlignment - 1) & ~((static_cast<int64>(nAlignment) - 1));
}

/**************************************************************************************************
CSNDInputSource - wraps working with SND files
**************************************************************************************************/
/*static*/ bool CSNDInputSource::GetHeaderMatches(BYTE aryHeader[64])
{
    if (memcmp(&aryHeader[0], "dns.", 4) == 0)
    {
        return true;
    }
    else if (memcmp(&aryHeader[0], ".snd", 4) == 0)
    {
        return true;
    }

    return false;
}

CSNDInputSource::CSNDInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode, int32 * pFlags)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    m_bBigEndian = false;
    APE_CLEAR(m_wfeSource);

    if (pIO == APE_NULL || pwfeSource == APE_NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    m_spIO.Assign(pIO);

    int nResult = AnalyzeSource(pFlags);
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / static_cast<int64>(m_wfeSource.nBlockAlign);
        if (pHeaderBytes) *pHeaderBytes = m_nHeaderBytes;
        if (pTerminatingBytes) *pTerminatingBytes = m_nTerminatingBytes;

        m_bIsValid = true;
    }

    if (pErrorCode) *pErrorCode = nResult;
}

CSNDInputSource::~CSNDInputSource()
{
}

int CSNDInputSource::AnalyzeSource(int32 * pFlags)
{
    bool bIsValid = false;
    bool bSupportedFormat = false;

    // get the file size (may want to error check this for files over 2 GB)
    m_nFileBytes = m_spIO->GetSize();

    // read the AU header
    class CAUHeader
    {
    public:
        uint32_t m_nMagicNumber;
        uint32_t m_nDataOffset;
        uint32_t m_nDataSize;
        uint32_t m_nEncoding;
        uint32_t m_nSampleRate;
        uint32_t m_nChannels;
    };
    CAUHeader Header; APE_CLEAR(Header);
    unsigned int nBytesRead = 0;
    if ((m_spIO->Read(&Header, sizeof(Header), &nBytesRead) == ERROR_SUCCESS) &&
        (nBytesRead == sizeof(Header)))
    {
        bool bMagicNumberValid = false;
        if (memcmp(&Header.m_nMagicNumber, "dns.", 4) == 0)
        {
            // already little-endian
            bMagicNumberValid = true;
        }
        else if (memcmp(&Header.m_nMagicNumber, ".snd", 4) == 0)
        {
            // big-endian (so reverse)
            bMagicNumberValid = true;
            m_bBigEndian = true;
            Header.m_nDataOffset = FlipByteOrder32(Header.m_nDataOffset);
            Header.m_nDataSize = FlipByteOrder32(Header.m_nDataSize);
            Header.m_nEncoding = FlipByteOrder32(Header.m_nEncoding);
            Header.m_nSampleRate = FlipByteOrder32(Header.m_nSampleRate);
            Header.m_nChannels = FlipByteOrder32(Header.m_nChannels);
        }

        if (bMagicNumberValid &&
            (Header.m_nDataOffset >= sizeof(Header)) &&
            (Header.m_nDataOffset < m_nFileBytes))
        {
            // get sizes
            m_nHeaderBytes = Header.m_nDataOffset;
            m_nDataBytes = m_nFileBytes - m_nHeaderBytes;
            if (Header.m_nDataSize > 0)
                m_nDataBytes = ape_min(static_cast<int64>(Header.m_nDataSize), m_nDataBytes);
            m_nTerminatingBytes = static_cast<uint32>(m_nFileBytes - m_nHeaderBytes - m_nDataBytes);

            // set format
            if (Header.m_nEncoding == 1)
            {
                // 8-bit mulaw
                // not supported
            }
            else if (Header.m_nEncoding == 2)
            {
                // 8-bit PCM (signed)
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, static_cast<int>(Header.m_nSampleRate), 8, static_cast<int>(Header.m_nChannels));
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 3)
            {
                // 16-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, static_cast<int>(Header.m_nSampleRate), 16, static_cast<int>(Header.m_nChannels));
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 4)
            {
                // 24-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, static_cast<int>(Header.m_nSampleRate), 24, static_cast<int>(Header.m_nChannels));
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 5)
            {
                // 32-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, static_cast<int>(Header.m_nSampleRate), 32, static_cast<int>(Header.m_nChannels));
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 6)
            {
                // 32-bit float
                #ifdef APE_SUPPORT_FLOAT_COMPRESSION
                    FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_IEEE_FLOAT, static_cast<int>(Header.m_nSampleRate), 32, static_cast<int>(Header.m_nChannels));
                    bSupportedFormat = true;
                #else
                    // not supported
                #endif
            }
            else if (Header.m_nEncoding == 7)
            {
                // 64-bit float
                // not supported
            }
            else
            {
                // unsupported format
                ASSERT(false);
            }
        }
        else
        {
            // invalid header
            ASSERT(false);
        }

        // update return value
        if (bSupportedFormat)
            bIsValid = true;
    }

    // seek to the end of the header
    m_spIO->Seek(m_nHeaderBytes, SeekFileBegin);

    // update flags
    *pFlags |= APE_FORMAT_FLAG_SND;
    if (m_bBigEndian)
        *pFlags |= APE_FORMAT_FLAG_BIG_ENDIAN;

    // we made it this far, everything must be cool
    return bIsValid ? ERROR_SUCCESS : ERROR_INVALID_INPUT_FILE;
}

uint32 CSNDInputSource::FlipByteOrder32(uint32 nValue)
{
    return (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
}

int CSNDInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    const unsigned int nBytes = static_cast<unsigned int>(m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (m_wfeSource.wBitsPerSample == 8)
        Convert8BitSignedToUnsigned(pBuffer, m_wfeSource.nChannels, nBlocks);
    else if (m_bBigEndian)
        FlipEndian(pBuffer, m_wfeSource.wBitsPerSample, m_wfeSource.nChannels, nBlocks);

    if (pBlocksRetrieved) *pBlocksRetrieved = static_cast<int>(nBytesRead / m_wfeSource.nBlockAlign);

    return ERROR_SUCCESS;
}

int CSNDInputSource::GetHeaderData(unsigned char * pBuffer)
{
    return GetHeaderDataHelper(m_bIsValid, pBuffer, m_nHeaderBytes, m_spIO);
}

int CSNDInputSource::GetTerminatingData(unsigned char * pBuffer)
{
    return GetTerminatingDataHelper(m_bIsValid, pBuffer, m_nTerminatingBytes, m_spIO);
}

/**************************************************************************************************
CCAFInputSource - wraps working with CAF files
**************************************************************************************************/
struct APE_CAFFileHeader {
    char cFileType[4]; // should equal 'caff'
    uint16 mFileVersion;
    uint16 mFileFlags;
};

/*static*/ bool CCAFInputSource::GetHeaderMatches(BYTE aryHeader[64])
{
    APE_CAFFileHeader Header;
    memcpy(&Header, &aryHeader[0], sizeof(APE_CAFFileHeader));
    Header.mFileVersion = static_cast<uint16>(Swap2Bytes(Header.mFileVersion));
    Header.mFileFlags = static_cast<uint16>(Swap2Bytes(Header.mFileFlags));

    if ((Header.cFileType[0] != 'c') ||
        (Header.cFileType[1] != 'a') ||
        (Header.cFileType[2] != 'f') ||
        (Header.cFileType[3] != 'f'))
    {
        return false;
    }

    if (Header.mFileVersion != 1)
    {
        return false;
    }

    return true;
}

CCAFInputSource::CCAFInputSource(CIO * pIO, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    m_bLittleEndian = false;
    APE_CLEAR(m_wfeSource);

    if (pIO == APE_NULL || pwfeSource == APE_NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    m_spIO.Assign(pIO);

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / static_cast<int64>(m_wfeSource.nBlockAlign);
        if (pHeaderBytes) *pHeaderBytes = m_nHeaderBytes;
        if (pTerminatingBytes) *pTerminatingBytes = m_nTerminatingBytes;

        m_bIsValid = true;
    }

    if (pErrorCode) *pErrorCode = nResult;
}

CCAFInputSource::~CCAFInputSource()
{
}

int CCAFInputSource::AnalyzeSource()
{
    // get the file size
    m_nFileBytes = m_spIO->GetSize();

    // get the header
    APE_CAFFileHeader Header;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &Header, sizeof(Header)))
    Header.mFileVersion = static_cast<uint16>(Swap2Bytes(Header.mFileVersion));
    Header.mFileFlags = static_cast<uint16>(Swap2Bytes(Header.mFileFlags));

    // check the header
    if ((Header.cFileType[0] != 'c') ||
        (Header.cFileType[1] != 'a') ||
        (Header.cFileType[2] != 'f') ||
        (Header.cFileType[3] != 'f'))
    {
        return ERROR_INVALID_INPUT_FILE;
    }

    if (Header.mFileVersion != 1)
    {
        return ERROR_INVALID_INPUT_FILE;
    }

    // read chunks
    #pragma pack(push, 1)
    struct APE_CAFChunkHeader {
        char cChunkType[4];
        uint64 mChunkSize;
    };
    struct APE_CAFAudioFormat {
        union
        {
            uint64 nSampleRate;
            double dSampleRate;
        };
        char cFormatID[4];
        uint32_t nFormatFlags;
        uint32_t nBytesPerPacket;
        uint32_t nFramesPerPacket;
        uint32_t nChannelsPerFrame;
        uint32_t nBitsPerChannel;
    };
    enum {
        APE_kCAFLinearPCMFormatFlagIsFloat = (1L << 0),
        APE_kCAFLinearPCMFormatFlagIsLittleEndian = (1L << 1)
    };
    #pragma pack(pop)

    bool bFoundDesc = false;
    while (true)
    {
        APE_CAFChunkHeader Chunk;
        if (ReadSafe(m_spIO, &Chunk, sizeof(Chunk)) != ERROR_SUCCESS)
            return ERROR_INVALID_INPUT_FILE; // we read past the last chunk and didn't find the necessary chunks

        Chunk.mChunkSize = Swap8Bytes(Chunk.mChunkSize);

        if ((Chunk.cChunkType[0] == 'd') &&
            (Chunk.cChunkType[1] == 'e') &&
            (Chunk.cChunkType[2] == 's') &&
            (Chunk.cChunkType[3] == 'c'))
        {
            if (Chunk.mChunkSize == sizeof(APE_CAFAudioFormat))
            {
                APE_CAFAudioFormat AudioFormat;
                RETURN_ON_ERROR(ReadSafe(m_spIO, &AudioFormat, sizeof(AudioFormat)))

                if ((AudioFormat.cFormatID[0] != 'l') ||
                    (AudioFormat.cFormatID[1] != 'p') ||
                    (AudioFormat.cFormatID[2] != 'c') ||
                    (AudioFormat.cFormatID[3] != 'm'))
                {
                    return ERROR_INVALID_INPUT_FILE;
                }

                AudioFormat.nSampleRate = Swap8Bytes(AudioFormat.nSampleRate);
                AudioFormat.nBitsPerChannel = Swap4Bytes(AudioFormat.nBitsPerChannel);
                AudioFormat.nChannelsPerFrame = Swap4Bytes(AudioFormat.nChannelsPerFrame);
                AudioFormat.nFormatFlags = Swap4Bytes(AudioFormat.nFormatFlags);

                // only support 8-bit, 16-bit, and 24-bit, maybe 32-bit
                bool bFloat = false;
                if (AudioFormat.nBitsPerChannel == 32)
                {
                    if (AudioFormat.nFormatFlags & APE_kCAFLinearPCMFormatFlagIsFloat)
                    {
                        #ifndef APE_SUPPORT_FLOAT_COMPRESSION
                            return ERROR_INVALID_INPUT_FILE;
                        #endif
                        bFloat = true;
                    }
                }
                else if ((AudioFormat.nBitsPerChannel != 8) && (AudioFormat.nBitsPerChannel != 16) && (AudioFormat.nBitsPerChannel != 24))
                {
                    return ERROR_INVALID_INPUT_FILE;
                }

                // if we're little endian, mark that
                if (AudioFormat.nFormatFlags & APE_kCAFLinearPCMFormatFlagIsLittleEndian)
                    m_bLittleEndian = true;

                FillWaveFormatEx(&m_wfeSource, bFloat ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM, static_cast<int>(AudioFormat.dSampleRate), static_cast<int>(AudioFormat.nBitsPerChannel), static_cast<int>(AudioFormat.nChannelsPerFrame));
                bFoundDesc = true;
            }
            else
            {
                return ERROR_INVALID_INPUT_FILE;
            }
        }
        else if ((Chunk.cChunkType[0] == 'd') &&
            (Chunk.cChunkType[1] == 'a') &&
            (Chunk.cChunkType[2] == 't') &&
            (Chunk.cChunkType[3] == 'a'))
        {
            // if we didn't first find the description chunk, fail on this file
            if (bFoundDesc == false)
                return ERROR_INVALID_INPUT_FILE;

            // calculate the header and terminating data
            m_nHeaderBytes = static_cast<uint32>(m_spIO->GetPosition());

            // data bytes are this chunk
            m_nDataBytes = static_cast<int64>(Chunk.mChunkSize);

            // align at the block size
            m_nDataBytes = (m_nDataBytes / m_wfeSource.nBlockAlign) * m_wfeSource.nBlockAlign;

            // terminating bytes are whatever is left
            m_nTerminatingBytes = static_cast<uint32>(m_nFileBytes - (m_nHeaderBytes + m_nDataBytes));

            // we made it this far, everything must be cool
            break;
        }
        else
        {
            // skip this chunk
            m_spIO->Seek(static_cast<int64>(Chunk.mChunkSize), SeekFileCurrent);
        }
    }

    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CCAFInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    const unsigned int nBytes = static_cast<unsigned int>(m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    // read data
    if (m_wfeSource.wBitsPerSample == 8)
        Convert8BitSignedToUnsigned(pBuffer, m_wfeSource.nChannels, nBlocks);
    else if (!m_bLittleEndian)
        FlipEndian(pBuffer, m_wfeSource.wBitsPerSample, m_wfeSource.nChannels, nBlocks);

    if (pBlocksRetrieved) *pBlocksRetrieved = static_cast<int>(nBytesRead / m_wfeSource.nBlockAlign);

    return ERROR_SUCCESS;
}

int CCAFInputSource::GetHeaderData(unsigned char * pBuffer)
{
    return GetHeaderDataHelper(m_bIsValid, pBuffer, m_nHeaderBytes, m_spIO);
}

int CCAFInputSource::GetTerminatingData(unsigned char * pBuffer)
{
    return GetTerminatingDataHelper(m_bIsValid, pBuffer, m_nTerminatingBytes, m_spIO);
}

bool CCAFInputSource::GetIsBigEndian() const
{
    return !m_bLittleEndian;
}

}
