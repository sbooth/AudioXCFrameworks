/**************************************************************************************************
CAPEInfo:
    -a class to make working with APE files and getting information about them simple
**************************************************************************************************/
#include "All.h"
#include "APEInfo.h"
#include "IO.h"
#include "APEHeader.h"
#include "GlobalFunctions.h"
#include "WholeFileIO.h"

namespace APE
{

/**************************************************************************************************
APE_FILE_INFO
**************************************************************************************************/
APE_FILE_INFO::APE_FILE_INFO()
{
    nVersion = 0;
    nCompressionLevel = 0;
    nFormatFlags = 0;
    nTotalFrames = 0;
    nBlocksPerFrame = 0;
    nFinalFrameBlocks = 0;
    nChannels = 0;
    nSampleRate = 0;
    nBitsPerSample = 0;
    nBytesPerSample = 0;
    nBlockAlign = 0;
    nWAVTerminatingBytes = 0;
    nWAVHeaderBytes = 0;
    nWAVDataBytes = 0;
    nWAVTotalBytes = 0;
    nAPETotalBytes = 0;
    nTotalBlocks = 0;
    nLengthMS = 0;
    nAverageBitrate = 0;
    nDecompressedBitrate = 0;
    nJunkHeaderBytes = 0;
    nSeekTableElements = 0;
    nMD5Invalid = 0;
}

APE_FILE_INFO::~APE_FILE_INFO()
{
}

/**************************************************************************************************
Construction
**************************************************************************************************/
CAPEInfo::CAPEInfo(int * pErrorCode, const wchar_t * pFilename, CAPETag * pTag, bool bAPL, bool bReadOnly, bool bAnalyzeTagNow, bool bReadWholeFile)
{
    *pErrorCode = ERROR_SUCCESS;
    CloseFile();

    // store the APL status
    m_bAPL = bAPL;

    // open the file
    m_spIO.Assign(CreateCIO());

    *pErrorCode = m_spIO->Open(pFilename, bReadOnly);
    if (*pErrorCode != ERROR_SUCCESS)
    {
        CloseFile();
        return;
    }

    // flag to read the whole file if specified
    if (bReadWholeFile)
    {
        // get size
        const int64 nSize = m_spIO->GetSize();

        // only create if we're less than 200 MB
        if (nSize < (APE_BYTES_IN_MEGABYTE * 200))
        {
            CWholeFileIO * pWholeFile = CreateWholeFileIO(m_spIO, nSize);
            if (pWholeFile != APE_NULL)
            {
                m_spIO.SetDelete(false);
                m_spIO.Assign(pWholeFile);
                m_spIO.SetDelete(true);
            }
        }
    }

    // get the file information
    if (GetFileInformation() != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // get the tag (do this second so that we don't do it on failure)
    if (pTag == APE_NULL)
    {
        // we don't want to analyze right away for non-local files
        // since a single I/O object is shared, we can't tag and read at the same time (i.e. in multiple threads)
        if (StringIsEqual(pFilename, L"http://", false, 7) ||
            StringIsEqual(pFilename, L"m01p://", false, 7) ||
            StringIsEqual(pFilename, L"https://", false, 8) ||
            StringIsEqual(pFilename, L"m01ps://", false, 8))
        {
            bAnalyzeTagNow = false;
        }
        m_spAPETag.Assign(new CAPETag(m_spIO, bAnalyzeTagNow, GetCheckForID3v1()));
    }
    else
    {
        m_spAPETag.Assign(pTag);
    }

    // update
    CheckHeaderInformation();
}

CAPEInfo::CAPEInfo(int * pErrorCode, CIO * pIO, CAPETag * pTag)
{
    m_bAPL = false;
    *pErrorCode = ERROR_SUCCESS;
    CloseFile();

    m_spIO.Assign(pIO, false, false);

    // get the file information
    if (GetFileInformation() != 0)
    {
        CloseFile();
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }

    // get the tag (do this second so that we don't do it on failure)
    if (pTag == APE_NULL)
        m_spAPETag.Assign(new CAPETag(m_spIO, true, GetCheckForID3v1()));
    else
        m_spAPETag.Assign(pTag);

    // update
    CheckHeaderInformation();
}

/**************************************************************************************************
Destruction
**************************************************************************************************/
CAPEInfo::~CAPEInfo()
{
    CloseFile();
}

/**************************************************************************************************
Close the file
**************************************************************************************************/
int CAPEInfo::CloseFile()
{
    m_spIO.Delete();
    m_APEFileInfo.spWaveHeaderData.Delete();
    m_APEFileInfo.spSeekByteTable64.Delete();
    m_APEFileInfo.spAPEDescriptor.Delete();
#ifdef APE_BACKWARDS_COMPATIBILITY
    m_APEFileInfo.spSeekBitTable.Delete();
#endif
    m_spAPETag.Delete();

    // re-initialize variables
    m_APEFileInfo.nSeekTableElements = 0;
    m_bHasFileInformationLoaded = false;

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Performs sanity checks on all of the header data.
**************************************************************************************************/
int CAPEInfo::CheckHeaderInformation()
{
    // Fixes a bug with MAC 3.99 where conversion from APE to APE could include the file tag
    // as part of the WAV terminating data. This sanity check fixes the problem.
    if ((m_APEFileInfo.spAPEDescriptor != APE_NULL) &&
        (m_APEFileInfo.spAPEDescriptor->nTerminatingDataBytes > 0))
    {
        int64 nFileBytes = m_spIO->GetSize();
        if (nFileBytes > 0)
        {
            nFileBytes -= m_spAPETag->GetTagBytes();
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nDescriptorBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nHeaderBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nSeekTableBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nHeaderDataBytes;
            nFileBytes -= m_APEFileInfo.spAPEDescriptor->nAPEFrameDataBytes;
            if (nFileBytes < m_APEFileInfo.nWAVTerminatingBytes)
            {
                m_APEFileInfo.nMD5Invalid = true;
                m_APEFileInfo.nWAVTerminatingBytes = static_cast<uint32>(nFileBytes);
                m_APEFileInfo.spAPEDescriptor->nTerminatingDataBytes = static_cast<uint32>(nFileBytes);
            }
        }
    }

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Get the file information about the file
**************************************************************************************************/
int CAPEInfo::GetFileInformation()
{
    // quit if there is no simple file
    if (m_spIO == APE_NULL) { return ERROR_UNDEFINED; }

    // quit if the file information has already been loaded
    if (m_bHasFileInformationLoaded) { return ERROR_SUCCESS; }

    // use a CAPEHeader class to help us analyze the file
    int nResult = ERROR_UNDEFINED;
    try
    {
        CAPEHeader APEHeader(m_spIO);
        nResult = APEHeader.Analyze(&m_APEFileInfo);
    }
    catch (...)
    {
        nResult = ERROR_UNDEFINED;
    }

    // update our internal state
    if (nResult == ERROR_SUCCESS)
        m_bHasFileInformationLoaded = true;

    // return
    return nResult;
}

/**************************************************************************************************
Primary query function
**************************************************************************************************/
int64 CAPEInfo::GetInfo(IAPEDecompress::APE_DECOMPRESS_FIELDS Field, int64 nParam1, int64 nParam2)
{
    int64 nResult = ERROR_UNDEFINED;

    switch (Field)
    {
    case IAPEDecompress::APE_INFO_FILE_VERSION:
        nResult = m_APEFileInfo.nVersion;
        break;
    case IAPEDecompress::APE_INFO_COMPRESSION_LEVEL:
        nResult = m_APEFileInfo.nCompressionLevel;
        break;
    case IAPEDecompress::APE_INFO_FORMAT_FLAGS:
        nResult = m_APEFileInfo.nFormatFlags;
        break;
    case IAPEDecompress::APE_INFO_SAMPLE_RATE:
        nResult = m_APEFileInfo.nSampleRate;
        break;
    case IAPEDecompress::APE_INFO_BITS_PER_SAMPLE:
        nResult = m_APEFileInfo.nBitsPerSample;
        break;
    case IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE:
        nResult = m_APEFileInfo.nBytesPerSample;
        break;
    case IAPEDecompress::APE_INFO_CHANNELS:
        nResult = m_APEFileInfo.nChannels;
        break;
    case IAPEDecompress::APE_INFO_BLOCK_ALIGN:
        nResult = m_APEFileInfo.nBlockAlign;
        break;
    case IAPEDecompress::APE_INFO_BLOCKS_PER_FRAME:
        nResult = m_APEFileInfo.nBlocksPerFrame;
        break;
    case IAPEDecompress::APE_INFO_FINAL_FRAME_BLOCKS:
        nResult = m_APEFileInfo.nFinalFrameBlocks;
        break;
    case IAPEDecompress::APE_INFO_TOTAL_FRAMES:
        nResult = m_APEFileInfo.nTotalFrames;
        break;
    case IAPEDecompress::APE_INFO_WAV_HEADER_BYTES:
        nResult = m_APEFileInfo.nWAVHeaderBytes;
        break;
    case IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES:
        nResult = m_APEFileInfo.nWAVTerminatingBytes;
        break;
    case IAPEDecompress::APE_INFO_WAV_DATA_BYTES:
        nResult = m_APEFileInfo.nWAVDataBytes;
        break;
    case IAPEDecompress::APE_INFO_WAV_TOTAL_BYTES:
        nResult = m_APEFileInfo.nWAVTotalBytes;
        break;
    case IAPEDecompress::APE_INFO_APE_TOTAL_BYTES:
        nResult = m_APEFileInfo.nAPETotalBytes;
        break;
    case IAPEDecompress::APE_INFO_TOTAL_BLOCKS:
        nResult = m_APEFileInfo.nTotalBlocks;
        break;
    case IAPEDecompress::APE_INFO_LENGTH_MS:
        nResult = m_APEFileInfo.nLengthMS;
        break;
    case IAPEDecompress::APE_INFO_AVERAGE_BITRATE:
        nResult = m_APEFileInfo.nAverageBitrate;
        break;
    case IAPEDecompress::APE_INFO_FRAME_BITRATE:
    {
        const int64 nFrame = nParam1;

        nResult = 0;

        const int64 nFrameBytes = GetInfo(IAPEDecompress::APE_INFO_FRAME_BYTES, nFrame);
        const int64 nFrameBlocks = GetInfo(IAPEDecompress::APE_INFO_FRAME_BLOCKS, nFrame);
        if ((nFrameBytes > 0) && (nFrameBlocks > 0) && m_APEFileInfo.nSampleRate > 0)
        {
            const int64 nFrameMS = (nFrameBlocks * 1000) / m_APEFileInfo.nSampleRate;
            if (nFrameMS != 0)
            {
                nResult = (nFrameBytes * 8) / nFrameMS;
            }
        }
        break;
    }
    case IAPEDecompress::APE_INFO_DECOMPRESSED_BITRATE:
        nResult = m_APEFileInfo.nDecompressedBitrate;
        break;
    case IAPEDecompress::APE_INFO_PEAK_LEVEL:
        nResult = ERROR_UNDEFINED; // no longer supported
        break;
    case IAPEDecompress::APE_INFO_SEEK_BIT:
    {
        const int64 nFrame = nParam1;
        if (GET_FRAMES_START_ON_BYTES_BOUNDARIES(this))
        {
            nResult = 0;
        }
        else
        {
            if ((nFrame < 0) || (static_cast<uint32>(nFrame) >= m_APEFileInfo.nTotalFrames))
                nResult = 0;
            else
#ifdef APE_BACKWARDS_COMPATIBILITY
                nResult = m_APEFileInfo.spSeekBitTable[nFrame];
#else
                nResult = 0;
#endif
        }
        break;
    }
    case IAPEDecompress::APE_INFO_SEEK_BYTE:
    {
        const int64 nFrame = nParam1;
        if ((nFrame < 0) || (static_cast<uint32>(nFrame) >= m_APEFileInfo.nTotalFrames))
            nResult = 0;
        else if (m_APEFileInfo.spSeekByteTable64 != APE_NULL)
            nResult = m_APEFileInfo.spSeekByteTable64[nFrame] + m_APEFileInfo.nJunkHeaderBytes;
        break;
    }
    case IAPEDecompress::APE_INFO_WAV_HEADER_DATA:
    {
        char * pBuffer = reinterpret_cast<char *>(nParam1);
        const int64 nMaxBytes = nParam2;

        if (m_APEFileInfo.nFormatFlags & APE_FORMAT_FLAG_CREATE_WAV_HEADER)
        {
            if (m_APEFileInfo.nWAVDataBytes >= (APE_BYTES_IN_GIGABYTE * 4))
            {
                if (static_cast<APE::int64>(sizeof(RF64_HEADER)) > nMaxBytes)
                {
                    nResult = ERROR_UNDEFINED;
                }
                else
                {
                    WAVEFORMATEX wfeFormat; APE_CLEAR(wfeFormat); GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&wfeFormat), 0);
                    RF64_HEADER WAVHeader; FillRF64Header(&WAVHeader, m_APEFileInfo.nWAVDataBytes, &wfeFormat);
                    memcpy(pBuffer, &WAVHeader, sizeof(RF64_HEADER));
                    nResult = ERROR_SUCCESS;
                }

            }
            else
            {
                if (static_cast<APE::int64>(sizeof(WAVE_HEADER)) > nMaxBytes)
                {
                    nResult = ERROR_UNDEFINED;
                }
                else
                {
                    WAVEFORMATEX wfeFormat; APE_CLEAR(wfeFormat); GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&wfeFormat), 0);
                    WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader, static_cast<int64>(m_APEFileInfo.nWAVDataBytes), &wfeFormat,
                        static_cast<intn>(m_APEFileInfo.nWAVTerminatingBytes));
                    memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                    nResult = ERROR_SUCCESS;
                }
            }
        }
        else
        {
            if (m_APEFileInfo.nWAVHeaderBytes > nMaxBytes)
            {
                nResult = ERROR_UNDEFINED;
            }
            else
            {
                if ((m_APEFileInfo.spWaveHeaderData != APE_NULL) && (m_APEFileInfo.nWAVHeaderBytes > 0))
                    memcpy(pBuffer, m_APEFileInfo.spWaveHeaderData, static_cast<size_t>(m_APEFileInfo.nWAVHeaderBytes));
                nResult = ERROR_SUCCESS;
            }
        }
        break;
    }
    case IAPEDecompress::APE_INFO_WAV_TERMINATING_DATA:
    {
        char * pBuffer = reinterpret_cast<char *>(nParam1);
        const int64 nMaxBytes = nParam2;

        if (m_APEFileInfo.nWAVTerminatingBytes > static_cast<uint32>(nMaxBytes))
        {
            nResult = ERROR_UNDEFINED;
        }
        else
        {
            if (m_APEFileInfo.nWAVTerminatingBytes > 0)
            {
                // variables
                const int64 nOriginalFileLocation = m_spIO->GetPosition();
                unsigned int nBytesRead = 0;

                // check for a tag
                m_spIO->Seek(-(static_cast<int64>(m_spAPETag->GetTagBytes()) + static_cast<int64>(m_APEFileInfo.nWAVTerminatingBytes)), SeekFileEnd);
                m_spIO->Read(pBuffer, m_APEFileInfo.nWAVTerminatingBytes, &nBytesRead);

                // restore the file pointer
                m_spIO->Seek(nOriginalFileLocation, SeekFileBegin);
            }
            nResult = ERROR_SUCCESS;
        }
        break;
    }
    case IAPEDecompress::APE_INFO_WAVEFORMATEX:
    {
        WAVEFORMATEX * pWaveFormatEx = reinterpret_cast<WAVEFORMATEX *>(nParam1);
        FillWaveFormatEx(pWaveFormatEx, WAVE_FORMAT_PCM, m_APEFileInfo.nSampleRate, m_APEFileInfo.nBitsPerSample, m_APEFileInfo.nChannels);
        nResult = ERROR_SUCCESS;
        break;
    }
    case IAPEDecompress::APE_INFO_IO_SOURCE:
        nResult = POINTER_TO_INT64(m_spIO.GetPtr());
        break;
    case IAPEDecompress::APE_INFO_FRAME_BYTES:
    {
        const int64 nFrame = nParam1;

        // bound-check the frame index
        if ((nFrame < 0) || (static_cast<uint32>(nFrame) >= m_APEFileInfo.nTotalFrames))
        {
            nResult = ERROR_UNDEFINED;
        }
        else
        {
            if (static_cast<uint32>(nFrame) != (m_APEFileInfo.nTotalFrames - 1))
                nResult = GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, nFrame + 1) - GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, nFrame);
            else
                nResult = static_cast<int64>(m_spIO->GetSize() - static_cast<int64>(m_spAPETag->GetTagBytes()) - static_cast<int64>(m_APEFileInfo.nWAVTerminatingBytes) - static_cast<int64>(GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, nFrame)));
        }
        break;
    }
    case IAPEDecompress::APE_INFO_FRAME_BLOCKS:
    {
        const int64 nFrame = nParam1;

        // bound-check the frame index
        if ((nFrame < 0) || (static_cast<uint32>(nFrame) >= m_APEFileInfo.nTotalFrames))
        {
            nResult = ERROR_UNDEFINED;
        }
        else
        {
            if (static_cast<uint32>(nFrame) != (m_APEFileInfo.nTotalFrames - 1))
                nResult = m_APEFileInfo.nBlocksPerFrame;
            else
                nResult = m_APEFileInfo.nFinalFrameBlocks;
        }
        break;
    }
    case IAPEDecompress::APE_INFO_TAG:
        nResult = POINTER_TO_INT64(static_cast<IAPETag *>(m_spAPETag.GetPtr()));
        break;
    case IAPEDecompress::APE_INFO_APL:
        nResult = static_cast<int64>(m_bAPL);
        break;
    case IAPEDecompress::APE_INFO_MD5:
        if (m_APEFileInfo.spAPEDescriptor != APE_NULL)
        {
            char * pBuffer = reinterpret_cast<char *>(nParam1);
            memcpy(pBuffer, m_APEFileInfo.spAPEDescriptor->cFileMD5, sizeof(m_APEFileInfo.spAPEDescriptor->cFileMD5));
            nResult = ERROR_SUCCESS;
        }
        break;
    case IAPEDecompress::APE_INFO_MD5_MATCHES:
        nResult = ERROR_INVALID_CHECKSUM;
        if (m_APEFileInfo.spAPEDescriptor != APE_NULL)
        {
            char * pBuffer = reinterpret_cast<char *>(nParam1);
            if (memcmp(pBuffer, m_APEFileInfo.spAPEDescriptor->cFileMD5, 16) == 0)
                nResult = ERROR_SUCCESS;
        }
        break;
    case IAPEDecompress::APE_INTERNAL_INFO:
        nResult = POINTER_TO_INT64(&m_APEFileInfo);
        break;
    case IAPEDecompress::APE_DECOMPRESS_CURRENT_BLOCK:
    case IAPEDecompress::APE_DECOMPRESS_CURRENT_MS:
    case IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS:
    case IAPEDecompress::APE_DECOMPRESS_LENGTH_MS:
    case IAPEDecompress::APE_DECOMPRESS_CURRENT_BITRATE:
    case IAPEDecompress::APE_DECOMPRESS_AVERAGE_BITRATE:
    case IAPEDecompress::APE_DECOMPRESS_CURRENT_FRAME:
        // all other conditions to prevent compiler warnings (4061, 4062, and Clang)
        break;
    }

    return nResult;
}

bool CAPEInfo::GetCheckForID3v1()
{
    // somebody sent me a WAV compressed with terminating data that was an ID3v1 tag
    // since the terminating data is at the end just like the tag, that made a mess
    // it was a legacy encode so it doesn't store the total number of APE bytes
    // so the only work around I could figure out was to simply not check for ID3v1 tags
    // if the terminating data is exactly the size of a tag
    bool bCheckForID3 = (m_APEFileInfo.nWAVTerminatingBytes != ID3_TAG_BYTES);
    if ((bCheckForID3 == false) && (m_APEFileInfo.spAPEDescriptor != APE_NULL))
    {
        // if the amount of data after everything is over 128 bytes, it could be a valid ID3 tag
        // this can only be determined on newer encodes since spAPEDescriptor is NULL for older encodes
        int64 nEndOfFile = static_cast<int64>(m_APEFileInfo.nJunkHeaderBytes) + static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nDescriptorBytes) + static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nHeaderBytes) + static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nSeekTableBytes) + static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nHeaderDataBytes);
        nEndOfFile += (static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nAPEFrameDataBytesHigh) << 32) + static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nAPEFrameDataBytes) + static_cast<int64>(m_APEFileInfo.spAPEDescriptor->nTerminatingDataBytes);
        const int64 nSize = m_spIO->GetSize();
        const int64 nExtra = nSize - nEndOfFile;
        if (nExtra >= ID3_TAG_BYTES)
        {
            // check for greater than or equal because some files have both tags so there would be even more extra than just the ID3v1 tag
            bCheckForID3 = true;
        }
    }

    return bCheckForID3;
}

}
