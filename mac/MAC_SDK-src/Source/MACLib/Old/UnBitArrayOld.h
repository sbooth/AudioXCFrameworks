#pragma once

#include "UnBitArrayBase.h"

namespace APE
{

class IAPEDecompress;

#pragma pack(push, 1)

/**************************************************************************************************
CUnBitArrayOld
// decodes 0000 up to and including 3890
**************************************************************************************************/
class CUnBitArrayOld : public CUnBitArrayBase
{
public:
    // construction/destruction
    CUnBitArrayOld(IAPEDecompress * pAPEDecompress, intn nVersion, int64 nFurthestReadByte);
    ~CUnBitArrayOld();

    // functions
    void GenerateArray(int * pOutputArray, int nElements, intn nBytesRequired = -1) APE_OVERRIDE;
    uint32 DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1 = 0) APE_OVERRIDE;

private:
    // helpers
    void GenerateArrayOld(int * pOutputArray, uint32 nNumberOfElements, int nMinimumBitArrayBytes);
    void GenerateArrayRice(int * pOutputArray, uint32 nNumberOfElements);
    uint32 DecodeValueRiceUnsigned(uint32 kl);

    // data
    uint32 m_nK;
    uint32 m_nKSum;
    uint32 m_nRefillBitThreshold;

    // functions
    __forceinline int DecodeValueNew(bool bCapOverflow);
    uint32 GetBitsRemaining() const;
    __forceinline uint32 Get_K(uint32 x);
};

/**************************************************************************************************
CUnBitArray3891To3989
**************************************************************************************************/
class CUnBitArray3891To3989 : public CUnBitArrayBase
{
public:
    // construction/destruction
    CUnBitArray3891To3989(APE::CIO * pIO, intn nVersion, int64 nFurthestReadByte);
    ~CUnBitArray3891To3989();

    void GenerateArray(int * pOutputArray, int nElements, intn nBytesRequired) APE_OVERRIDE;
    int64 DecodeValueRange(UNBIT_ARRAY_STATE & BitArrayState) APE_OVERRIDE;
    void FlushState(UNBIT_ARRAY_STATE & BitArrayState) APE_OVERRIDE;
    void FlushBitArray() APE_OVERRIDE;
    void Finalize() APE_OVERRIDE;

private:
    // data
    CSmartPtr<RangeOverflowTable> m_spRangeTable;
    RANGE_CODER_STRUCT_DECOMPRESS m_RangeCoderInfo;

    // functions
    uint32 RangeDecodeFast(int nShift);
    uint32 RangeDecodeFastWithUpdate(int nShift);
    void GenerateArrayRange(int * pOutputArray, int nElements);
};

#pragma pack(pop)

}
