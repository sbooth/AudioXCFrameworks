#pragma once

#include "UnBitArrayBase.h"

namespace APE
{

class IAPEDecompress;

struct RANGE_CODER_STRUCT_DECOMPRESS
{
    unsigned int low;       // low end of interval
    unsigned int range;     // length of interval
    unsigned int buffer;    // buffer for input/output
    unsigned int padding;   // for 64-bit alignment
};

/**************************************************************************************************
RangeOverflowTable
**************************************************************************************************/
class RangeOverflowTable
{
public:
    RangeOverflowTable(const uint32 * RANGE_TOTAL);
    ~RangeOverflowTable();
    uint8 operator[](uint32 nIndex) const;

private:
    uint8 * m_pTable;
};

/**************************************************************************************************
CUnBitArray
**************************************************************************************************/
class CUnBitArray : public CUnBitArrayBase
{
public:
    // construction/destruction
    CUnBitArray(APE::CIO * pIO, intn nVersion, int64 nFurthestReadByte);
    ~CUnBitArray();

    uint32 DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1 = 0, int nParam2 = 0) APE_OVERRIDE;
    void GenerateArray(int * pOutputArray, int nElements, intn nBytesRequired) APE_OVERRIDE;
    int64 DecodeValueRange(UNBIT_ARRAY_STATE & BitArrayState) APE_OVERRIDE;
    void FlushState(UNBIT_ARRAY_STATE & BitArrayState) APE_OVERRIDE;
    void FlushBitArray() APE_OVERRIDE;
    void Finalize() APE_OVERRIDE;

private:
    // data
    int m_nFlushCounter;
    int m_nFinalizeCounter;
    RANGE_CODER_STRUCT_DECOMPRESS m_RangeCoderInfo;

    // functions
    inline uint32 DecodeByte();
    uint32 DecodeOverflow(uint32 & nPivotValue);
    uint32 RangeDecodeFast(int nShift);
    uint32 RangeDecodeFastWithUpdate(int nShift);
    void GenerateArrayRange(int * pOutputArray, int nElements);
};

}
