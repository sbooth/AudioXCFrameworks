#include "All.h"

#ifdef APE_BACKWARDS_COMPATIBILITY

#include "APEDecompressOld.h"
#include "../APEInfo.h"

namespace APE
{

CAPEDecompressOld::CAPEDecompressOld(int * pErrorCode, CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock)
{
    *pErrorCode = ERROR_SUCCESS;

    // open / analyze the file
    m_spAPEInfo.Assign(pAPEInfo);

    // version check (this implementation only works with 3.92 and earlier files)
    if (GetInfo(APE_INFO_FILE_VERSION) > 3920)
    {
        *pErrorCode = ERROR_UNDEFINED;
        return;
    }

    // create the buffer
    m_nBlockAlign = (int)GetInfo(APE_INFO_BLOCK_ALIGN);
    if ((m_nBlockAlign <= 0) || (m_nBlockAlign > 32))
    {
        *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return;
    }
    
    // initialize other stuff
    m_nBufferTail = 0;
    m_bDecompressorInitialized = false;
    m_nCurrentFrame = 0;
    m_nCurrentBlock = 0;
    
    // set the "real" start and finish blocks
    m_nStartBlock = (nStartBlock < 0) ? 0 : ape_min(nStartBlock, (int)GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_nFinishBlock = (nFinishBlock < 0) ? (int)GetInfo(APE_INFO_TOTAL_BLOCKS) : ape_min(nFinishBlock, (int)GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_bIsRanged = (m_nStartBlock != 0) || (m_nFinishBlock != (int)GetInfo(APE_INFO_TOTAL_BLOCKS));
}

CAPEDecompressOld::~CAPEDecompressOld()
{

}

int CAPEDecompressOld::InitializeDecompressor()
{
    // check if we have anything to do
    if (m_bDecompressorInitialized)
        return ERROR_SUCCESS;

    // initialize the decoder
    RETURN_ON_ERROR(m_UnMAC.Initialize(this))

    int64 nMaximumDecompressedFrameBytes = m_nBlockAlign * intn(GetInfo(APE_INFO_BLOCKS_PER_FRAME));
    int64 nTotalBufferBytes = ape_max(65536, (nMaximumDecompressedFrameBytes + 16) * 2);
    m_spBuffer.Assign(new char [(unsigned int) nTotalBufferBytes], true);
    if (m_spBuffer == NULL)
        return ERROR_INSUFFICIENT_MEMORY;

    // update the initialized flag
    m_bDecompressorInitialized = true;

    // seek to the beginning
    return Seek(0);
}

int CAPEDecompressOld::GetData(char * pBuffer, int64 nBlocks, int64 * pBlocksRetrieved)
{
    if (pBlocksRetrieved) *pBlocksRetrieved = 0;

    RETURN_ON_ERROR(InitializeDecompressor())
    
    // cap
    int64 nBlocksUntilFinish = m_nFinishBlock - m_nCurrentBlock;
    nBlocks = ape_min(nBlocks, nBlocksUntilFinish);

    int64 nBlocksRetrieved = 0;

    // fulfill as much of the request as possible
    int64 nTotalBytesNeeded = nBlocks * m_nBlockAlign;
    int64 nBytesLeft = nTotalBytesNeeded;
    int64 nBlocksDecoded = 1;

    while (nBytesLeft > 0 && nBlocksDecoded > 0)
    {
        // empty the buffer
        int64 nBytesAvailable = m_nBufferTail;
        int64 nIntialBytes = ape_min(nBytesLeft, nBytesAvailable);
        if (nIntialBytes > 0)
        {
            memcpy(&pBuffer[nTotalBytesNeeded - nBytesLeft], &m_spBuffer[0], size_t(nIntialBytes));
            
            if ((m_nBufferTail - nIntialBytes) > 0)
                memmove(&m_spBuffer[0], &m_spBuffer[nIntialBytes], size_t(m_nBufferTail - nIntialBytes));
                
            nBytesLeft -= nIntialBytes;
            m_nBufferTail -= nIntialBytes;

        }

        // decode more
        if (nBytesLeft > 0)
        {
            nBlocksDecoded = m_UnMAC.DecompressFrame((unsigned char *) &m_spBuffer[m_nBufferTail], (int32) m_nCurrentFrame++);
            if (nBlocksDecoded == -1)
            {
                return ERROR_UNDEFINED;
            }
            m_nBufferTail += (nBlocksDecoded * m_nBlockAlign);
        }
    }
    
    nBlocksRetrieved = (nTotalBytesNeeded - nBytesLeft) / m_nBlockAlign;

    // update the position
    m_nCurrentBlock += nBlocksRetrieved;

    if (pBlocksRetrieved) *pBlocksRetrieved = nBlocksRetrieved;
    
    return ERROR_SUCCESS;
}

int CAPEDecompressOld::Seek(int64 nBlockOffset)
{
    RETURN_ON_ERROR(InitializeDecompressor())

    // use the offset
    nBlockOffset += m_nStartBlock;
    
    // cap (to prevent seeking too far)
    if (nBlockOffset >= m_nFinishBlock)
        nBlockOffset = m_nFinishBlock - 1;
    if (nBlockOffset < m_nStartBlock)
        nBlockOffset = m_nStartBlock;

    // flush the buffer
    m_nBufferTail = 0;
    
    // seek to the perfect location
    int64 nBaseFrame = nBlockOffset / intn(GetInfo(APE_INFO_BLOCKS_PER_FRAME));
    int64 nBlocksToSkip = nBlockOffset % intn(GetInfo(APE_INFO_BLOCKS_PER_FRAME));
    int64 nBytesToSkip = nBlocksToSkip * m_nBlockAlign;
        
    // skip necessary blocks
    int64 nMaximumDecompressedFrameBytes = m_nBlockAlign * (int)GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    CSmartPtr<char> spTempBuffer;
    spTempBuffer.Assign(new char[(unsigned int) (nMaximumDecompressedFrameBytes + 16)], true);
    ZeroMemory(spTempBuffer.GetPtr(), size_t(nMaximumDecompressedFrameBytes + 16));
    
    m_nCurrentFrame = nBaseFrame;

    intn nBlocksDecoded = m_UnMAC.DecompressFrame((unsigned char *)spTempBuffer.GetPtr(), (int32) m_nCurrentFrame++);
    
    if (nBlocksDecoded == -1)
    {
        return ERROR_UNDEFINED;
    }
    
    int64 nBytesToKeep = (nBlocksDecoded * m_nBlockAlign) - nBytesToSkip;
    memcpy(&m_spBuffer[m_nBufferTail], &spTempBuffer[nBytesToSkip], size_t(nBytesToKeep));
    m_nBufferTail += nBytesToKeep;
    
    m_nCurrentBlock = nBlockOffset;
    
    return ERROR_SUCCESS;
}

int64 CAPEDecompressOld::GetInfo(APE_DECOMPRESS_FIELDS Field, int64 nParam1, int64 nParam2)
{
    int64 nRetVal = 0;
    bool bHandled = true;

    switch (Field)
    {
    case APE_DECOMPRESS_CURRENT_BLOCK:
        nRetVal = m_nCurrentBlock - m_nStartBlock;
        break;
    case APE_DECOMPRESS_CURRENT_MS:
    {
        int64 nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nRetVal = int64((double(m_nCurrentBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_TOTAL_BLOCKS:
        nRetVal = m_nFinishBlock - m_nStartBlock;
        break;
    case APE_DECOMPRESS_LENGTH_MS:
    {
        int64 nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
            nRetVal = int64((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(nSampleRate));
        break;
    }
    case APE_DECOMPRESS_CURRENT_BITRATE:
        nRetVal = GetInfo(APE_INFO_FRAME_BITRATE, intn(m_nCurrentFrame));
        break;
    case APE_DECOMPRESS_AVERAGE_BITRATE:
    {
        if (m_bIsRanged)
        {
            // figure the frame range
            const int64 nBlocksPerFrame = GetInfo(APE_INFO_BLOCKS_PER_FRAME);
            int64 nStartFrame = m_nStartBlock / nBlocksPerFrame;
            int64 nFinishFrame = (m_nFinishBlock + nBlocksPerFrame - 1) / nBlocksPerFrame;

            // get the number of bytes in the first and last frame
            int64 nTotalBytes = (GetInfo(APE_INFO_FRAME_BYTES, intn(nStartFrame)) * (m_nStartBlock % nBlocksPerFrame)) / nBlocksPerFrame;
            if (nFinishFrame != nStartFrame)
                nTotalBytes += (GetInfo(APE_INFO_FRAME_BYTES, intn(nFinishFrame)) * (m_nFinishBlock % nBlocksPerFrame)) / nBlocksPerFrame;

            // get the number of bytes in between
            const int64 nTotalFrames = GetInfo(APE_INFO_TOTAL_FRAMES);
            for (int64 nFrame = nStartFrame + 1; (nFrame < nFinishFrame) && (nFrame < nTotalFrames); nFrame++)
                nTotalBytes += GetInfo(APE_INFO_FRAME_BYTES, nFrame);

            // figure the bitrate
            int64 nTotalMS = int64((double(m_nFinishBlock - m_nStartBlock) * double(1000)) / double(GetInfo(APE_INFO_SAMPLE_RATE)));
            if (nTotalMS != 0)
                nRetVal = (nTotalBytes * 8) / nTotalMS;
        }
        else
        {
            nRetVal = GetInfo(APE_INFO_AVERAGE_BITRATE);
        }

        break;
    }
    default:
        bHandled = false;
    }

    if (!bHandled && m_bIsRanged)
    {
        bHandled = true;

        switch (Field)
        {
        case APE_INFO_WAV_HEADER_BYTES:
            nRetVal = sizeof(WAVE_HEADER);
            break;
        case APE_INFO_WAV_HEADER_DATA:
        {
            char * pBuffer = (char *) nParam1;
            int nMaxBytes = (int)nParam2;
            
            if (sizeof(WAVE_HEADER) > (size_t) nMaxBytes)
            {
                nRetVal = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; GetInfo(APE_INFO_WAVEFORMATEX, (int64) &wfeFormat, 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader, 
                    (m_nFinishBlock - m_nStartBlock) * intn(GetInfo(APE_INFO_BLOCK_ALIGN)), 
                    &wfeFormat, 0);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nRetVal = 0;
            }
            break;
        }
        case APE_INFO_WAV_TERMINATING_BYTES:
            nRetVal = 0;
            break;
        case APE_INFO_WAV_TERMINATING_DATA:
            nRetVal = 0;
            break;
        default:
            bHandled = false;
        }
    }

    if (!bHandled)
        nRetVal = m_spAPEInfo->GetInfo(Field, nParam1, nParam2);

    return nRetVal;
}

}

#endif // #ifdef APE_BACKWARDS_COMPATIBILITY
