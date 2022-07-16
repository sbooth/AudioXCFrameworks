#pragma once

#include "../APEDecompress.h"
#include "UnMAC.h"

namespace APE
{

class CAPEDecompressOld : public IAPEDecompress
{
public:
    CAPEDecompressOld(int * pErrorCode, CAPEInfo * pAPEInfo, int nStartBlock = -1, int nFinishBlock = -1);
    ~CAPEDecompressOld();

    int GetData(char * pBuffer, int64 nBlocks, int64 * pBlocksRetrieved);
    int Seek(int64 nBlockOffset);

    int64 GetInfo(APE_DECOMPRESS_FIELDS Field, int64 nParam1 = 0, int64 nParam2 = 0);
    
protected:
    // buffer
    CSmartPtr<char> m_spBuffer;
    int64 m_nBufferTail;
    
    // file info
    int64 m_nBlockAlign;
    int64 m_nCurrentFrame;

    // start / finish information
    int64 m_nStartBlock;
    int64 m_nFinishBlock;
    int64 m_nCurrentBlock;
    bool m_bIsRanged;

    // decoding tools    
    CUnMAC m_UnMAC;
    CSmartPtr<CAPEInfo> m_spAPEInfo;
    
    bool m_bDecompressorInitialized;
    int InitializeDecompressor();
};

}

