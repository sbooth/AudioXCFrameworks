#include "All.h"
#include "WAVInputSource.h"
#include "IO.h"
#include "MACLib.h"
#include "GlobalFunctions.h"

namespace APE
{

#define Swap2Bytes(val) \
((((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00))

#define Swap4Bytes(val) \
((((val) >> 24) & 0x000000FF) | (((val) >> 8) & 0x0000FF00) | \
(((val) << 8) & 0x00FF0000) | (((val) << 24) & 0xFF000000))

#define Swap8Bytes(val) \
((((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | \
(((val) >> 24) & 0x0000000000FF0000) | (((val) >> 8) & 0x00000000FF000000) | \
(((val) << 8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | \
(((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000))

struct RIFF_HEADER 
{
    char cRIFF[4];          // the characters 'RIFF' indicating that it's a RIFF file
    uint32 nBytes;          // the number of bytes following this header
};

struct DATA_TYPE_ID_HEADER 
{
    char cDataTypeID[4];      // should equal 'WAVE' for a WAV file
};

struct WAV_FORMAT_HEADER
{
    uint16 nFormatTag;            // the format of the WAV...should equal 1 for a PCM file
    uint16 nChannels;             // the number of channels
    uint32 nSamplesPerSecond;     // the number of samples per second
    uint32 nBytesPerSecond;       // the bytes per second
    uint16 nBlockAlign;           // block alignment
    uint16 nBitsPerSample;        // the number of bits per sample
};

struct RIFF_CHUNK_HEADER
{
    char cChunkLabel[4];      // should equal "data" indicating the data chunk
    uint32 nChunkBytes;       // the bytes of the chunk  
};

CInputSource * CreateInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int32 * pFlags, int * pErrorCode)
{ 
    // error check the parameters
    if ((pSourceName == NULL) || (wcslen(pSourceName) == 0))
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return NULL;
    }

    // get the extension
    const wchar_t * pExtension = &pSourceName[wcslen(pSourceName)];
    while ((pExtension > pSourceName) && (*pExtension != '.'))
        pExtension--;

    // create the proper input source
    if (StringIsEqual(pExtension, L".wav", false) || StringIsEqual(pSourceName, L"-", false))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        return new CWAVInputSource(pSourceName, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
    }
    else if (StringIsEqual(pExtension, L".aiff", false) || StringIsEqual(pExtension, L".aif", false))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        *pFlags |= MAC_FORMAT_FLAG_AIFF | MAC_FORMAT_FLAG_BIG_ENDIAN;
        return new CAIFFInputSource(pSourceName, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
    }
    else if (StringIsEqual(pExtension, L".w64", false))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        *pFlags |= MAC_FORMAT_FLAG_W64;
        return new CW64InputSource(pSourceName, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
    }
    else if (StringIsEqual(pExtension, L".snd", false) || StringIsEqual(pExtension, L".au", false))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        return new CSNDInputSource(pSourceName, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode, pFlags);
    }
    else if (StringIsEqual(pExtension, L".caf", false))
    {
        if (pErrorCode) *pErrorCode = ERROR_SUCCESS;
        *pFlags |= MAC_FORMAT_FLAG_CAF | MAC_FORMAT_FLAG_BIG_ENDIAN;
        return new CCAFInputSource(pSourceName, pwfeSource, pTotalBlocks, pHeaderBytes, pTerminatingBytes, pErrorCode);
    }
    else
    {
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return NULL;
    }
}

/**************************************************************************************************
CInputSource - base input format class (allows multiple format support)
**************************************************************************************************/
int CInputSource::GetHeaderDataHelper(bool bIsValid, unsigned char * pBuffer, uint32 nHeaderBytes, CIO * pIO)
{
    if (!bIsValid) return ERROR_UNDEFINED;

    int nResult = ERROR_SUCCESS;

    if (nHeaderBytes > 0)
    {
        int64 nOriginalFileLocation = pIO->GetPosition();

        if (nOriginalFileLocation != 0)
        {
            pIO->SetSeekMethod(APE_FILE_BEGIN);
            pIO->SetSeekPosition(0);
            pIO->PerformSeek();
        }

        unsigned int nBytesRead = 0;
        int nReadRetVal = pIO->Read(pBuffer, nHeaderBytes, &nBytesRead);

        if ((nReadRetVal != ERROR_SUCCESS) || (nHeaderBytes != int(nBytesRead)))
        {
            nResult = ERROR_UNDEFINED;
        }

        pIO->SetSeekMethod(APE_FILE_BEGIN);
        pIO->SetSeekPosition(nOriginalFileLocation);
        pIO->PerformSeek();
    }

    return nResult;
}

int CInputSource::GetTerminatingDataHelper(bool bIsValid, unsigned char * pBuffer, uint32 nTerminatingBytes, CIO * pIO)
{
    if (!bIsValid) return ERROR_UNDEFINED;

    int nResult = ERROR_SUCCESS;

    if (nTerminatingBytes > 0)
    {
        int64 nOriginalFileLocation = pIO->GetPosition();

        pIO->SetSeekMethod(APE_FILE_END);
        pIO->SetSeekPosition(-int64(nTerminatingBytes));
        pIO->PerformSeek();

        unsigned int nBytesRead = 0;
        int nReadRetVal = pIO->Read(pBuffer, nTerminatingBytes, &nBytesRead);

        if ((nReadRetVal != ERROR_SUCCESS) || (nTerminatingBytes != int(nBytesRead)))
        {
            nResult = ERROR_UNDEFINED;
        }

        pIO->SetSeekMethod(APE_FILE_BEGIN);
        pIO->SetSeekPosition(nOriginalFileLocation);
        pIO->PerformSeek();
    }

    return nResult;
}

/**************************************************************************************************
CWAVInputSource - wraps working with WAV files
**************************************************************************************************/
CWAVInputSource::CWAVInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nTerminatingBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    memset(&m_wfeSource, 0, sizeof(m_wfeSource));

    m_bUnknownLengthPipe = false;

    if (pSourceName == NULL || pwfeSource == NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }
    
    m_spIO.Assign(CreateCIO());
    if (m_spIO->Open(pSourceName, true) != ERROR_SUCCESS)
    {
        m_spIO.Delete();
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // read to a buffer so pipes work (that way we don't have to seek back to get the header)
    m_spIO->SetReadToBuffer();

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / int64(m_wfeSource.nBlockAlign);
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
    // see if we're a pipe
    bool bPipe = false;
    TCHAR cName[1024] = { 0 };
    m_spIO->GetName(cName);
    if (_tcsicmp(cName, _T("-")) == 0)
        bPipe = true;
    
    // get the file size
    m_nFileBytes = m_spIO->GetSize();

    // get the RIFF header
    RIFF_HEADER RIFFHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFHeader, sizeof(RIFFHeader))) 

    // make sure the RIFF header is valid
    if (!(RIFFHeader.cRIFF[0] == 'R' && RIFFHeader.cRIFF[1] == 'I' && RIFFHeader.cRIFF[2] == 'F' && RIFFHeader.cRIFF[3] == 'F') &&
        !(RIFFHeader.cRIFF[0] == 'R' && RIFFHeader.cRIFF[1] == 'F' && RIFFHeader.cRIFF[2] == '6' && RIFFHeader.cRIFF[3] == '4'))
        return ERROR_INVALID_INPUT_FILE;

    // get the file size from RIFF header in case we're a pipe and use the maximum
    if (RIFFHeader.nBytes != -1)
    {
        // use maximum size between header and file size
        int64 nHeaderBytes = int64(RIFFHeader.nBytes) + sizeof(RIFF_HEADER);
        m_nFileBytes = ape_max(m_nFileBytes, nHeaderBytes);
    }
    else
    {
        if (bPipe)
        {
            // mark that we need to read the pipe with unknown length
            m_bUnknownLengthPipe = true;
            m_nFileBytes = -1;
        }
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
        // move the file pointer to the end of this chunk
        CSmartPtr<unsigned char> spExtraChunk(new unsigned char [RIFFChunkHeader.nChunkBytes], true);
        RETURN_ON_ERROR(ReadSafe(m_spIO, spExtraChunk, RIFFChunkHeader.nChunkBytes))

        // check again for the data chunk
        RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader))) 
    }
    
    // read the format info
    WAV_FORMAT_HEADER WAVFormatHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &WAVFormatHeader, sizeof(WAVFormatHeader))) 

    // error check the header to see if we support it
    if ((WAVFormatHeader.nFormatTag != WAVE_FORMAT_PCM) && (WAVFormatHeader.nFormatTag != WAVE_FORMAT_EXTENSIBLE))
        return ERROR_INVALID_INPUT_FILE;

    // if the format is an odd bits per sample, just update to a known number -- decoding stores the header so will still be correct (and the block align is that size anyway)
    int nSampleBits = 8 * WAVFormatHeader.nBlockAlign / ape_max(1, WAVFormatHeader.nChannels);
    if (nSampleBits > 0)
        WAVFormatHeader.nBitsPerSample = (uint16) (((WAVFormatHeader.nBitsPerSample + (nSampleBits - 1)) / nSampleBits) * nSampleBits);

    // copy the format information to the WAVEFORMATEX passed in
    FillWaveFormatEx(&m_wfeSource, WAVFormatHeader.nFormatTag, WAVFormatHeader.nSamplesPerSecond, WAVFormatHeader.nBitsPerSample, WAVFormatHeader.nChannels);

    // skip over any extra data in the header
    if (RIFFChunkHeader.nChunkBytes != -1)
    {
        int nWAVFormatHeaderExtra = RIFFChunkHeader.nChunkBytes - sizeof(WAVFormatHeader);
        if (nWAVFormatHeaderExtra < 0)
        {
            return ERROR_INVALID_INPUT_FILE;
        }
        else if (nWAVFormatHeaderExtra > 0)
        {
            // read the extra
            CSmartPtr<unsigned char> spWAVFormatHeaderExtra(new unsigned char [nWAVFormatHeaderExtra], true);
            RETURN_ON_ERROR(ReadSafe(m_spIO, spWAVFormatHeaderExtra, nWAVFormatHeaderExtra));

            // the extra specifies the format and it might not be PCM, so check
            #pragma pack(push, 1)
            struct CWAVFormatExtra
            {
                uint16 cbSize;
                uint16 nValieBitsPerSample;
                uint32 nChannelMask;
                BYTE guidSubFormat[16];
            };
            #pragma pack(pop)

            if (nWAVFormatHeaderExtra >= sizeof(CWAVFormatExtra))
            {
                CWAVFormatExtra * pExtra = (CWAVFormatExtra *) spWAVFormatHeaderExtra.GetPtr();
                
                const BYTE guidPCM[16] = { 1, 0, 0, 0, 0, 0, 16, 0, 128, 0, 0, 170, 0, 56, 155, 113 }; // KSDATAFORMAT_SUBTYPE_PCM but that isn't cross-platform
                if (memcmp(&pExtra->guidSubFormat, &guidPCM, 16) != 0)
                {
                    // we're not PCM, so error
                    return ERROR_INVALID_INPUT_FILE;
                }
            }
        }
    }
    
    // find the data chunk
    RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader))) 

    while (!(RIFFChunkHeader.cChunkLabel[0] == 'd' && RIFFChunkHeader.cChunkLabel[1] == 'a' && RIFFChunkHeader.cChunkLabel[2] == 't' && RIFFChunkHeader.cChunkLabel[3] == 'a')) 
    {
        // move the file pointer to the end of this chunk
        CSmartPtr<unsigned char> spRIFFChunk(new unsigned char[RIFFChunkHeader.nChunkBytes], true);
        RETURN_ON_ERROR(ReadSafe(m_spIO, spRIFFChunk, RIFFChunkHeader.nChunkBytes));

        // check again for the data chunk
        RETURN_ON_ERROR(ReadSafe(m_spIO, &RIFFChunkHeader, sizeof(RIFFChunkHeader))) 
    }

    // we're at the data block
    m_nHeaderBytes = (uint32) m_spIO->GetPosition();
    m_nDataBytes = (RIFFChunkHeader.nChunkBytes == -1) ? int64(-1) : RIFFChunkHeader.nChunkBytes;
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
    m_nTerminatingBytes = (uint32) (m_nFileBytes - m_nDataBytes - m_nHeaderBytes);

    // no terminating data if we're a pipe (since seeking to read it would fail)
    if (bPipe)
        m_nTerminatingBytes = 0;

    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CWAVInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    int nReadResult = m_spIO->Read(pBuffer, nBytes, &nBytesRead);
    if (nReadResult != ERROR_SUCCESS)
        return nReadResult;

    if (pBlocksRetrieved) *pBlocksRetrieved = (nBytesRead / m_wfeSource.nBlockAlign);

    return ERROR_SUCCESS;
}

int CWAVInputSource::GetHeaderData(unsigned char * pBuffer)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nResult = ERROR_SUCCESS;

    if (m_nHeaderBytes > 0)
    {
        int nFileBufferBytes = int(m_nHeaderBytes);
        unsigned char * pFileBuffer = m_spIO->GetBuffer(&nFileBufferBytes);
        if (pFileBuffer != NULL)
        {
            // we have the data already cached, so no need to seek and read
            memcpy(pBuffer, pFileBuffer, (size_t) ape_min(m_nHeaderBytes, uint32(nFileBufferBytes)));
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
CAIFFInputSource::CAIFFInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    memset(&m_wfeSource, 0, sizeof(m_wfeSource));

    if (pSourceName == NULL || pwfeSource == NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }
    
    m_spIO.Assign(CreateCIO());
    if (m_spIO->Open(pSourceName, true) != ERROR_SUCCESS)
    {
        m_spIO.Delete();
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / int64(m_wfeSource.nBlockAlign);
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
    if (RIFFHeader.nBytes != (m_nFileBytes - sizeof(RIFF_HEADER)))
        return ERROR_INVALID_INPUT_FILE;

    // read the AIFF header
    #pragma pack(push, 1)
    struct AIFF_HEADER
    {
        char cAIFF[4];
        char cCOMM[4];
        int32 nSize;
        int16 nChannels;
        uint32 nFrames;
        int16 nSampleSize;
        char cSampleRate[6];
        char cExtra[4];
    };
    #pragma pack(pop)

    AIFF_HEADER AIFFHeader;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &AIFFHeader, sizeof(AIFFHeader)))
    AIFFHeader.nSize = Swap4Bytes(AIFFHeader.nSize);
    AIFFHeader.nChannels = Swap2Bytes(AIFFHeader.nChannels);
    AIFFHeader.nFrames = Swap4Bytes(AIFFHeader.nFrames);
    AIFFHeader.nSampleSize = Swap2Bytes(AIFFHeader.nSampleSize);
    uint32 nSampleRate = IEEE754ExtendedFloatToUINT32((unsigned char *) &AIFFHeader.cSampleRate[0]);

    // only support AIFF
    if ((AIFFHeader.cAIFF[0] != 'A') ||
        (AIFFHeader.cAIFF[1] != 'I') ||
        (AIFFHeader.cAIFF[2] != 'F') ||
        (AIFFHeader.cAIFF[3] != 'F'))
    {
        return ERROR_INVALID_INPUT_FILE;
    }

    // only support 8-bit, 16-bit, and 24-bit
    if ((AIFFHeader.nSampleSize != 8) && (AIFFHeader.nSampleSize != 16) && (AIFFHeader.nSampleSize != 24))
        return ERROR_INVALID_INPUT_FILE;

    m_nDataBytes = -1;
    while (true)
    {
        struct GenericRIFFChunkHeaderStruct
        {
            char            cChunkLabel[4];            // the label of the chunk (PCM data = 'data')
            uint32            nChunkLength;            // the length of the chunk
        };
        GenericRIFFChunkHeaderStruct Generic;
        RETURN_ON_ERROR(ReadSafe(m_spIO, &Generic, sizeof(Generic)))
        Generic.nChunkLength = Swap4Bytes(Generic.nChunkLength);
        
        if ((Generic.cChunkLabel[0] == 'S') && (Generic.cChunkLabel[1] == 'S') && (Generic.cChunkLabel[2] == 'N') && (Generic.cChunkLabel[3] == 'D'))
        {
            // read the SSND header
            struct SSNDHeader
            {
                uint32 offset;
                uint32 blocksize;
            };
            SSNDHeader Header;
            RETURN_ON_ERROR(ReadSafe(m_spIO, &Header, sizeof(Header)))
            m_nDataBytes = int64(Generic.nChunkLength) - 8;

            // check the size
            if (int64(m_nDataBytes / AIFFHeader.nFrames) != int64(AIFFHeader.nSampleSize * AIFFHeader.nChannels / 8))
                return ERROR_INVALID_INPUT_FILE;
            break;
        }
        m_spIO->SetSeekMethod(APE_FILE_CURRENT);
        m_spIO->SetSeekPosition(Generic.nChunkLength);
        m_spIO->PerformSeek();
    }

    // make sure we found the SSND header
    if (m_nDataBytes < 0)
        return ERROR_INVALID_INPUT_FILE;
    
    // copy the format information to the WAVEFORMATEX passed in
    FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, nSampleRate, AIFFHeader.nSampleSize, AIFFHeader.nChannels);

    // calculate the header and terminating data
    m_nHeaderBytes = (uint32) m_spIO->GetPosition();
    m_nTerminatingBytes = (uint32) (m_nFileBytes - (m_nHeaderBytes + m_nDataBytes));

    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CAIFFInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (m_wfeSource.wBitsPerSample == 16)
    {
        for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
        {
            unsigned char cTemp = pBuffer[(nSample * 2) + 0];
            pBuffer[(nSample * 2) + 0] = pBuffer[(nSample * 2) + 1];
            pBuffer[(nSample * 2) + 1] = cTemp;
        }
    }
    else if (m_wfeSource.wBitsPerSample == 24)
    {
        for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
        {
            unsigned char cTemp = pBuffer[(nSample * 3) + 0];
            pBuffer[(nSample * 3) + 0] = pBuffer[(nSample * 3) + 2];
            pBuffer[(nSample * 3) + 2] = cTemp;
        }
    }

    if (pBlocksRetrieved) *pBlocksRetrieved = (nBytesRead / m_wfeSource.nBlockAlign);

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

uint32 CAIFFInputSource::IEEE754ExtendedFloatToUINT32(unsigned char * buffer)
{
    unsigned long mantissa;
    unsigned long last = 0;
    unsigned char exp;

    uint32 n = *((uint32 *) (buffer + 2));
    n = Swap4Bytes(n);
    *((uint32*)(buffer + 2)) = n;

    mantissa = *((unsigned long*)(buffer + 2));
    exp = 30 - *(buffer + 1);
    while (exp--)
    {
        last = mantissa;
        mantissa >>= 1;
    }
    if (last & 0x00000001)
        mantissa++;

    return (mantissa);
}

/**************************************************************************************************
CW64InputSource - wraps working with W64 files
**************************************************************************************************/
CW64InputSource::CW64InputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    memset(&m_wfeSource, 0, sizeof(m_wfeSource));

    if (pSourceName == NULL || pwfeSource == NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    m_spIO.Assign(CreateCIO());
    if (m_spIO->Open(pSourceName, true) != ERROR_SUCCESS)
    {
        m_spIO.Delete();
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / int64(m_wfeSource.nBlockAlign);
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
    static const GUID guidRIFF = { 0x66666972, 0x912E, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00 };
    static const GUID guidWAVE = { 0x65766177, 0xACF3, 0x11D3, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A };
    static const GUID guidDATA = { 0x61746164, 0xACF3, 0x11D3, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A };
    static const GUID guidFMT = { 0x20746D66, 0xACF3, 0x11D3, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A };
    bool bReadMetadataChunks = false;

    // read the riff header
    bool bDataChunkRead = false;
    bool bFormatChunkRead = false;
    W64ChunkHeader RIFFHeader;
    unsigned int nBytesRead = 0;
    m_nFileBytes = m_spIO->GetSize();

    m_spIO->Read(&RIFFHeader, sizeof(RIFFHeader), &nBytesRead);
    if ((memcmp(&RIFFHeader.guidIdentifier, &guidRIFF, sizeof(GUID)) == 0) && (RIFFHeader.nBytes == uint64(m_nFileBytes)))
    {
        // read and verify the wave data type header
        GUID DataHeader;
        nBytesRead = m_spIO->Read(&DataHeader, sizeof(DataHeader), &nBytesRead);
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
                int64 nChunkRemainingBytes = Header.nBytes - sizeof(Header);
                if ((m_spIO->GetPosition() + nChunkRemainingBytes) > m_nFileBytes)
                    break;

                // switched based on the chunk type
                if ((memcmp(&Header.guidIdentifier, &guidFMT, sizeof(GUID)) == 0) &&
                    (nChunkRemainingBytes >= sizeof(WAVFormatChunkData)))
                {
                    // read data
                    WAVFormatChunkData Data;
                    m_spIO->Read(&Data, sizeof(Data), &nBytesRead);
                    if (nBytesRead != sizeof(Data))
                        break;

                    // skip the rest
                    m_spIO->SetSeekMethod(APE_FILE_CURRENT);
                    m_spIO->SetSeekPosition(Align(nChunkRemainingBytes, 8) - sizeof(Data));
                    m_spIO->PerformSeek();

                    // verify the format (must be WAVE_FORMAT_PCM)
                    if (Data.nFormatTag != WAVE_FORMAT_PCM)
                    {
                        break;
                    }

                    // copy information over for internal storage
                    // may want to error check this header (bad avg bytes per sec, bad format, bad block align, etc...)
                    FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, Data.nSamplesPerSecond, Data.nBitsPerSample, Data.nChannels);
                    
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
                    m_nHeaderBytes = (uint32) m_spIO->GetPosition();

                    bDataChunkRead = true;

                    // short circuit if we don't need metadata
                    if (!bReadMetadataChunks && (bFormatChunkRead && bDataChunkRead))
                        break;

                    // move to the end of WAVEFORM data, so we can read other chunks behind it (if necessary)
                    m_spIO->SetSeekMethod(APE_FILE_CURRENT);
                    m_spIO->SetSeekPosition(Align(nChunkRemainingBytes, 8));
                    m_spIO->PerformSeek();
                }
                else
                {
                    m_spIO->SetSeekMethod(APE_FILE_CURRENT);
                    m_spIO->SetSeekPosition(Align(nChunkRemainingBytes, 8));
                    m_spIO->PerformSeek();
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
        m_nTerminatingBytes = (uint32) (m_nFileBytes - m_nDataBytes - m_nHeaderBytes);

        // we're valid if we make it this far
        m_bIsValid = true;
    }

    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CW64InputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (pBlocksRetrieved) *pBlocksRetrieved = (nBytesRead / m_wfeSource.nBlockAlign);

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

/**************************************************************************************************
CSNDInputSource - wraps working with SND files
**************************************************************************************************/
CSNDInputSource::CSNDInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode, int32 * pFlags)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    memset(&m_wfeSource, 0, sizeof(m_wfeSource));
    m_bBigEndian = false;

    if (pSourceName == NULL || pwfeSource == NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }

    m_spIO.Assign(CreateCIO());
    if (m_spIO->Open(pSourceName, true) != ERROR_SUCCESS)
    {
        m_spIO.Delete();
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    int nResult = AnalyzeSource(pFlags);
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / int64(m_wfeSource.nBlockAlign);
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
        uint32 m_nMagicNumber;
        uint32 m_nDataOffset;
        uint32 m_nDataSize;
        uint32 m_nEncoding;
        uint32 m_nSampleRate;
        uint32 m_nChannels;
    };
    CAUHeader Header = { 0 };
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
                m_nDataBytes = ape_min((int64) Header.m_nDataSize, m_nDataBytes);
            m_nTerminatingBytes = (uint32) (m_nFileBytes - m_nHeaderBytes - m_nDataBytes);

            // set format
            if (Header.m_nEncoding == 1)
            {
                // 8-bit mulaw
                // not supported
            }
            else if (Header.m_nEncoding == 2)
            {
                // 8-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, Header.m_nSampleRate, 8, Header.m_nChannels);
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 3)
            {
                // 16-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, Header.m_nSampleRate, 16, Header.m_nChannels);
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 4)
            {
                // 24-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, Header.m_nSampleRate, 24, Header.m_nChannels);
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 5)
            {
                // 32-bit PCM
                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, Header.m_nSampleRate, 32, Header.m_nChannels);
                bSupportedFormat = true;
            }
            else if (Header.m_nEncoding == 6)
            {
                // 32-bit float
                // not supported
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
    m_spIO->SetSeekMethod(APE_FILE_BEGIN);
    m_spIO->SetSeekPosition(m_nHeaderBytes);
    m_spIO->PerformSeek();

    // update flags
    *pFlags |= MAC_FORMAT_FLAG_SND;
    if (m_bBigEndian)
        *pFlags |= MAC_FORMAT_FLAG_BIG_ENDIAN;

    // we made it this far, everything must be cool
    return bIsValid ? ERROR_SUCCESS : ERROR_UNDEFINED;
}

int CSNDInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (m_bBigEndian)
    {
        if (m_wfeSource.wBitsPerSample == 16)
        {
            for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
            {
                unsigned char cTemp = pBuffer[(nSample * 2) + 0];
                pBuffer[(nSample * 2) + 0] = pBuffer[(nSample * 2) + 1];
                pBuffer[(nSample * 2) + 1] = cTemp;
            }
        }
        else if (m_wfeSource.wBitsPerSample == 24)
        {
            for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
            {
                unsigned char cTemp = pBuffer[(nSample * 3) + 0];
                pBuffer[(nSample * 3) + 0] = pBuffer[(nSample * 3) + 2];
                pBuffer[(nSample * 3) + 2] = cTemp;
            }
        }
        else if (m_wfeSource.wBitsPerSample == 32)
        {
            for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
            {
                uint32 nValue = *((uint32 *) &pBuffer[(nSample * 4) + 0]);
                uint32 nFlippedValue = (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
                *((uint32 *) &pBuffer[(nSample * 4) + 0]) = nFlippedValue;
            }
        }
    }

    if (pBlocksRetrieved) *pBlocksRetrieved = (nBytesRead / m_wfeSource.nBlockAlign);

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
CCAFInputSource::CCAFInputSource(const wchar_t * pSourceName, WAVEFORMATEX * pwfeSource, int64 * pTotalBlocks, int64 * pHeaderBytes, int64 * pTerminatingBytes, int * pErrorCode)
{
    m_bIsValid = false;
    m_nDataBytes = 0;
    m_nFileBytes = 0;
    m_nHeaderBytes = 0;
    m_nTerminatingBytes = 0;
    memset(&m_wfeSource, 0, sizeof(m_wfeSource));

    if (pSourceName == NULL || pwfeSource == NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return;
    }
    
    m_spIO.Assign(CreateCIO());
    if (m_spIO->Open(pSourceName, true) != ERROR_SUCCESS)
    {
        m_spIO.Delete();
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    int nResult = AnalyzeSource();
    if (nResult == ERROR_SUCCESS)
    {
        // fill in the parameters
        if (pwfeSource) memcpy(pwfeSource, &m_wfeSource, sizeof(WAVEFORMATEX));
        if (pTotalBlocks) *pTotalBlocks = m_nDataBytes / int64(m_wfeSource.nBlockAlign);
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
    struct CAFFileHeader {
        char cFileType[4]; // should equal 'caff'
        uint16 mFileVersion;
        uint16 mFileFlags;
    };
    CAFFileHeader Header;
    RETURN_ON_ERROR(ReadSafe(m_spIO, &Header, sizeof(Header)))
    Header.mFileVersion = Swap2Bytes(Header.mFileVersion);
    Header.mFileFlags = Swap2Bytes(Header.mFileFlags);

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
    struct CAFChunkHeader {
        char cChunkType[4];
        uint64 mChunkSize;
    };
    struct CAFAudioFormat {
        uint64 mSampleRate;
        char cFormatID[4];
        uint32 mFormatFlags;
        uint32 mBytesPerPacket;
        uint32 mFramesPerPacket;
        uint32 mChannelsPerFrame;
        uint32 mBitsPerChannel;
    };
    #pragma pack(pop)

    bool bFoundDesc = false;
    while (true)
    {
        CAFChunkHeader Chunk;
        if (ReadSafe(m_spIO, &Chunk, sizeof(Chunk)) != ERROR_SUCCESS)
            return ERROR_INVALID_INPUT_FILE; // we read past the last chunk and didn't find the necessary chunks

        Chunk.mChunkSize = Swap8Bytes(Chunk.mChunkSize);

        if ((Chunk.cChunkType[0] == 'd') &&
            (Chunk.cChunkType[1] == 'e') &&
            (Chunk.cChunkType[2] == 's') &&
            (Chunk.cChunkType[3] == 'c'))
        {
            if (Chunk.mChunkSize == sizeof(CAFAudioFormat))
            {
                CAFAudioFormat AudioFormat;
                RETURN_ON_ERROR(ReadSafe(m_spIO, &AudioFormat, sizeof(AudioFormat)))

                if ((AudioFormat.cFormatID[0] != 'l') ||
                    (AudioFormat.cFormatID[1] != 'p') ||
                    (AudioFormat.cFormatID[2] != 'c') ||
                    (AudioFormat.cFormatID[3] != 'm'))
                {
                    return ERROR_INVALID_INPUT_FILE;
                }

                AudioFormat.mSampleRate = Swap8Bytes(AudioFormat.mSampleRate);
                double dSampleRate = *((double *) &AudioFormat.mSampleRate);
                AudioFormat.mBitsPerChannel = Swap4Bytes(AudioFormat.mBitsPerChannel);
                AudioFormat.mChannelsPerFrame = Swap4Bytes(AudioFormat.mChannelsPerFrame);

                // only support 8-bit, 16-bit, and 24-bit
                if ((AudioFormat.mBitsPerChannel != 8) && (AudioFormat.mBitsPerChannel != 16) && (AudioFormat.mBitsPerChannel != 24))
                    return ERROR_INVALID_INPUT_FILE;

                FillWaveFormatEx(&m_wfeSource, WAVE_FORMAT_PCM, int(dSampleRate), AudioFormat.mBitsPerChannel, AudioFormat.mChannelsPerFrame);
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
            m_nHeaderBytes = (uint32) m_spIO->GetPosition();

            // data bytes are this chunk
            m_nDataBytes = Chunk.mChunkSize;

            // terminating bytes are whatever is left
            m_nTerminatingBytes = (uint32) (m_nFileBytes - (m_nHeaderBytes + m_nDataBytes));

            // we made it this far, everything must be cool
            break;
        }
        else
        {
            // skip this chunk
            m_spIO->SetSeekPosition(Chunk.mChunkSize);
            m_spIO->SetSeekMethod(APE_FILE_CURRENT);
            m_spIO->PerformSeek();
        }
    }
    
    // we made it this far, everything must be cool
    return ERROR_SUCCESS;
}

int CCAFInputSource::GetData(unsigned char * pBuffer, int nBlocks, int * pBlocksRetrieved)
{
    if (!m_bIsValid) return ERROR_UNDEFINED;

    int nBytes = (m_wfeSource.nBlockAlign * nBlocks);
    unsigned int nBytesRead = 0;

    if (m_spIO->Read(pBuffer, nBytes, &nBytesRead) != ERROR_SUCCESS)
        return ERROR_IO_READ;

    if (m_wfeSource.wBitsPerSample == 16)
    {
        for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
        {
            unsigned char cTemp = pBuffer[(nSample * 2) + 0];
            pBuffer[(nSample * 2) + 0] = pBuffer[(nSample * 2) + 1];
            pBuffer[(nSample * 2) + 1] = cTemp;
        }
    }
    else if (m_wfeSource.wBitsPerSample == 24)
    {
        for (int nSample = 0; nSample < nBlocks * m_wfeSource.nChannels; nSample++)
        {
            unsigned char cTemp = pBuffer[(nSample * 3) + 0];
            pBuffer[(nSample * 3) + 0] = pBuffer[(nSample * 3) + 2];
            pBuffer[(nSample * 3) + 2] = cTemp;
        }
    }

    if (pBlocksRetrieved) *pBlocksRetrieved = (nBytesRead / m_wfeSource.nBlockAlign);

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

}