#include "All.h"
#define APE_ENABLE_CIRCLE_BUFFER_WRITE
#include "APEDecompress.h"
#include "APEDecompressCore.h"
#include "APEInfo.h"
#include "NewPredictor.h"
#include "FloatTransform.h"
#include "MemoryIO.h"

namespace APE
{

#define DECODE_BLOCK_SIZE        4096

CAPEDecompressCore::CAPEDecompressCore(int * pErrorCode, CAPEDecompress * pDecompress, CAPEInfo * pAPEInfo)
: m_semProcess(1), m_semReady(1)
{
    m_semProcess.Wait();

    *pErrorCode = ERROR_SUCCESS;

    // open / analyze the file
    m_pAPEInfo = pAPEInfo;
    m_pDecompress = pDecompress;

    // get format information
    APE_CLEAR(m_wfeInput);
    m_pAPEInfo->GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&m_wfeInput));
    m_nBlockAlign = static_cast<int>(m_pAPEInfo->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN));

    // initialize other stuff
    m_nInputBytes = 0;
    m_nSkipBytes = 0;
    m_bDecompressorInitialized = false;
    m_nFrameBlocks = 0;
    m_bErrorDecodingCurrentFrame = false;
    m_bInterimMode = false;
    m_nLastX = 0;
    m_nSpecialCodes = 0;
    m_nCRC = 0;
    m_nStoredCRC = 0;
    m_nErrorState = ERROR_SUCCESS;
    m_bCancelFrame = false;
    m_bExit = false;
    APE_CLEAR(m_aryBitArrayStates);

    // channel data
    m_sparyChannelData.Assign(new int [APE_MAXIMUM_CHANNELS], true);

    // predictors
    APE_CLEAR(m_aryPredictor);

    // version check (this implementation only works with 3.93 and later files)
    // we do this after setting all the variables or else we get compile warnings
    if (m_pAPEInfo->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) < 3930)
    {
        *pErrorCode = ERROR_UNDEFINED;
        return;
    }
}

CAPEDecompressCore::~CAPEDecompressCore()
{
    // stop any threading
    Exit();
    Wait();

    // delete the predictors
    for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
    {
        if (m_aryPredictor[z] != APE_NULL)
            delete m_aryPredictor[z];
    }
}

int CAPEDecompressCore::Run()
{
    while (!m_bExit)
    {
        m_semProcess.Wait();

        if (m_bExit) break;

        int nResult = DecodeFrame();
        if (nResult != ERROR_SUCCESS)
            SetErrorState(nResult);
        else
            m_semReady.Post();
    }

    return 0;
}

void CAPEDecompressCore::DecodeFrame(int nSkipBytes, int64 nFrameBlocks)
{
    m_spUnBitArray->FillAndResetBitArray(0, static_cast<int64>(nSkipBytes) * 8);
    m_nSkipBytes = nSkipBytes;
    m_nFrameBlocks = nFrameBlocks;
    m_nErrorState = ERROR_SUCCESS;
    m_bCancelFrame = false;

    m_semProcess.Post();
}

void CAPEDecompressCore::CancelFrame()
{
    m_bCancelFrame = true;
}

unsigned char* CAPEDecompressCore::GetInputBuffer(uint32 nInputBytes)
{
    if (m_nInputBytes < nInputBytes)
    {
        m_spInputData.Assign(new unsigned char[static_cast<size_t>(nInputBytes)], true);
        m_spIO.Assign(new CMemoryIO(m_spInputData, static_cast<int>(nInputBytes)));
        m_spUnBitArray.Assign(CreateUnBitArray(m_pDecompress, m_spIO, static_cast<int>(m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION))));
        m_nInputBytes = nInputBytes;
    }

    return m_spInputData;
}

void CAPEDecompressCore::GetFrameData(unsigned char * pBuffer)
{
    m_cbFrameBuffer.Get(pBuffer, GetFrameBytes());
}

uint32 CAPEDecompressCore::GetFrameBytes() const
{
    return static_cast<uint32>(m_nFrameBlocks) * static_cast<uint32>(m_nBlockAlign);
}

int CAPEDecompressCore::SetErrorState(int nError)
{
    m_nErrorState = nError;
    m_nFrameBlocks = 0;
    m_cbFrameBuffer.Empty();

    m_semReady.Post();

    return m_nErrorState;
}

int CAPEDecompressCore::GetErrorState() const
{
    return m_nErrorState;
}

int CAPEDecompressCore::InitializeDecompressor()
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
    m_cbFrameBuffer.CreateBuffer(static_cast<uint32>(m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCKS_PER_FRAME)) * static_cast<uint32>(m_nBlockAlign), static_cast<uint32>(m_nBlockAlign * 64));

    // create the predictors
    const int nChannels = ape_min(ape_max(static_cast<int>(m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), 1), 32);
    const int nCompressionLevel = static_cast<int>(m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL));
    const int nVersion = static_cast<int>(m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION));
    const int nBitsPerSample = static_cast<int>(m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE));

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

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Decodes blocks of data
**************************************************************************************************/
int CAPEDecompressCore::DecodeFrame()
{
    InitializeDecompressor();

    int nResult = ERROR_SUCCESS;

    m_cbFrameBuffer.Empty();

    // determine the maximum blocks we can decode
    // note that we won't do end capping because we can't use data
    // until EndFrame(...) successfully handles the frame
    // that means we may decode a little extra in end capping cases
    // but this allows robust error handling of bad frames
    int64 nBlocksLeft = m_nFrameBlocks;
    if (nBlocksLeft <= 0)
        return ERROR_DECOMPRESSING_FRAME;

    // loop and decode data
    while ((nBlocksLeft > 0) && (nResult == ERROR_SUCCESS))
    {
        // analyze
        const int64 nBlocksThisPass = m_nFrameBlocks;

        // start the frame
        StartFrame();

        // decode data
        DecodeBlocksToFrameBuffer(nBlocksThisPass);

        // end the frame
        EndFrame();

        // handle errors (either mid-frame or from a CRC at the end of the frame)
        if (m_bErrorDecodingCurrentFrame)
        {
            // remove any decoded data for this frame from the buffer
            m_cbFrameBuffer.Empty();

            // enter interim mode if we're a 24-bit file and try the frame again
            // this is because for a while (from the addition of 32-bit to version 8.50) we would encode the file using int64 values instead of int32 values for a couple things
            if ((m_bInterimMode == false) && (m_pDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE) == 24))
            {
                m_bInterimMode = true;
                for (int z = 0; z < APE_MAXIMUM_CHANNELS; z++)
                {
                    if (m_aryPredictor[z] != APE_NULL)
                        m_aryPredictor[z]->SetInterimMode(true);
                }
                m_spUnBitArray->FillAndResetBitArray(0, static_cast<int64>(m_nSkipBytes) * 8);
                continue;
            }
            else
            {
                // save the return value
                nResult = ERROR_INVALID_CHECKSUM;
            }
        }

        nBlocksLeft -= nBlocksThisPass;
    }

    if (m_bCancelFrame)
        m_nFrameBlocks = 0;

    m_bCancelFrame = false;

    return nResult;
}

void CAPEDecompressCore::DecodeBlocksToFrameBuffer(int64 nBlocks)
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
                if (m_pAPEInfo->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) >= 3950)
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
}

void CAPEDecompressCore::StartFrame()
{
    m_nCRC = 0xFFFFFFFF;

    // get the frame header
    m_nStoredCRC = static_cast<unsigned int>(m_spUnBitArray->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_UNSIGNED_INT));
    m_bErrorDecodingCurrentFrame = false;

    // get any 'special' codes if the file uses them (for silence, false stereo, etc.)
    m_nSpecialCodes = 0;
    if (GET_USES_SPECIAL_FRAMES(m_pAPEInfo))
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

void CAPEDecompressCore::EndFrame()
{
    // finalize
    m_spUnBitArray->Finalize();

    // check the CRC
    m_nCRC = m_nCRC ^ 0xFFFFFFFF;
    m_nCRC >>= 1;
    if (m_nCRC != m_nStoredCRC)
    {
        // error
        m_bErrorDecodingCurrentFrame = true;
    }
}

void CAPEDecompressCore::WaitUntilReady()
{
    m_semReady.Wait();
}

void CAPEDecompressCore::Exit()
{
    m_bExit = true;

    m_semProcess.Post();
}

}
