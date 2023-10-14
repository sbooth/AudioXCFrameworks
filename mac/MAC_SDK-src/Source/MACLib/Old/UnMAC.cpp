/**************************************************************************************************
Includes
**************************************************************************************************/
#include "All.h"
#ifdef APE_BACKWARDS_COMPATIBILITY

#define APE_DECOMPRESS_CORE_GET_UNBITARRAY
#include "APEInfo.h"
#include "UnMAC.h"
#include "Prepare.h"
#include "APEDecompressCore.h"

namespace APE
{

/**************************************************************************************************
CUnMAC class construction
**************************************************************************************************/
CUnMAC::CUnMAC()
{
    // initialize member variables
    m_bInitialized = false;
    m_LastDecodedFrameIndex = -1;
    m_pAPEDecompress = APE_NULL;
    APE_CLEAR(m_wfeInput);

    m_pAPEDecompressCore = APE_NULL;
    m_pPrepare = APE_NULL;

    m_nBlocksProcessed = 0;
    m_nCRC = 0;
    m_nStoredCRC = 0;
}

/**************************************************************************************************
CUnMAC class destruction
**************************************************************************************************/
CUnMAC::~CUnMAC()
{
    // uninitialize the decoder in case it isn't already
    Uninitialize();
}

/**************************************************************************************************
Initialize
**************************************************************************************************/
int CUnMAC::Initialize(IAPEDecompress * pAPEDecompress)
{
    // uninitialize if it is currently initialized
    if (m_bInitialized)
        Uninitialize();

    if (pAPEDecompress == APE_NULL)
    {
        Uninitialize();
        return ERROR_INITIALIZING_UNMAC;
    }

    // set the member pointer to the IAPEDecompress class
    m_pAPEDecompress = pAPEDecompress;

    // set the last decode frame to -1 so it forces a seek on start
    m_LastDecodedFrameIndex = -1;

    m_pAPEDecompressCore = new CAPEDecompressCore(pAPEDecompress);
    m_pPrepare = new CPrepare;

    // set the initialized flag to true
    m_bInitialized = true;

    APE_CLEAR(m_wfeInput);
    m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&m_wfeInput));

    // return a successful value
    return ERROR_SUCCESS;
}

/**************************************************************************************************
Uninitialize
**************************************************************************************************/
int CUnMAC::Uninitialize()
{
    if (m_bInitialized)
    {
        APE_SAFE_DELETE(m_pAPEDecompressCore)
        APE_SAFE_DELETE(m_pPrepare)

        // clear the APE info pointer
        m_pAPEDecompress = APE_NULL;

        // set the last decoded frame again
        m_LastDecodedFrameIndex = -1;

        // set the initialized flag to false
        m_bInitialized = false;
    }

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Decompress frame
**************************************************************************************************/
intn CUnMAC::DecompressFrame(unsigned char * pOutputData, int32 FrameIndex)
{
    return DecompressFrameOld(pOutputData, FrameIndex);
}

/**************************************************************************************************
Seek to the proper frame (if necessary) and do any alignment of the bit array
**************************************************************************************************/
int CUnMAC::SeekToFrame(intn FrameIndex)
{
    if (GET_FRAMES_START_ON_BYTES_BOUNDARIES(m_pAPEDecompress))
    {
        if ((m_LastDecodedFrameIndex == -1) || ((FrameIndex - 1) != m_LastDecodedFrameIndex))
        {
            const intn SeekRemainder = (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, FrameIndex) - m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, 0)) % 4;
            m_pAPEDecompressCore->GetUnBitArrray()->FillAndResetBitArray(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, FrameIndex) - SeekRemainder, static_cast<int64>(SeekRemainder) * 8);
        }
        else
        {
            m_pAPEDecompressCore->GetUnBitArrray()->AdvanceToByteBoundary();
        }
    }
    else
    {
        if ((m_LastDecodedFrameIndex == -1) || ((FrameIndex - 1) != m_LastDecodedFrameIndex))
        {
            m_pAPEDecompressCore->GetUnBitArrray()->FillAndResetBitArray(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SEEK_BYTE, FrameIndex), static_cast<intn>(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SEEK_BIT, FrameIndex)));
        }
    }

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Old code for frame decompression
**************************************************************************************************/
intn CUnMAC::DecompressFrameOld(unsigned char * pOutputData, int32 FrameIndex)
{
    // error check the parameters (too high of a frame index, etc.)
    if (FrameIndex >= m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TOTAL_FRAMES)) { return ERROR_SUCCESS; }

    // get the number of samples in the frame
    int nBlocks = 0;
    nBlocks = ((static_cast<int64>(FrameIndex) + 1) >= m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TOTAL_FRAMES)) ? static_cast<int>(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FINAL_FRAME_BLOCKS)) : static_cast<int>(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCKS_PER_FRAME));
    if (nBlocks == 0)
        return -1; // nothing to do (file must be zero length) (have to return error)

    // take care of seeking and frame alignment
    if (SeekToFrame(FrameIndex) != 0) { return ERROR_UNDEFINED; }

    // get the checksum
    unsigned int nSpecialCodes = 0;
    uint32 nStoredCRC = 0;

    if (!GET_USES_CRC(m_pAPEDecompress))
    {
        nStoredCRC = static_cast<uint32>(m_pAPEDecompressCore->GetUnBitArrray()->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_UNSIGNED_RICE, 30));
        if (nStoredCRC == 0)
        {
            nSpecialCodes = SPECIAL_FRAME_LEFT_SILENCE | SPECIAL_FRAME_RIGHT_SILENCE;
        }
    }
    else
    {
        nStoredCRC = static_cast<uint32>(m_pAPEDecompressCore->GetUnBitArrray()->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_UNSIGNED_INT));

        // get any 'special' codes if the file uses them (for silence, false stereo, etc.)
        nSpecialCodes = 0;
        if (GET_USES_SPECIAL_FRAMES(m_pAPEDecompress))
        {
            if (nStoredCRC & 0x80000000)
            {
                nSpecialCodes = static_cast<unsigned int>(m_pAPEDecompressCore->GetUnBitArrray()->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_UNSIGNED_INT));
            }
            nStoredCRC &= 0x7FFFFFFF;
        }
    }

    // the CRC that will be figured during decompression
    uint32 CRC = 0xFFFFFFFF;

    // decompress and convert from (x,y) -> (l,r)
    // sort of int and ugly.... sorry
    if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) == 2)
    {
        m_pAPEDecompressCore->GenerateDecodedArrays(nBlocks, static_cast<intn>(nSpecialCodes), static_cast<intn>(FrameIndex));

        WAVEFORMATEX WaveFormatEx; APE_CLEAR(WaveFormatEx); m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&WaveFormatEx));
        m_pPrepare->UnprepareOld(m_pAPEDecompressCore->GetDataX(), m_pAPEDecompressCore->GetDataY(), nBlocks, &WaveFormatEx,
            pOutputData, static_cast<unsigned int*>(&CRC), static_cast<int>(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)));
    }
    else if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) == 1)
    {
        m_pAPEDecompressCore->GenerateDecodedArrays(nBlocks, static_cast<intn>(nSpecialCodes), static_cast<intn>(FrameIndex));

        WAVEFORMATEX WaveFormatEx; APE_CLEAR(WaveFormatEx); m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&WaveFormatEx));
        m_pPrepare->UnprepareOld(m_pAPEDecompressCore->GetDataX(), APE_NULL, nBlocks, &WaveFormatEx,
            pOutputData, static_cast<unsigned int *>(&CRC), static_cast<int>(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)));
    }

    if (GET_USES_SPECIAL_FRAMES(m_pAPEDecompress))
    {
        CRC >>= 1;
    }

    // check the CRC
    if (!GET_USES_CRC(m_pAPEDecompress))
    {
        const uint32 nChecksum = CalculateOldChecksum(m_pAPEDecompressCore->GetDataX(), m_pAPEDecompressCore->GetDataY(), static_cast<intn>(m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), nBlocks);
        if (nChecksum != nStoredCRC)
            return ERROR_UNDEFINED;
    }
    else
    {
        if (CRC != nStoredCRC)
            return ERROR_UNDEFINED;
    }

    m_LastDecodedFrameIndex = FrameIndex;
    return nBlocks;
}


/**************************************************************************************************
Figures the old checksum using the X,Y data
**************************************************************************************************/
uint32 CUnMAC::CalculateOldChecksum(const int * pDataX, const int * pDataY, intn nChannels, intn nBlocks)
{
    uint32 nChecksum = 0;

    if (nChannels == 2)
    {
        for (int z = 0; z < nBlocks; z++)
        {
            const int R = pDataX[z] - (pDataY[z] / 2);
            const int L = R + pDataY[z];
            nChecksum += static_cast<uint32>(labs(R) + labs(L));
        }
    }
    else if (nChannels == 1)
    {
        for (int z = 0; z < nBlocks; z++)
            nChecksum += static_cast<uint32>(labs(pDataX[z]));
    }

    return nChecksum;
}
}
#endif // #ifdef APE_BACKWARDS_COMPATIBILITY
