#include "All.h"
#include "APEHeader.h"
#include "APEInfo.h"

#define WAV_HEADER_SANITY (1024*1024) // no WAV header should be larger than 1MB, do not even try to read if larger

namespace APE
{

CAPEHeader::CAPEHeader(CIO * pIO)
{
    m_pIO = pIO;
}

CAPEHeader::~CAPEHeader()
{
}

int CAPEHeader::FindDescriptor(bool bSeek)
{
    // store the original location and seek to the beginning
    int64 nOriginalFileLocation = m_pIO->GetPosition();
    m_pIO->SetSeekMethod(APE_FILE_BEGIN);
    m_pIO->SetSeekPosition(0);
    m_pIO->PerformSeek();

    // set the default junk bytes to 0
    int nJunkBytes = 0;

    // skip an ID3v2 tag (which we really don't support anyway...)
    unsigned int nBytesRead = 0; 
    unsigned char cID3v2Header[10] = { 0 };
    m_pIO->Read((unsigned char *) cID3v2Header, 10, &nBytesRead);
    if (cID3v2Header[0] == 'I' && cID3v2Header[1] == 'D' && cID3v2Header[2] == '3') 
    {
        // why is it so hard to figure the length of an ID3v2 tag ?!?
        unsigned int nSyncSafeLength = 0;
        nSyncSafeLength = (cID3v2Header[6] & 127) << 21;
        nSyncSafeLength += (cID3v2Header[7] & 127) << 14;
        nSyncSafeLength += (cID3v2Header[8] & 127) << 7;
        nSyncSafeLength += (cID3v2Header[9] & 127);

        bool bHasTagFooter = false;

        if (cID3v2Header[5] & 16) 
        {
            bHasTagFooter = true;
            nJunkBytes = nSyncSafeLength + 20;
        }
        else 
        {
            nJunkBytes = nSyncSafeLength + 10;
        }

        // error check
        if (cID3v2Header[5] & 64)
        {
            // this ID3v2 length calculator algorithm can't cope with extended headers
            // we should be ok though, because the scan for the MAC header below should
            // really do the trick
        }

        m_pIO->SetSeekMethod(APE_FILE_BEGIN);
        m_pIO->SetSeekPosition(nJunkBytes);
        m_pIO->PerformSeek();

        // scan for padding (slow and stupid, but who cares here...)
        if (!bHasTagFooter)
        {
            char cTemp = 0;
            m_pIO->Read((unsigned char *) &cTemp, 1, &nBytesRead);
            while (cTemp == 0 && nBytesRead == 1)
            {
                nJunkBytes++;
                m_pIO->Read((unsigned char *) &cTemp, 1, &nBytesRead);
            }
        }
    }
    m_pIO->SetSeekMethod(APE_FILE_BEGIN);
    m_pIO->SetSeekPosition(nJunkBytes);
    m_pIO->PerformSeek();

    // scan until we hit the APE_DESCRIPTOR, the end of the file, or 1 MB later
    unsigned int nGoalID = (' ' << 24) | ('C' << 16) | ('A' << 8) | ('M');
    unsigned int nReadID = 0;
    int nResult = m_pIO->Read(&nReadID, 4, &nBytesRead);
    if (nResult != 0 || nBytesRead != 4) return ERROR_UNDEFINED;

    nBytesRead = 1;
    int nScanBytes = 0;
    while ((nGoalID != nReadID) && (nBytesRead == 1) && (nScanBytes < (1024 * 1024)))
    {
        unsigned char cTemp = 0;
        m_pIO->Read(&cTemp, 1, &nBytesRead);
        nReadID = (((unsigned int) cTemp) << 24) | (nReadID >> 8);
        nJunkBytes++;
        nScanBytes++;
    }

    if (nGoalID != nReadID)
        nJunkBytes = -1;

    // seek to the proper place (depending on result and settings)
    if (bSeek && (nJunkBytes != -1))
    {
        // successfully found the start of the file (seek to it and return)
        m_pIO->SetSeekMethod(APE_FILE_BEGIN);
        m_pIO->SetSeekPosition(nJunkBytes);
        m_pIO->PerformSeek();
    }
    else
    {
        // restore the original file pointer
        m_pIO->SetSeekMethod(APE_FILE_BEGIN);
        m_pIO->SetSeekPosition(nOriginalFileLocation);
        m_pIO->PerformSeek();
    }

    return nJunkBytes;
}

void CAPEHeader::Convert32BitSeekTable(APE_FILE_INFO * pInfo, uint32 * pSeekTable32, int nSeekTableElements)
{
    pInfo->spSeekByteTable64.Assign(new int64 [nSeekTableElements], true);
    int64 nSeekAdd = 0;
    for (int z = 0; z < pInfo->nSeekTableElements; z++)
    {
        if ((z > 0) && (pSeekTable32[z] < pSeekTable32[z - 1]))
            nSeekAdd += int64(0xFFFFFFFF) + int64(1);

        pInfo->spSeekByteTable64[z] = nSeekAdd + pSeekTable32[z];
    }
}

int CAPEHeader::Analyze(APE_FILE_INFO * pInfo)
{
    // error check
    if ((m_pIO == NULL) || (pInfo == NULL))
        return ERROR_BAD_PARAMETER;

    // variables
    unsigned int nBytesRead = 0;

    // find the descriptor
    pInfo->nJunkHeaderBytes = FindDescriptor(true);
    if (pInfo->nJunkHeaderBytes < 0)
        return ERROR_UNDEFINED;

    // read the first 8 bytes of the descriptor (ID and version)
    APE_COMMON_HEADER CommonHeader; memset(&CommonHeader, 0, sizeof(APE_COMMON_HEADER));
    if (m_pIO->Read(&CommonHeader, sizeof(APE_COMMON_HEADER), &nBytesRead) || nBytesRead != sizeof(APE_COMMON_HEADER))
        return ERROR_IO_READ;

    // make sure we're at the ID
    if (CommonHeader.cID[0] != 'M' || CommonHeader.cID[1] != 'A' || CommonHeader.cID[2] != 'C' || CommonHeader.cID[3] != ' ')
        return ERROR_UNDEFINED;

    int nResult = ERROR_UNDEFINED;

    if (CommonHeader.nVersion >= 3980)
    {
        // current header format
        nResult = AnalyzeCurrent(pInfo);
    }
    else
    {
        // legacy support
        nResult = AnalyzeOld(pInfo);
    }

    // check for invalid channels
    if ((pInfo->nChannels > APE_MAXIMUM_CHANNELS) || (pInfo->nChannels < APE_MINIMUM_CHANNELS))
    {
        return ERROR_INVALID_INPUT_FILE;
    }

    return nResult;
}

int CAPEHeader::AnalyzeCurrent(APE_FILE_INFO * pInfo)
{
    // variable declares
    unsigned int nBytesRead = 0;
    pInfo->spAPEDescriptor.Assign(new APE_DESCRIPTOR); memset(pInfo->spAPEDescriptor, 0, sizeof(APE_DESCRIPTOR));
    APE_HEADER APEHeader; memset(&APEHeader, 0, sizeof(APEHeader));

    // read the descriptor
    m_pIO->SetSeekMethod(APE_FILE_BEGIN);
    m_pIO->SetSeekPosition(pInfo->nJunkHeaderBytes);
    m_pIO->PerformSeek();
    if (m_pIO->Read(pInfo->spAPEDescriptor, sizeof(APE_DESCRIPTOR), &nBytesRead) || nBytesRead != sizeof(APE_DESCRIPTOR))
        return ERROR_IO_READ;

    if ((pInfo->spAPEDescriptor->nDescriptorBytes - nBytesRead) > 0)
    {
        m_pIO->SetSeekMethod(APE_FILE_CURRENT);
        m_pIO->SetSeekPosition(pInfo->spAPEDescriptor->nDescriptorBytes - nBytesRead);
        m_pIO->PerformSeek();
    }

    // read the header
    if (m_pIO->Read(&APEHeader, sizeof(APEHeader), &nBytesRead) || nBytesRead != sizeof(APEHeader))
        return ERROR_IO_READ;

    if ((pInfo->spAPEDescriptor->nHeaderBytes - nBytesRead) > 0)
    {
        m_pIO->SetSeekMethod(APE_FILE_CURRENT);
        m_pIO->SetSeekPosition(pInfo->spAPEDescriptor->nHeaderBytes - nBytesRead);
        m_pIO->PerformSeek();
    }

    // fill the APE info structure
    pInfo->nVersion               = int(pInfo->spAPEDescriptor->nVersion);
    pInfo->nCompressionLevel      = int(APEHeader.nCompressionLevel);
    pInfo->nFormatFlags           = int(APEHeader.nFormatFlags);
    pInfo->nTotalFrames           = int(APEHeader.nTotalFrames);
    pInfo->nFinalFrameBlocks      = int(APEHeader.nFinalFrameBlocks);
    pInfo->nBlocksPerFrame        = int(APEHeader.nBlocksPerFrame);
    pInfo->nChannels              = int(APEHeader.nChannels);
    pInfo->nSampleRate            = int(APEHeader.nSampleRate);
    pInfo->nBitsPerSample         = int(APEHeader.nBitsPerSample);
    pInfo->nBytesPerSample        = pInfo->nBitsPerSample / 8;
    pInfo->nBlockAlign            = pInfo->nBytesPerSample * pInfo->nChannels;
    pInfo->nTotalBlocks           = (APEHeader.nTotalFrames == 0) ? 0 : (int64(APEHeader.nTotalFrames -  1) * int64(pInfo->nBlocksPerFrame)) + int64(APEHeader.nFinalFrameBlocks);

    pInfo->nWAVDataBytes = int64(pInfo->nTotalBlocks) * int64(pInfo->nBlockAlign);
    pInfo->nWAVTotalBytes = pInfo->nWAVDataBytes + pInfo->nWAVHeaderBytes + pInfo->nWAVTerminatingBytes;
    pInfo->nAPETotalBytes = m_pIO->GetSize();

    int nWAVHeaderSize = sizeof(WAVE_HEADER);
    if (pInfo->nWAVDataBytes >= (BYTES_IN_GIGABYTE * 4))
        nWAVHeaderSize = sizeof(RF64_HEADER);
    pInfo->nWAVHeaderBytes        = (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER) ? nWAVHeaderSize : pInfo->spAPEDescriptor->nHeaderDataBytes;
    pInfo->nWAVTerminatingBytes   = pInfo->spAPEDescriptor->nTerminatingDataBytes;
    pInfo->nLengthMS              = int((double(pInfo->nTotalBlocks) * double(1000)) / double(pInfo->nSampleRate));
    pInfo->nAverageBitrate        = (pInfo->nLengthMS <= 0) ? 0 : int((double(pInfo->nAPETotalBytes) * double(8)) / double(pInfo->nLengthMS));
    pInfo->nDecompressedBitrate   = (pInfo->nBlockAlign * pInfo->nSampleRate * 8) / 1000;
    pInfo->nSeekTableElements     = pInfo->spAPEDescriptor->nSeekTableBytes / 4;
    pInfo->nMD5Invalid            = false;

    // check for nonsense in nSeekTableElements field
    if ((unsigned)pInfo->nSeekTableElements > (unsigned)pInfo->nAPETotalBytes / 4)
    {
        ASSERT(0);
        return ERROR_INVALID_INPUT_FILE;
    }

    // get the seek tables (really no reason to get the whole thing if there's extra)
    CSmartPtr<uint32> spSeekByteTable32;
    spSeekByteTable32.Assign(new uint32 [pInfo->nSeekTableElements], true);
    if (spSeekByteTable32 == NULL) { return ERROR_UNDEFINED; }

    if (m_pIO->Read((unsigned char *) spSeekByteTable32.GetPtr(), 4 * pInfo->nSeekTableElements, &nBytesRead) || nBytesRead != 4 * (unsigned) pInfo->nSeekTableElements)
        return ERROR_IO_READ;

    // convert to int64
    Convert32BitSeekTable(pInfo, spSeekByteTable32, pInfo->nSeekTableElements);

    // get the wave header
    if (!(APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
    {
        if (pInfo->nWAVHeaderBytes < 0 || pInfo->nWAVHeaderBytes > WAV_HEADER_SANITY)
        {
            return ERROR_INVALID_INPUT_FILE;
        }
        pInfo->spWaveHeaderData.Assign(new unsigned char [(size_t) pInfo->nWAVHeaderBytes], true);
        if (pInfo->spWaveHeaderData == NULL) { return ERROR_UNDEFINED; }
        if (m_pIO->Read((unsigned char *) pInfo->spWaveHeaderData, (unsigned int) pInfo->nWAVHeaderBytes, &nBytesRead) || nBytesRead != pInfo->nWAVHeaderBytes)
            return ERROR_IO_READ;
    }


    // check for an invalid blocks per frame
    if (pInfo->nBlocksPerFrame <= 0)
        return ERROR_INVALID_INPUT_FILE;

    if (pInfo->nCompressionLevel >= 5000)
    {
        if (pInfo->nBlocksPerFrame > (10 * ONE_MILLION))
            return ERROR_INVALID_INPUT_FILE;
    }
    else
    {
        if (pInfo->nBlocksPerFrame > ONE_MILLION)
            return ERROR_INVALID_INPUT_FILE;
    }

    // check the final frame size being nonsense
    if (APEHeader.nFinalFrameBlocks > pInfo->nBlocksPerFrame)
        return ERROR_INVALID_INPUT_FILE;

    return ERROR_SUCCESS;
}

int CAPEHeader::AnalyzeOld(APE_FILE_INFO * pInfo)
{
    // variable declares
    unsigned int nBytesRead = 0;

    // read the MAC header from the file
    APE_HEADER_OLD APEHeader;

    m_pIO->SetSeekMethod(APE_FILE_BEGIN);
    m_pIO->SetSeekPosition(pInfo->nJunkHeaderBytes);
    m_pIO->PerformSeek();

    if (m_pIO->Read((unsigned char *) &APEHeader, sizeof(APEHeader), &nBytesRead) || nBytesRead != sizeof(APEHeader))
        return ERROR_IO_READ;

    // fail on 0 length APE files (catches non-finalized APE files)
    if (APEHeader.nTotalFrames == 0)
        return ERROR_UNDEFINED;

    int nPeakLevel = -1;
    if (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
        m_pIO->Read((unsigned char *) &nPeakLevel, 4, &nBytesRead);

    if (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
    {
        if (m_pIO->Read((unsigned char *) &pInfo->nSeekTableElements, 4, &nBytesRead) || nBytesRead != 4)
            return ERROR_IO_READ;
    }
    else
        pInfo->nSeekTableElements = APEHeader.nTotalFrames;
    
    // fill the APE info structure
    pInfo->nVersion               = int(APEHeader.nVersion);
    pInfo->nCompressionLevel      = int(APEHeader.nCompressionLevel);
    pInfo->nFormatFlags           = int(APEHeader.nFormatFlags);
    pInfo->nTotalFrames           = int(APEHeader.nTotalFrames);
    pInfo->nFinalFrameBlocks      = int(APEHeader.nFinalFrameBlocks);
    pInfo->nBlocksPerFrame        = ((APEHeader.nVersion >= 3900) || ((APEHeader.nVersion >= 3800) && (APEHeader.nCompressionLevel == MAC_COMPRESSION_LEVEL_EXTRA_HIGH))) ? 73728 : 9216;
    if ((APEHeader.nVersion >= 3950)) pInfo->nBlocksPerFrame = 73728 * 4;
    pInfo->nChannels              = int(APEHeader.nChannels);
    pInfo->nSampleRate            = int(APEHeader.nSampleRate);
    pInfo->nBitsPerSample         = (pInfo->nFormatFlags & MAC_FORMAT_FLAG_8_BIT) ? 8 : ((pInfo->nFormatFlags & MAC_FORMAT_FLAG_24_BIT) ? 24 : 16);
    pInfo->nBytesPerSample        = pInfo->nBitsPerSample / 8;
    pInfo->nBlockAlign            = pInfo->nBytesPerSample * pInfo->nChannels;
    pInfo->nTotalBlocks           = (APEHeader.nTotalFrames == 0) ? 0 : (int64(APEHeader.nTotalFrames -  1) * int64(pInfo->nBlocksPerFrame)) + int64(APEHeader.nFinalFrameBlocks);
    pInfo->nWAVHeaderBytes        = (APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER) ? sizeof(WAVE_HEADER) : APEHeader.nHeaderBytes;
    pInfo->nWAVTerminatingBytes   = int(APEHeader.nTerminatingBytes);
    pInfo->nWAVDataBytes          = pInfo->nTotalBlocks * pInfo->nBlockAlign;
    pInfo->nWAVTotalBytes         = pInfo->nWAVDataBytes + pInfo->nWAVHeaderBytes + pInfo->nWAVTerminatingBytes;
    pInfo->nAPETotalBytes         = m_pIO->GetSize();
    pInfo->nLengthMS              = int((double(pInfo->nTotalBlocks) * double(1000)) / double(pInfo->nSampleRate));
    pInfo->nAverageBitrate        = (pInfo->nLengthMS <= 0) ? 0 : int((double(pInfo->nAPETotalBytes) * double(8)) / double(pInfo->nLengthMS));
    pInfo->nDecompressedBitrate   = (pInfo->nBlockAlign * pInfo->nSampleRate * 8) / 1000;
    pInfo->nMD5Invalid            = false;

    // check for an invalid blocks per frame
    if (pInfo->nBlocksPerFrame > (10 * ONE_MILLION) || pInfo->nBlocksPerFrame <= 0)
        return ERROR_INVALID_INPUT_FILE;

    // check the final frame size being nonsense
    if (APEHeader.nFinalFrameBlocks > pInfo->nBlocksPerFrame)
        return ERROR_INVALID_INPUT_FILE;

    // check for nonsense in nSeekTableElements field
    if ((unsigned)pInfo->nSeekTableElements > (unsigned)(pInfo->nAPETotalBytes / 4))
    {
        ASSERT(0);
        return ERROR_INVALID_INPUT_FILE;
    }

    // get the wave header
    if (!(APEHeader.nFormatFlags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
    {
        if (APEHeader.nHeaderBytes > WAV_HEADER_SANITY) return ERROR_INVALID_INPUT_FILE;
        if (m_pIO->GetPosition() + APEHeader.nHeaderBytes > m_pIO->GetSize()) { return ERROR_UNDEFINED; }
        pInfo->spWaveHeaderData.Assign(new unsigned char [APEHeader.nHeaderBytes], true);
        if (pInfo->spWaveHeaderData == NULL) { return ERROR_UNDEFINED; }
        if (m_pIO->Read((unsigned char *) pInfo->spWaveHeaderData, APEHeader.nHeaderBytes, &nBytesRead) || nBytesRead != APEHeader.nHeaderBytes)
            return ERROR_IO_READ;
    }

    // get the seek tables (really no reason to get the whole thing if there's extra)
    CSmartPtr<uint32> spSeekByteTable32;
    spSeekByteTable32.Assign(new uint32[pInfo->nSeekTableElements], true);
    if (spSeekByteTable32 == NULL) { return ERROR_UNDEFINED; }

    if (m_pIO->Read((unsigned char *) spSeekByteTable32.GetPtr(), 4 * pInfo->nSeekTableElements, &nBytesRead) || nBytesRead != 4 * (unsigned)pInfo->nSeekTableElements)
        return ERROR_IO_READ;

    // convert to int64
    Convert32BitSeekTable(pInfo, spSeekByteTable32, pInfo->nSeekTableElements);

    // seek bit table (for older files)
    if (APEHeader.nVersion <= 3800) 
    {
        pInfo->spSeekBitTable.Assign(new unsigned char [pInfo->nSeekTableElements], true);
        if (pInfo->spSeekBitTable == NULL) { return ERROR_UNDEFINED; }

        if (m_pIO->Read((unsigned char *) pInfo->spSeekBitTable, pInfo->nSeekTableElements, &nBytesRead) || nBytesRead != (unsigned)pInfo->nSeekTableElements)
            return ERROR_IO_READ;
    }

    return ERROR_SUCCESS;
}

}