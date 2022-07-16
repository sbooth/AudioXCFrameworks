#pragma once

namespace APE
{

class IAPEDecompress;
class CIO;

struct UNBIT_ARRAY_STATE
{
    uint32 k;
    uint32 nKSum;
};

class CUnBitArrayBase
{
public:

    // enumeration
    enum DECODE_VALUE_METHOD
    {
        DECODE_VALUE_METHOD_UNSIGNED_INT,
        DECODE_VALUE_METHOD_UNSIGNED_RICE,
        DECODE_VALUE_METHOD_X_BITS
    };

    // construction / destruction
    CUnBitArrayBase(int64 nFurthestReadByte);
    virtual ~CUnBitArrayBase();
    
    // functions
    virtual int FillBitArray();
    virtual int FillAndResetBitArray(int64 nFileLocation = -1, int64 nNewBitIndex = 0);
        
    virtual void GenerateArray(int* pOutputArray, int nElements, intn nBytesRequired) = 0;
    virtual uint32 DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1 = 0, int nParam2 = 0) = 0;
    
    virtual void AdvanceToByteBoundary();
    virtual bool EnsureBitsAvailable(uint32 nBits, bool bThrowExceptionOnFailure);

    virtual int64 DecodeValueRange(UNBIT_ARRAY_STATE & BitArrayState) { (void) BitArrayState; return 0; }
    virtual void FlushState(UNBIT_ARRAY_STATE & BitArrayState) { (void) BitArrayState; }
    virtual void FlushBitArray() { }
    virtual void Finalize() { }
    
protected:
    virtual int CreateHelper(CIO * pIO, intn nBytes, intn nVersion);
    virtual uint32 DecodeValueXBits(uint32 nBits);
    
    uint32 m_nElements;
    uint32 m_nBytes;
    uint32 m_nBits;
    uint32 m_nGoodBytes;
    
    intn m_nVersion;
    CIO * m_pIO;
    int64 m_nFurthestReadByte;

    uint32 m_nCurrentBitIndex;
    uint32 * m_pBitArray;
};

CUnBitArrayBase * CreateUnBitArray(IAPEDecompress * pAPEDecompress, intn nVersion);

}
