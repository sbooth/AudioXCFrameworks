#include "All.h"
#define APE_ENABLE_CIRCLE_BUFFER_WRITE
#include "APEDecompress.h"
#include "APEInfo.h"
#include "NewPredictor.h"
#include "FloatTransform.h"

namespace APE
{

#define DECODE_BLOCK_SIZE        4096

CAPEDecompress::CAPEDecompress(int * pErrorCode, CAPEInfo * pAPEInfo, int64 nStartBlock, int64 nFinishBlock)
{
    *pErrorCode = ERROR_SUCCESS;

    // open / analyze the file
    m_spAPEInfo.Assign(pAPEInfo);

    // get format information
    APE_CLEAR(m_wfeInput);
    m_spAPEInfo->GetInfo(APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&m_wfeInput));
    m_nBlockAlign = static_cast<int>(m_spAPEInfo->GetInfo(APE_INFO_BLOCK_ALIGN));

    // initialize other stuff
    m_bDecompressorInitialized = false;
    m_nCurrentFrame = 0;
    m_nCurrentBlock = 0;
    m_nCurrentFrameBufferBlock = 0;
    m_nFrameBufferFinishedBlocks = 0;
    m_bErrorDecodingCurrentFrame = false;
    m_bErrorDecodingLastFrame = false;
    m_nErrorDecodingCurrentFrameOutputSilenceBlocks = 0;
    m_bInterimMode = false;
    m_nLastX = 0;
    m_nSpecialCodes = 0;
    m_nCRC = 0;
    m_nStoredCRC = 0;
    APE_CLEAR(m_aryBitArrayStates);

    // set the "real" start and finish blocks
    m_nStartBlock = (nStartBlock < 0) ? 0 : ape_min(nStartBlock, m_spAPEInfo->GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_nFinishBlock = (nFinishBlock < 0) ? m_spAPEInfo->GetInfo(APE_INFO_TOTAL_BLOCKS) : ape_min(nFinishBlock, m_spAPEInfo->GetInfo(APE_INFO_TOTAL_BLOCKS));
    m_bIsRanged = (m_nStartBlock != 0) || (m_nFinishBlock != m_spAPEInfo->GetInfo(APE_INFO_TOTAL_BLOCKS));

    // channel data
    m_sparyChannelData.Assign(new int [APE_MAXIMUM_CHANNELS], true);

    // predictors
    APE_CLEAR(m_aryPredictor);

    // version check (this implementation only works with 3.93 and later files)
    // we do this after setting all the variables or else we get compile warnings
    if (m_spAPEInfo->GetInfo(APE_INFO_FILE_VERSION) < 3930)
    {
        *pErrorCode = ERROR_UNDEFINED;
        return;
    }
}

CAPEDecompress::~CAPEDecompress()
{
    m_sparyChannelData.Delete();
    for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
    {
        if (m_aryPredictor[z] != APE_NULL)
            delete m_aryPredictor[z];
    }
}

int CAPEDecompress::InitializeDecompressor()
{
    // check if we have anything to do
    if (m_bDecompressorInitialized)
        return ERROR_SUCCESS;

    // update the initialized flag
    m_bDecompressorInitialized = true;

    // check the block align
    if ((m_nBlockAlign <= 0) || (m_nBlockAlign > 256))
        return ERROR_INVALID_INPUT_FILE;

    // create a frame buffer
    m_cbFrameBuffer.CreateBuffer((static_cast<uint32>(GetInfo(APE_INFO_BLOCKS_PER_FRAME)) + DECODE_BLOCK_SIZE) * static_cast<uint32>(m_nBlockAlign), static_cast<uint32>(m_nBlockAlign * 64));

    // create decoding components
    m_spUnBitArray.Assign(CreateUnBitArray(this, static_cast<int>(GetInfo(APE_INFO_FILE_VERSION))));
    if (m_spUnBitArray == APE_NULL)
        return ERROR_UNSUPPORTED_FILE_VERSION;

    // create the predictors
    const int nChannels = ape_min(ape_max(static_cast<int>(GetInfo(APE_INFO_CHANNELS)), 1), 32);
    const int nCompressionLevel = static_cast<int>(GetInfo(APE_INFO_COMPRESSION_LEVEL));
    const int nVersion = static_cast<int>(GetInfo(APE_INFO_FILE_VERSION));
    const int nBitsPerSample = static_cast<int>(GetInfo(APE_INFO_BITS_PER_SAMPLE));

    // loop channels
    for (int nChannel = 0; nChannel < nChannels; nChannel++)
    {
        if (nVersion >= 3950)
            if (nBitsPerSample < 32)
                m_aryPredictor[nChannel] = new CPredictorDecompress3950toCurrent<int, short>(nCompressionLevel, nVersion, nBitsPerSample);
            else
                m_aryPredictor[nChannel] = new CPredictorDecompress3950toCurrent<int64, int>(nCompressionLevel, nVersion, nBitsPerSample);
        else
            m_aryPredictor[nChannel] = new CPredictorDecompressNormal3930to3950(nCompressionLevel, nVersion);
    }

    // start with interim mode off
    m_bInterimMode = false;
    for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
    {
        if (m_aryPredictor[z] != APE_NULL)
            m_aryPredictor[z]->SetInterimMode(false);
    }

    // seek to the beginning
    return Seek(0);
}

int CAPEDecompress::GetData(unsigned char * pBuffer, int64 nBlocks, int64 * pBlocksRetrieved, APE_GET_DATA_PROCESSING * pProcessing)
{
    int nResult = ERROR_SUCCESS;
    if (pBlocksRetrieved) *pBlocksRetrieved = 0;

    // make sure we're initialized
    RETURN_ON_ERROR(InitializeDecompressor())

    // cap
    const int64 nBlocksUntilFinish = m_nFinishBlock - m_nCurrentBlock;
    const int64 nBlocksToRetrieve = ape_min(nBlocks, nBlocksUntilFinish);

    // get the data
    unsigned char * pBufferGet = pBuffer;
    int64 nBlocksLeft = nBlocksToRetrieve; int nBlocksThisPass = 1;
    while ((nBlocksLeft > 0) && (nBlocksThisPass > 0))
    {
        // fill up the frame buffer
        const int nDecodeRetVal = FillFrameBuffer();
        if (nDecodeRetVal != ERROR_SUCCESS)
            nResult = nDecodeRetVal;

        // analyze how much to remove from the buffer
        const int64 nFrameBufferBlocks = ape_min(m_nFrameBufferFinishedBlocks, static_cast<int64>(m_cbFrameBuffer.MaxGet()) / m_nBlockAlign);
        nBlocksThisPass = static_cast<int>(ape_min(nBlocksLeft, nFrameBufferBlocks));

        // remove as much as possible
        if (nBlocksThisPass > 0)
        {
            m_cbFrameBuffer.Get(pBufferGet, static_cast<uint32>(nBlocksThisPass * m_nBlockAlign));
            pBufferGet = &pBufferGet[nBlocksThisPass * m_nBlockAlign];
            nBlocksLeft -= nBlocksThisPass;
            m_nFrameBufferFinishedBlocks -= nBlocksThisPass;
        }
    }

    // calculate the blocks retrieved
    int64 nBlocksRetrieved = static_cast<int64>(nBlocksToRetrieve - nBlocksLeft);

    // update position
    m_nCurrentBlock += nBlocksRetrieved;
    if (pBlocksRetrieved) *pBlocksRetrieved = nBlocksRetrieved;

    // process data
    const int64 nBlocksDecoded = nBlocksRetrieved;
    if ((pProcessing == APE_NULL) || (pProcessing->bApplyFloatProcessing == true))
    {
        if (GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_FLOATING_POINT)
            CFloatTransform::Process(reinterpret_cast<uint32 *>(pBuffer), static_cast<int>(nBlocksDecoded * GetInfo(IAPEDecompress::APE_INFO_CHANNELS)));
    }

    if ((pProcessing == APE_NULL) || (pProcessing->bApplySigned8BitProcessing == true))
    {
        if (GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_SIGNED_8_BIT)
        {
            const int64 nChannels = GetInfo(IAPEDecompress::APE_INFO_CHANNELS);
            for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
            {
                const unsigned char cTemp = pBuffer[nSample];
                const char cTemp2 = static_cast<char>(cTemp + 128);
                pBuffer[nSample] = static_cast<unsigned char>(cTemp2);
            }
        }
    }

    if ((pProcessing == APE_NULL) || (pProcessing->bApplyBigEndianProcessing == true))
    {
        if (GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_BIG_ENDIAN)
        {
            const int64 nChannels = GetInfo(IAPEDecompress::APE_INFO_CHANNELS);
            const int64 nBitdepth = GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE);
            if (nBitdepth == 16)
            {
                for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
                {
                    const unsigned char cTemp = pBuffer[(nSample * 2) + 0];
                    pBuffer[(nSample * 2) + 0] = pBuffer[(nSample * 2) + 1];
                    pBuffer[(nSample * 2) + 1] = cTemp;
                }
            }
            else if (nBitdepth == 24)
            {
                for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
                {
                    const unsigned char cTemp = pBuffer[(3 * nSample) + 0];
                    pBuffer[(3 * nSample) + 0] = pBuffer[(3 * nSample) + 2];
                    pBuffer[(3 * nSample) + 2] = cTemp;
                }
            }
            else if (nBitdepth == 32)
            {
                uint32 * pBuffer32 = reinterpret_cast<uint32 *>(&pBuffer[0]);
                for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
                {
                    const uint32 nValue = pBuffer32[nSample];
                    const uint32 nFlippedValue = (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
                    pBuffer32[nSample] = nFlippedValue;
                }
            }
        }
    }

    return nResult;
}

int CAPEDecompress::Seek(int64 nBlockOffset)
{
    RETURN_ON_ERROR(InitializeDecompressor())

    // use the offset
    nBlockOffset += m_nStartBlock;

    // cap (to prevent seeking too far)
    if (nBlockOffset >= m_nFinishBlock)
        nBlockOffset = m_nFinishBlock - 1;
    if (nBlockOffset < m_nStartBlock)
        nBlockOffset = m_nStartBlock;

    // seek to the perfect location
    const int64 nBaseFrame = nBlockOffset / GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    const int64 nBlocksToSkip = nBlockOffset % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    const int64 nBytesToSkip = nBlocksToSkip * m_nBlockAlign;

    m_nCurrentBlock = nBaseFrame * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    m_nCurrentFrameBufferBlock = nBaseFrame * GetInfo(APE_INFO_BLOCKS_PER_FRAME);
    m_nCurrentFrame = nBaseFrame;
    m_nFrameBufferFinishedBlocks = 0;
    m_cbFrameBuffer.Empty();
    RETURN_ON_ERROR(SeekToFrame(m_nCurrentFrame))

    // skip necessary blocks
    CSmartPtr<unsigned char> spTempBuffer(new unsigned char [static_cast<size_t>(nBytesToSkip)], true);
    if (spTempBuffer == APE_NULL) return ERROR_INSUFFICIENT_MEMORY;

    int64 nBlocksRetrieved = 0;
    GetData(spTempBuffer, nBlocksToSkip, &nBlocksRetrieved, APE_NULL);
    if (nBlocksRetrieved != nBlocksToSkip)
        return ERROR_UNDEFINED;

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Decodes blocks of data
**************************************************************************************************/
int CAPEDecompress::FillFrameBuffer()
{
    int nResult = ERROR_SUCCESS;

    // determine the maximum blocks we can decode
    // note that we won't do end capping because we can't use data
    // until EndFrame(...) successfully handles the frame
    // that means we may decode a little extra in end capping cases
    // but this allows robust error handling of bad frames

    // loop and decode data
    int64 nBlocksLeft = static_cast<int64>(m_cbFrameBuffer.MaxAdd()) / m_nBlockAlign;
    while ((nBlocksLeft > 0) && (nResult == ERROR_SUCCESS))
    {
        // output silence from previous error
        if (m_nErrorDecodingCurrentFrameOutputSilenceBlocks > 0)
        {
            // output silence
            const int64 nOutputSilenceBlocks = ape_min(m_nErrorDecodingCurrentFrameOutputSilenceBlocks, nBlocksLeft);
            unsigned char cSilence = static_cast<unsigned char>((GetInfo(APE_INFO_BITS_PER_SAMPLE) == 8) ? 127 : 0);
            for (int z = 0; z < nOutputSilenceBlocks * m_nBlockAlign; z++)
            {
                *m_cbFrameBuffer.GetDirectWritePointer() = cSilence;
                m_cbFrameBuffer.UpdateAfterDirectWrite(1);
            }

            // decrement
            m_nErrorDecodingCurrentFrameOutputSilenceBlocks -= nOutputSilenceBlocks;
            nBlocksLeft -= nOutputSilenceBlocks;
            m_nFrameBufferFinishedBlocks += nOutputSilenceBlocks;
            m_nCurrentFrameBufferBlock += nOutputSilenceBlocks;
            if (nBlocksLeft <= 0)
                break;
        }

        // get frame size
        const int64 nFrameBlocks = GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame);
        if (nFrameBlocks < 0)
            break;

        // analyze
        const int64 nFrameOffsetBlocks = m_nCurrentFrameBufferBlock % GetInfo(APE_INFO_BLOCKS_PER_FRAME);
        const int64 nFrameBlocksLeft = nFrameBlocks - nFrameOffsetBlocks;
        const int64 nBlocksThisPass = ape_min(nFrameBlocksLeft, nBlocksLeft);

        // start the frame if we need to
        if (nFrameOffsetBlocks == 0)
            StartFrame();

        // decode data
        DecodeBlocksToFrameBuffer(nBlocksThisPass);

        // end the frame if we decoded all the blocks from the current frame
        bool bEndedFrame = false;
        if ((nFrameOffsetBlocks + nBlocksThisPass) >= nFrameBlocks)
        {
            EndFrame();
            bEndedFrame = true;
        }

        // handle errors (either mid-frame or from a CRC at the end of the frame)
        if (m_bErrorDecodingCurrentFrame)
        {
            int nFrameBlocksDecoded = 0;
            if (bEndedFrame)
            {
                // remove the frame buffer blocks that have been marked as good
                m_nFrameBufferFinishedBlocks -= GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame - 1);

                // assume that the frame buffer contains the correct number of blocks for the entire frame
                nFrameBlocksDecoded = static_cast<int>(GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame - 1));
            }
            else
            {
                // move to the next frame
                m_nCurrentFrame++;

                // calculate how many blocks were output before we errored
                nFrameBlocksDecoded = static_cast<int>((m_nCurrentFrameBufferBlock - (GetInfo(APE_INFO_BLOCKS_PER_FRAME) * (m_nCurrentFrame - 1))));
            }

            // restart the current frame (unless we're coming already from an error condition)
            if (m_bErrorDecodingLastFrame == false)
                m_nCurrentFrame--;

            // remove any decoded data for this frame from the buffer
            const int nFrameBytesDecoded = nFrameBlocksDecoded * m_nBlockAlign;
            m_cbFrameBuffer.RemoveTail(static_cast<uint32>(nFrameBytesDecoded));

            // seek to try to synchronize after an error
            if (m_nCurrentFrame < GetInfo(APE_INFO_TOTAL_FRAMES))
                SeekToFrame(m_nCurrentFrame);

            // reset our frame buffer position to the beginning of the frame
            m_nCurrentFrameBufferBlock = (m_nCurrentFrame - 1) * GetInfo(APE_INFO_BLOCKS_PER_FRAME);

            // enter interim mode if we're a 24-bit file and try the frame again
            // this is because for a while (from the addition of 32-bit to version 8.50) we would encode the file using int64 values instead of int32 values for a couple things
            if ((m_bInterimMode == false) && (GetInfo(APE_INFO_BITS_PER_SAMPLE) == 24))
            {
                m_bInterimMode = true;
                for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
                {
                    if (m_aryPredictor[z] != APE_NULL)
                        m_aryPredictor[z]->SetInterimMode(true);
                }
            }
            else
            {
                // output silence for the duration of the error frame (we can't just dump it to the
                // frame buffer here since the frame buffer may not be large enough to hold the
                // duration of the entire frame)
                m_nErrorDecodingCurrentFrameOutputSilenceBlocks += nFrameBlocks;

                // save the return value
                nResult = ERROR_INVALID_CHECKSUM;
            }
        }

        // update the number of blocks that still fit in the buffer
        nBlocksLeft = static_cast<int64>(m_cbFrameBuffer.MaxAdd()) / m_nBlockAlign;
    }

    return nResult;
}

void CAPEDecompress::DecodeBlocksToFrameBuffer(int64 nBlocks)
{
    // decode the samples
    int nBlocksProcessed = 0;
    const int nFrameBufferBytes = static_cast<int>(m_cbFrameBuffer.MaxGet());

    try
    {
        if (m_wfeInput.nChannels > 2)
        {
            for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
            {
                for (int nChannel = 0; nChannel < m_wfeInput.nChannels; nChannel++)
                {
                    const int64 nValue = m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[nChannel]);
                    const int nValue2 = m_aryPredictor[nChannel]->DecompressValue(nValue, 0);
                    m_sparyChannelData[nChannel] = nValue2;
                }
                m_Prepare.Unprepare(m_sparyChannelData, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
            }
        }
        else if (m_wfeInput.nChannels == 2)
        {
            if ((m_nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) &&
                (m_nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE))
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int aryValues[2] = { 0, 0 };
                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
                }
            }
            else if (m_nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int aryValues[2] = { m_aryPredictor[0]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0])), 0 };

                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
                }
            }
            else
            {
                if (m_spAPEInfo->GetInfo(APE_INFO_FILE_VERSION) >= 3950)
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        const int64 nY = m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[1]);
                        const int64 nX = m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0]);
                        int Y = m_aryPredictor[1]->DecompressValue(nY, m_nLastX);
                        int X = m_aryPredictor[0]->DecompressValue(nX, Y);
                        m_nLastX = X;

                        int aryValues[2] = { X, Y };
                        unsigned char * pOutput = m_cbFrameBuffer.GetDirectWritePointer();
                        m_Prepare.Unprepare(aryValues, &m_wfeInput, pOutput);
                        m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
                    }
                }
                else
                {
                    for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                    {
                        int X = m_aryPredictor[0]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0]));
                        int Y = m_aryPredictor[1]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[1]));

                        int aryValues[2] = { X, Y };
                        m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                        m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
                    }
                }
            }
        }
        else if (m_wfeInput.nChannels == 1)
        {
            if (m_nSpecialCodes & SPECIAL_FRAME_MONO_SILENCE)
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int aryValues[2] = { 0, 0 };
                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
                }
            }
            else
            {
                for (nBlocksProcessed = 0; nBlocksProcessed < nBlocks; nBlocksProcessed++)
                {
                    int aryValues[2] = { m_aryPredictor[0]->DecompressValue(m_spUnBitArray->DecodeValueRange(m_aryBitArrayStates[0])), 0 };
                    m_Prepare.Unprepare(aryValues, &m_wfeInput, m_cbFrameBuffer.GetDirectWritePointer());
                    m_cbFrameBuffer.UpdateAfterDirectWrite(static_cast<uint32>(m_nBlockAlign));
                }
            }
        }
    }
    catch(...)
    {
        m_bErrorDecodingCurrentFrame = true;
    }

    // get actual blocks that have been decoded and added to the frame buffer
    int nActualBlocks = (static_cast<int>(m_cbFrameBuffer.MaxGet()) - nFrameBufferBytes) / m_nBlockAlign;
    nActualBlocks = ape_max(nActualBlocks, 0);
    if (nBlocks != nActualBlocks)
        m_bErrorDecodingCurrentFrame = true;

    // update CRC
    m_nCRC = m_cbFrameBuffer.UpdateCRC(m_nCRC, static_cast<uint32>(nActualBlocks * m_nBlockAlign));

    // bump frame decode position
    m_nCurrentFrameBufferBlock += nActualBlocks;
}

void CAPEDecompress::StartFrame()
{
    m_nCRC = 0xFFFFFFFF;

    // get the frame header
    m_nStoredCRC = static_cast<unsigned int>(m_spUnBitArray->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_UNSIGNED_INT));
    m_bErrorDecodingLastFrame = m_bErrorDecodingCurrentFrame;
    m_bErrorDecodingCurrentFrame = false;
    m_nErrorDecodingCurrentFrameOutputSilenceBlocks = 0;

    // get any 'special' codes if the file uses them (for silence, false stereo, etc.)
    m_nSpecialCodes = 0;
    if (GET_USES_SPECIAL_FRAMES(m_spAPEInfo))
    {
        if (m_nStoredCRC & 0x80000000)
        {
            m_nSpecialCodes = static_cast<int>(m_spUnBitArray->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_UNSIGNED_INT));
        }
        m_nStoredCRC &= 0x7FFFFFFF;
    }

    for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
    {
        if (m_aryPredictor[z] != APE_NULL)
            m_aryPredictor[z]->Flush();
    }

    for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
    {
        m_spUnBitArray->FlushState(m_aryBitArrayStates[z]);
    }

    m_spUnBitArray->FlushBitArray();
    m_nLastX = 0;
}

void CAPEDecompress::EndFrame()
{
    m_nFrameBufferFinishedBlocks += GetInfo(APE_INFO_FRAME_BLOCKS, m_nCurrentFrame);
    m_nCurrentFrame++;

    // finalize
    m_spUnBitArray->Finalize();

    // check the CRC
    m_nCRC = m_nCRC ^ 0xFFFFFFFF;
    m_nCRC >>= 1;
    if (m_nCRC != m_nStoredCRC)
    {
        // error
        m_bErrorDecodingCurrentFrame = true;

        // We didn't use to check the CRC of the last frame in MAC 3.98 and earlier.  This caused some confusion for one
        // user that had a lot of 3.97 Extra High files that have CRC errors on the last frame.  They would verify
        // with old versions, but not with newer versions.  It's still unknown what corrupted the user's files but since
        // only the last frame was bad, it's likely to have been caused by a buggy tagger.
        //if ((m_nCurrentFrame >= GetInfo(APE_INFO_TOTAL_FRAMES)) && (GetInfo(APE_INFO_FILE_VERSION) < 3990))
        //    m_bErrorDecodingCurrentFrame = false;
    }
}

/**************************************************************************************************
Seek to the proper frame (if necessary) and do any alignment of the bit array
**************************************************************************************************/
int CAPEDecompress::SeekToFrame(int64 nFrameIndex)
{
    const int64 nSeekRemainder = (GetInfo(APE_INFO_SEEK_BYTE, nFrameIndex) - GetInfo(APE_INFO_SEEK_BYTE, 0)) % 4;
    return m_spUnBitArray->FillAndResetBitArray(GetInfo(APE_INFO_SEEK_BYTE, nFrameIndex) - nSeekRemainder, nSeekRemainder * 8);
}

/**************************************************************************************************
Get information from the decompressor
**************************************************************************************************/
int64 CAPEDecompress::GetInfo(IAPEDecompress::APE_DECOMPRESS_FIELDS Field, int64 nParam1, int64 nParam2)
{
    int64 nResult = 0;
    bool bHandled = false;

    switch (Field)
    {
    case APE_DECOMPRESS_CURRENT_BLOCK:
    {
        nResult = static_cast<int64>(m_nCurrentBlock - m_nStartBlock);
        bHandled = true;
        break;
    }
    case APE_DECOMPRESS_CURRENT_MS:
    {
        const int64 nSampleRate = m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0);
        if (nSampleRate > 0)
        {
            nResult = static_cast<int64>((static_cast<double>(m_nCurrentBlock) * static_cast<double>(1000)) / static_cast<double>(nSampleRate));
            bHandled = true;
        }
        break;
    }
    case APE_DECOMPRESS_TOTAL_BLOCKS:
    {
        nResult = static_cast<int64>(m_nFinishBlock - m_nStartBlock);
        bHandled = true;
        break;
    }
    case APE_DECOMPRESS_LENGTH_MS:
    {
        const int64 nSampleRate = static_cast<int64>(m_spAPEInfo->GetInfo(APE_INFO_SAMPLE_RATE, 0, 0));
        if (nSampleRate > 0)
        {
            nResult = static_cast<int64>((static_cast<double>(m_nFinishBlock - m_nStartBlock) * static_cast<double>(1000)) / static_cast<double>(nSampleRate));
            bHandled = true;
        }
        break;
    }
    case APE_DECOMPRESS_CURRENT_BITRATE:
    {
        nResult = GetInfo(APE_INFO_FRAME_BITRATE, m_nCurrentFrame);
        bHandled = true;
        break;
    }
    case APE_DECOMPRESS_CURRENT_FRAME:
    {
        nResult = m_nCurrentFrame;
        bHandled = true;
        break;
    }
    case APE_DECOMPRESS_AVERAGE_BITRATE:
    {
        if (m_bIsRanged)
        {
            // figure the frame range
            const int64 nBlocksPerFrame = GetInfo(APE_INFO_BLOCKS_PER_FRAME);
            const int64 nStartFrame = static_cast<int64>(m_nStartBlock / nBlocksPerFrame);
            const int64 nFinishFrame = static_cast<int64>(m_nFinishBlock + nBlocksPerFrame - 1) / nBlocksPerFrame;

            // get the number of bytes in the first and last frame
            int64 nTotalBytes = (GetInfo(APE_INFO_FRAME_BYTES, nStartFrame) * (m_nStartBlock % nBlocksPerFrame)) / nBlocksPerFrame;
            if (nFinishFrame != nStartFrame)
                nTotalBytes += (GetInfo(APE_INFO_FRAME_BYTES, nFinishFrame) * (m_nFinishBlock % nBlocksPerFrame)) / nBlocksPerFrame;

            // get the number of bytes in between
            const int64 nTotalFrames = GetInfo(APE_INFO_TOTAL_FRAMES);
            for (int64 nFrame = nStartFrame + 1; (nFrame < nFinishFrame) && (nFrame < nTotalFrames); nFrame++)
                nTotalBytes += GetInfo(APE_INFO_FRAME_BYTES, nFrame);

            // figure the bitrate
            const int64 nTotalMS = static_cast<int64>((static_cast<double>(m_nFinishBlock - m_nStartBlock) * static_cast<double>(1000)) / static_cast<double>(GetInfo(APE_INFO_SAMPLE_RATE)));
            if (nTotalMS != 0)
                nResult = static_cast<int64>((nTotalBytes * 8) / nTotalMS);
        }
        else
        {
            nResult = GetInfo(APE_INFO_AVERAGE_BITRATE);
        }
        bHandled = true;
        break;
    }
    case APE_INFO_WAV_HEADER_BYTES:
    {
        if (m_bIsRanged)
        {
            nResult = sizeof(WAVE_HEADER);
            bHandled = true;
        }
        break;
    }
    case APE_INFO_WAV_HEADER_DATA:
    {
        if (m_bIsRanged)
        {
            char * pBuffer = reinterpret_cast<char *>(nParam1);
            const int64 nMaxBytes = nParam2;

            if (static_cast<APE::int64>(sizeof(WAVE_HEADER)) > nMaxBytes)
            {
                nResult = -1;
            }
            else
            {
                WAVEFORMATEX wfeFormat; APE_CLEAR(wfeFormat);
                GetInfo(APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&wfeFormat), 0);
                WAVE_HEADER WAVHeader; FillWaveHeader(&WAVHeader,
                    (m_nFinishBlock - m_nStartBlock) * GetInfo(APE_INFO_BLOCK_ALIGN),
                    &wfeFormat, 0);
                memcpy(pBuffer, &WAVHeader, sizeof(WAVE_HEADER));
                nResult = 0;
            }
            bHandled = true;
        }

        break;
    }
    case APE_INFO_WAV_TERMINATING_BYTES:
    {
        if (m_bIsRanged)
        {
            nResult = 0;
            bHandled = true;
        }
        break;
    }
    case APE_INFO_WAV_TERMINATING_DATA:
    {
        if (m_bIsRanged)
        {
            nResult = 0;
            bHandled = true;
        }
        break;
    }
    case APE_INFO_APE_TOTAL_BYTES:
    case APE_INFO_APL:
    case APE_INFO_AVERAGE_BITRATE:
    case APE_INFO_BITS_PER_SAMPLE:
    case APE_INFO_BLOCKS_PER_FRAME:
    case APE_INFO_BLOCK_ALIGN:
    case APE_INFO_BYTES_PER_SAMPLE:
    case APE_INFO_CHANNELS:
    case APE_INFO_COMPRESSION_LEVEL:
    case APE_INFO_DECOMPRESSED_BITRATE:
    case APE_INFO_FILE_VERSION:
    case APE_INFO_FINAL_FRAME_BLOCKS:
    case APE_INFO_FORMAT_FLAGS:
    case APE_INFO_FRAME_BITRATE:
    case APE_INFO_FRAME_BLOCKS:
    case APE_INFO_FRAME_BYTES:
    case APE_INFO_IO_SOURCE:
    case APE_INFO_LENGTH_MS:
    case APE_INFO_MD5:
    case APE_INFO_MD5_MATCHES:
    case APE_INFO_PEAK_LEVEL:
    case APE_INFO_SAMPLE_RATE:
    case APE_INFO_SEEK_BIT:
    case APE_INFO_SEEK_BYTE:
    case APE_INFO_TAG:
    case APE_INFO_TOTAL_BLOCKS:
    case APE_INFO_TOTAL_FRAMES:
    case APE_INFO_WAVEFORMATEX:
    case APE_INFO_WAV_DATA_BYTES:
    case APE_INFO_WAV_TOTAL_BYTES:
    case APE_INTERNAL_INFO:
    {
        // all other conditions to prevent compiler warnings (4061 and Clang)
        break;
    }
    }

    if (!bHandled)
        nResult = m_spAPEInfo->GetInfo(Field, nParam1, nParam2);

    return nResult;
}

}
