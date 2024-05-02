#pragma once

#include "IO.h"
#include "MD5.h"

namespace APE
{

//#define BUILD_RANGE_TABLE

struct RANGE_CODER_STRUCT_COMPRESS
{
    unsigned int low;         // low end of interval
    unsigned int range;       // length of interval
    unsigned int help;        // bytes_to_follow resp. intermediate value
    unsigned char buffer;     // buffer for input / output
    unsigned char padding[3]; // buffer alignment
};

struct BIT_ARRAY_STATE
{
    uint32 nKSum;
};

#pragma pack(push, 1)

class CBitArray
{
public:
    // construction / destruction
    CBitArray(APE::CIO * pIO);
    virtual ~CBitArray();

    // encoding
    int EncodeUnsignedLong(unsigned int n);
    int EncodeValue(int64 nEncode, BIT_ARRAY_STATE & BitArrayState);
    int EncodeBits(unsigned int nValue, int nBits);

    // output (saving)
    int OutputBitArray(bool bFinalize = false);

    // other functions
    void Finalize();
    void AdvanceToByteBoundary();
#ifdef APE_ENABLE_BIT_ARRAY_INLINES
    __forceinline uint32 GetCurrentBitIndex() { return m_nCurrentBitIndex; }
    __forceinline CMD5Helper& GetMD5Helper() { return m_MD5; }
#endif
    void FlushState(BIT_ARRAY_STATE & BitArrayState);
    void FlushBitArray();

private:
    // data members
    CSmartPtr<uint32> m_spBitArray;
    CIO * m_pIO;
    uint32 m_nCurrentBitIndex;
    RANGE_CODER_STRUCT_COMPRESS m_RangeCoderInfo;
    CMD5Helper m_MD5;

#ifdef BUILD_RANGE_TABLE
    void OutputRangeTable();
#endif
};

#pragma pack(pop)

}
