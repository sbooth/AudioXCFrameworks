#include "All.h"
#include "UnBitArrayBase.h"
#include "APEInfo.h"
#include "UnBitArray.h"
#ifdef APE_BACKWARDS_COMPATIBILITY
    #include "Old/APEDecompressOld.h"
    #include "Old/UnBitArrayOld.h"
#endif

namespace APE
{

const uint32 POWERS_OF_TWO_MINUS_ONE[33] = {0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535,131071,262143,524287,1048575,2097151,4194303,8388607,16777215,33554431,67108863,134217727,268435455,536870911,1073741823,2147483647,4294967295U};

CUnBitArrayBase * CreateUnBitArray(IAPEDecompress * pAPEDecompress, intn nVersion)
{
    // determine the furthest position we should read in the I/O object
    int64 nFurthestReadByte = GET_IO(pAPEDecompress)->GetSize();
    if (nFurthestReadByte > 0)
    {
       // terminating data
       nFurthestReadByte -= pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES);

       // tag (not worth analyzing the tag since we could be a remote file, etc.)
       // also don't not read the tag if we're working on an APL file
       if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APL) == 0)
       {
           CAPETag* pAPETag = (CAPETag*)pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TAG);
           if ((pAPETag != NULL) && pAPETag->GetAnalyzed())
               nFurthestReadByte -= pAPETag->GetTagBytes();
       }
    }

#ifdef APE_BACKWARDS_COMPATIBILITY
    if (nVersion >= 3900)
        return (CUnBitArrayBase * ) new CUnBitArray(GET_IO(pAPEDecompress), nVersion, nFurthestReadByte);
    else
        return (CUnBitArrayBase * ) new CUnBitArrayOld(pAPEDecompress, nVersion, nFurthestReadByte);
#else
    // create the appropriate object
    if (nVersion < 3900)
    {
        // we should no longer be trying to decode files this old (this check should be redundant since 
        // the main gate keeper should be CreateIAPEDecompressCore(...)
        ASSERT(false);
        return NULL;
    }

    return (CUnBitArrayBase * ) new CUnBitArray(GET_IO(pAPEDecompress), nVersion, nFurthestReadByte);
#endif
}

CUnBitArrayBase::CUnBitArrayBase(int64 nFurthestReadByte)
{
    m_nFurthestReadByte = nFurthestReadByte;
}

CUnBitArrayBase::~CUnBitArrayBase()
{
}

void CUnBitArrayBase::AdvanceToByteBoundary() 
{
    int nMod = m_nCurrentBitIndex % 8;
    if (nMod != 0) { m_nCurrentBitIndex += 8 - nMod; }
}

bool CUnBitArrayBase::EnsureBitsAvailable(uint32 nBits, bool bThrowExceptionOnFailure)
{
    bool bResult = true;

    // get more data if necessary
    if ((m_nCurrentBitIndex + nBits) >= (m_nGoodBytes * 8))
    {
        // fill
        FillBitArray();

        // if we still don't have enough good bytes, we don't have the bits available
        if ((m_nCurrentBitIndex + nBits) >= (m_nGoodBytes * 8))
        {
            // overread error
            ASSERT(false);

            // throw exception if specified
            if (bThrowExceptionOnFailure)
                throw(1);

            // data not available
            bResult = false;
        }
    }

    return bResult;
}

uint32 CUnBitArrayBase::DecodeValueXBits(uint32 nBits) 
{
    // get more data if necessary
    EnsureBitsAvailable(nBits, true);

    // variable declares
    uint32 nLeftBits = 32 - (m_nCurrentBitIndex & 31);
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
    m_nCurrentBitIndex += nBits;
    
    // if their isn't an overflow to the right value, get the value and exit
    if (nLeftBits >= nBits)
        return (m_pBitArray[nBitArrayIndex] & (POWERS_OF_TWO_MINUS_ONE[nLeftBits])) >> (nLeftBits - nBits);
    
    // must get the "split" value from left and right
    int nRightBits = nBits - nLeftBits;
    
    uint32 nLeftValue = ((m_pBitArray[nBitArrayIndex] & POWERS_OF_TWO_MINUS_ONE[nLeftBits]) << nRightBits);
    uint32 nRightValue = (m_pBitArray[nBitArrayIndex + 1] >> (32 - nRightBits));
    return (nLeftValue | nRightValue);
}

int CUnBitArrayBase::FillAndResetBitArray(int64 nFileLocation, int64 nNewBitIndex)
{
    if (nNewBitIndex < 0) return ERROR_INVALID_INPUT_FILE;

    // seek if necessary
    if (nFileLocation != -1)
    {
        m_pIO->SetSeekMethod(APE_FILE_BEGIN);
        m_pIO->SetSeekPosition(nFileLocation);
        if (m_pIO->PerformSeek() != 0)
            return ERROR_IO_READ;
    }

    // fill
    m_nCurrentBitIndex = m_nBits; // position at the end of the buffer
    int nResult = FillBitArray();

    // set bit index
    m_nCurrentBitIndex = (uint32) nNewBitIndex;

    return nResult;
}

int CUnBitArrayBase::FillBitArray() 
{
    // get the bit array index
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
    
    // move the remaining data to the front
    int nBytesToMove = m_nBytes - (nBitArrayIndex * 4);
    if (nBytesToMove > 0)
        memmove((void *) (m_pBitArray), (const void *) (m_pBitArray + nBitArrayIndex), nBytesToMove);

    // get the number of bytes to read
    int64 nBytesToRead = nBitArrayIndex * 4;
    if (m_nFurthestReadByte > 0)
    {
        int64 nFurthestReadBytes = m_nFurthestReadByte - m_pIO->GetPosition();
        if (nBytesToRead > nFurthestReadBytes)
        {
            nBytesToRead = nFurthestReadBytes;
            if (nBytesToRead < 0)
                nBytesToRead = 0;
        }
    }

    // read the new data
    unsigned int nBytesRead = 0;
    int nResult = m_pIO->Read((unsigned char *) (m_pBitArray + m_nElements - nBitArrayIndex), (unsigned int) nBytesToRead, &nBytesRead);

    // zero anything at the tail we didn't fill
    m_nGoodBytes = ((m_nElements - nBitArrayIndex) * 4) + nBytesRead;
    if (m_nGoodBytes < m_nBytes)
        memset(&((unsigned char *) m_pBitArray)[m_nGoodBytes], 0, m_nBytes - m_nGoodBytes);

    // adjust the m_Bit pointer
    m_nCurrentBitIndex = m_nCurrentBitIndex & 31;
    
    // return
    return (nResult == 0) ? 0 : ERROR_IO_READ;
}

int CUnBitArrayBase::CreateHelper(CIO * pIO, intn nBytes, intn nVersion)
{
    // check the parameters
    if ((pIO == NULL) || (nBytes <= 0)) { return ERROR_BAD_PARAMETER; }

    // save the size
    m_nElements = uint32(nBytes) / 4;
    m_nBytes = m_nElements * 4;
    m_nBits = m_nBytes * 8;
    m_nGoodBytes = 0;
    
    // set the variables
    m_pIO = pIO;
    m_nVersion = nVersion;
    m_nCurrentBitIndex = 0;
    
    // create the bitarray (we allocate and empty a little extra as buffer insurance, although it should never be necessary)
    m_pBitArray = new uint32 [m_nElements + 64];
    memset(m_pBitArray, 0, (m_nElements + 64) * sizeof(uint32));
    
    return (m_pBitArray != NULL) ? 0 : ERROR_INSUFFICIENT_MEMORY;
}

}