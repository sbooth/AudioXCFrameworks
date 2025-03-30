/**************************************************************************************************
Includes
**************************************************************************************************/
#include "All.h"
#include "BitArray.h"

namespace APE
{

/**************************************************************************************************
Declares
**************************************************************************************************/
#define BIT_ARRAY_ELEMENTS            (4096)                      // the number of elements in the bit array (4 MB)
#define BIT_ARRAY_BYTES               (BIT_ARRAY_ELEMENTS * 4)    // the number of bytes in the bit array
#define BIT_ARRAY_BITS                (BIT_ARRAY_BYTES    * 8)    // the number of bits in the bit array

#define MAX_ELEMENT_BITS              128
#define REFILL_BIT_THRESHOLD          (BIT_ARRAY_BITS - MAX_ELEMENT_BITS)

#define CODE_BITS 32
#define TOP_VALUE (static_cast<unsigned int>(1) << (CODE_BITS - 1))
#define SHIFT_BITS (CODE_BITS - 9)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

#define OVERFLOW_SIGNAL 1
#define OVERFLOW_PIVOT_VALUE 32768

/**************************************************************************************************
Lookup tables
**************************************************************************************************/
#define MODEL_ELEMENTS                  64
#define RANGE_OVERFLOW_SHIFT            16

const uint32 RANGE_TOTAL[64] = {0,19578,36160,48417,56323,60899,63265,64435,64971,65232,65351,65416,65447,65466,65476,65482,65485,65488,65490,65491,65492,65493,65494,65495,65496,65497,65498,65499,65500,65501,65502,65503,65504,65505,65506,65507,65508,65509,65510,65511,65512,65513,65514,65515,65516,65517,65518,65519,65520,65521,65522,65523,65524,65525,65526,65527,65528,65529,65530,65531,65532,65533,65534,65535,};
const uint32 RANGE_WIDTH[64] = {19578,16582,12257,7906,4576,2366,1170,536,261,119,65,31,19,10,6,3,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};

#ifdef BUILD_RANGE_TABLE
    int g_aryOverflows[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int g_nTotalOverflow = 0;
#endif

/**************************************************************************************************
Constructor
**************************************************************************************************/
CBitArray::CBitArray(CIO * pIO)
{
    // allocate memory for the bit array
    m_spBitArray.Assign(new uint32[BIT_ARRAY_ELEMENTS], true);
    memset(m_spBitArray, 0, BIT_ARRAY_BYTES);

    // empty the range code information
    APE_CLEAR(m_RangeCoderInfo);

    // initialize other variables
    m_nCurrentBitIndex = 0;
    m_pIO = pIO;
}

/**************************************************************************************************
Destructor
**************************************************************************************************/
CBitArray::~CBitArray()
{
    // free the bit array
    m_spBitArray.Delete();
#ifdef BUILD_RANGE_TABLE
    OutputRangeTable();
#endif
}

/**************************************************************************************************
Output the bit array via the CIO (typically saves to disk)
**************************************************************************************************/
int CBitArray::OutputBitArray(bool bFinalize)
{
    // write the entire file to disk
    unsigned int nBytesWritten = 0;
    unsigned int nBytesToWrite = 0;

    if (bFinalize)
    {
        nBytesToWrite = ((m_nCurrentBitIndex >> 5) * 4) + 4;

        RETURN_ON_ERROR(m_pIO->Write(m_spBitArray, nBytesToWrite, &nBytesWritten))

        // reset the bit pointer and zero the array
        m_nCurrentBitIndex = 0;
        memset(m_spBitArray, 0, BIT_ARRAY_BYTES);
    }
    else
    {
        nBytesToWrite = (m_nCurrentBitIndex >> 5) * 4;

        RETURN_ON_ERROR(m_pIO->Write(m_spBitArray, nBytesToWrite, &nBytesWritten))

        // move the last value to the front of the bit array
        m_spBitArray[0] = m_spBitArray[m_nCurrentBitIndex >> 5];
        m_nCurrentBitIndex = (m_nCurrentBitIndex & 31);

        // zero the rest of the memory (may not need the +1 because of frame byte alignment)
        memset(&m_spBitArray[1], 0, static_cast<size_t>(ape_min(static_cast<int>(nBytesToWrite + 1), BIT_ARRAY_BYTES - 4)));
    }

    // return a success
    return ERROR_SUCCESS;
}

/**************************************************************************************************
Range coding macros -- ugly, but outperform inline's (every cycle counts here)
**************************************************************************************************/
#define PUTC(VALUE) m_spBitArray[m_nCurrentBitIndex >> 5] |= (static_cast<uint32>(VALUE) & 0xFF) << (24 - (m_nCurrentBitIndex & 31)); m_nCurrentBitIndex += 8;
#define PUTC_NOCAP(VALUE) m_spBitArray[m_nCurrentBitIndex >> 5] |= static_cast<uint32>(VALUE) << (24 - (m_nCurrentBitIndex & 31)); m_nCurrentBitIndex += 8;

#define NORMALIZE_RANGE_CODER                                                                    \
    while (m_RangeCoderInfo.range <= BOTTOM_VALUE)                                               \
    {                                                                                            \
        if (m_RangeCoderInfo.low < (0xFF << SHIFT_BITS))                                         \
        {                                                                                        \
            PUTC(m_RangeCoderInfo.buffer);                                                       \
            for ( ; m_RangeCoderInfo.help; m_RangeCoderInfo.help--) { PUTC_NOCAP(0xFF); }        \
            m_RangeCoderInfo.buffer = static_cast<unsigned char>(m_RangeCoderInfo.low >> static_cast<unsigned int>(SHIFT_BITS)); \
        }                                                                                        \
        else if (m_RangeCoderInfo.low & TOP_VALUE)                                               \
        {                                                                                        \
            PUTC(m_RangeCoderInfo.buffer + 1);                                                   \
            m_nCurrentBitIndex += (m_RangeCoderInfo.help * 8);                                   \
            m_RangeCoderInfo.help = 0;                                                           \
            m_RangeCoderInfo.buffer = static_cast<unsigned char>(m_RangeCoderInfo.low >> static_cast<unsigned int>(SHIFT_BITS)); \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            m_RangeCoderInfo.help++;                                                             \
        }                                                                                        \
                                                                                                 \
        m_RangeCoderInfo.low = (m_RangeCoderInfo.low << 8) & (TOP_VALUE - 1);                    \
        m_RangeCoderInfo.range <<= 8;                                                            \
    }

#define ENCODE_FAST(RANGE_WIDTH, RANGE_TOTAL, SHIFT)                                             \
    NORMALIZE_RANGE_CODER                                                                        \
    const unsigned int nTemp = static_cast<unsigned int>(m_RangeCoderInfo.range >> (SHIFT));     \
    m_RangeCoderInfo.range = nTemp * (RANGE_WIDTH);                                              \
    m_RangeCoderInfo.low += nTemp * (RANGE_TOTAL);

#define ENCODE_DIRECT(VALUE, SHIFT)                                                              \
    NORMALIZE_RANGE_CODER                                                                        \
    m_RangeCoderInfo.range = m_RangeCoderInfo.range >> (SHIFT);                                  \
    m_RangeCoderInfo.low += m_RangeCoderInfo.range * (VALUE);

/**************************************************************************************************
Directly encode bits to the bitstream
**************************************************************************************************/
int CBitArray::EncodeBits(unsigned int nValue, int nBits)
{
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (m_nCurrentBitIndex > REFILL_BIT_THRESHOLD)
    {
        RETURN_ON_ERROR(OutputBitArray())
    }

    ENCODE_DIRECT(nValue, nBits)
    return ERROR_SUCCESS;
}

/**************************************************************************************************
Encodes an unsigned int to the bit array (no rice coding)
**************************************************************************************************/
int CBitArray::EncodeUnsignedLong(unsigned int n)
{
    // make sure there are at least 8 bytes in the buffer
    if (m_nCurrentBitIndex > (BIT_ARRAY_BYTES - 8))
    {
        RETURN_ON_ERROR(OutputBitArray())
    }

    // encode the value
    uint32 nBitArrayIndex = m_nCurrentBitIndex >> 5;
    const int nBitIndex = static_cast<int>(m_nCurrentBitIndex & 31);

    if (nBitIndex == 0)
    {
        m_spBitArray[nBitArrayIndex] = n;
    }
    else
    {
        m_spBitArray[nBitArrayIndex] |= n >> nBitIndex;
        m_spBitArray[nBitArrayIndex + 1] = n << (32 - nBitIndex);
    }

    m_nCurrentBitIndex += 32;

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Advance to a byte boundary (for frame alignment)
**************************************************************************************************/
void CBitArray::AdvanceToByteBoundary()
{
    while (m_nCurrentBitIndex % 8)
        m_nCurrentBitIndex++;
}

/**************************************************************************************************
Encode a value
**************************************************************************************************/
int CBitArray::EncodeValue(int64 nEncode, BIT_ARRAY_STATE & BitArrayState)
{
    // make sure there is room for the data
    // this is a little slower than ensuring a huge block to start with, but it's safer
    if (m_nCurrentBitIndex > REFILL_BIT_THRESHOLD)
    {
        RETURN_ON_ERROR(OutputBitArray())
    }

    // convert to unsigned
    nEncode = (nEncode > 0) ? nEncode * 2 - 1 : -nEncode * 2;

    // figure the pivot value
    uint32 nPivotValue = ape_max(BitArrayState.nKSum / 32, static_cast<uint32>(1));
    const uint64 nOverflow64 = static_cast<uint64>(nEncode / nPivotValue);
    uint32 nOverflow = static_cast<uint32>(nOverflow64);
    if (nOverflow != nOverflow64)
    {
        // overflow happened; increase the pivot value and signal the situation in the bitstream
        nPivotValue = OVERFLOW_PIVOT_VALUE;
        nOverflow = static_cast<uint32>(nEncode / nPivotValue);

        // signal overflow using a low overflow value that can never occur as a "special" overflow value otherwise
        ENCODE_FAST(RANGE_WIDTH[MODEL_ELEMENTS - 1], RANGE_TOTAL[MODEL_ELEMENTS - 1], RANGE_OVERFLOW_SHIFT)
        ENCODE_DIRECT((OVERFLOW_SIGNAL >> 16) & 0xFFFF, 16)
        ENCODE_DIRECT(OVERFLOW_SIGNAL & 0xFFFF, 16)
    }
    const uint32 nBase = static_cast<uint32>(nEncode - (static_cast<int64>(nOverflow) * static_cast<int64>(nPivotValue)));

    // update nKSum
    // this was switched to int64 on the nKsum in build 8.68, but that made 32-bit noise encodes that failed to verify since it wasn't done on the other side
    BitArrayState.nKSum += static_cast<uint32>((nEncode + 1) / 2) - ((BitArrayState.nKSum + 16) >> 5);

    // store the overflow
    if (nOverflow < (MODEL_ELEMENTS - 1))
    {
        ENCODE_FAST(RANGE_WIDTH[nOverflow], RANGE_TOTAL[nOverflow], RANGE_OVERFLOW_SHIFT)

        #ifdef BUILD_RANGE_TABLE
            g_aryOverflows[nOverflow]++;
            g_nTotalOverflow++;
        #endif
    }
    else
    {
        // store the "special" overflow (tells that perfect k is encoded next)
        ENCODE_FAST(RANGE_WIDTH[MODEL_ELEMENTS - 1], RANGE_TOTAL[MODEL_ELEMENTS - 1], RANGE_OVERFLOW_SHIFT)

        #ifdef BUILD_RANGE_TABLE
            g_aryOverflows[MODEL_ELEMENTS - 1]++;
            g_nTotalOverflow++;
        #endif

        // code the overflow using straight bits
        ENCODE_DIRECT((nOverflow >> 16) & 0xFFFF, 16)
        ENCODE_DIRECT(nOverflow & 0xFFFF, 16)
    }

    // code the base
    if (nPivotValue >= (1 << 16))
    {
        uint32 nPivotValueBits = 0;
        while ((nPivotValue >> nPivotValueBits) > 0) { nPivotValueBits++; }
        const uint32 nShift = (nPivotValueBits >= 16) ? nPivotValueBits - 16 : 0;
        const uint32 nSplitFactor = static_cast<uint32>(1) << nShift;

        // we know that base is smaller than pivot coming into this
        // however, after we divide both by an integer, they could be the same
        // we account by adding one to the pivot, but this hurts compression
        // by (1 / nSplitFactor) -- therefore we maximize the split factor
        // that gets one added to it

        // encode the pivot as two pieces
        const uint32 nPivotValueA = (nPivotValue / nSplitFactor) + 1;
        const uint32 nPivotValueB = nSplitFactor;

        const uint32 nBaseA = nBase / nSplitFactor;
        const uint32 nBaseB = nBase % nSplitFactor;

        {
            NORMALIZE_RANGE_CODER
            const uint32 nTemp = m_RangeCoderInfo.range / nPivotValueA;
            m_RangeCoderInfo.range = nTemp;
            m_RangeCoderInfo.low += nTemp * nBaseA;
        }

        {
            NORMALIZE_RANGE_CODER
            const uint32 nTemp = m_RangeCoderInfo.range / nPivotValueB;
            m_RangeCoderInfo.range = nTemp;
            m_RangeCoderInfo.low += nTemp * nBaseB;
        }
    }
    else
    {
        NORMALIZE_RANGE_CODER
        const uint32 nTemp = m_RangeCoderInfo.range / nPivotValue;
        m_RangeCoderInfo.range = nTemp;
        m_RangeCoderInfo.low += nTemp * nBase;
    }

    return ERROR_SUCCESS;
}

/**************************************************************************************************
Flush
**************************************************************************************************/
void CBitArray::FlushBitArray()
{
    // advance to a byte boundary (for alignment)
    AdvanceToByteBoundary();

    // the range coder
    m_RangeCoderInfo.low = 0;  // full code range
    m_RangeCoderInfo.range = TOP_VALUE;
    m_RangeCoderInfo.buffer = 0;
    m_RangeCoderInfo.help = 0;  // no bytes to follow
}

void CBitArray::FlushState(BIT_ARRAY_STATE & BitArrayState)
{
    // ksum
    BitArrayState.nKSum = (1 << 10) * 16;
}

/**************************************************************************************************
Finalize
**************************************************************************************************/
void CBitArray::Finalize()
{
    NORMALIZE_RANGE_CODER

    const unsigned int nTemp = (m_RangeCoderInfo.low >> SHIFT_BITS) + 1;

    if (nTemp > 0xFF) // we have a carry
    {
        PUTC(m_RangeCoderInfo.buffer + 1)
        for ( ; m_RangeCoderInfo.help; m_RangeCoderInfo.help--)
        {
            PUTC(0)
        }
    }
    else  // no carry
    {
        PUTC(m_RangeCoderInfo.buffer)
        for ( ; m_RangeCoderInfo.help; m_RangeCoderInfo.help--)
        {
            PUTC((static_cast<unsigned char>(0xFF)))
        }
    }

    // we must output these bytes so the decoder can properly work at the end of the stream
    PUTC(nTemp & 0xFF)
    PUTC(0)
    PUTC(0)
    PUTC(0)
}

/**************************************************************************************************
Build a range table (for development / debugging)
**************************************************************************************************/
#ifdef BUILD_RANGE_TABLE
void CBitArray::OutputRangeTable()
{
    int z;

    if (g_nTotalOverflow == 0) return;

    int nTotal = 0;
    int aryWidth[256]; APE_CLEAR(aryWidth);
    for (z = 0; z < MODEL_ELEMENTS; z++)
    {
        aryWidth[z] = static_cast<int>(((float(g_aryOverflows[z]) * float(65536)) + (g_nTotalOverflow / 2)) / float(g_nTotalOverflow));
        if (aryWidth[z] == 0) aryWidth[z] = 1;
        nTotal += aryWidth[z];
    }

    z = 0;
    while (nTotal > 65536)
    {
        if (aryWidth[z] != 1)
        {
            aryWidth[z]--;
            nTotal--;
        }
        z++;
        if (z == MODEL_ELEMENTS) z = 0;
    }

    z = 0;
    while (nTotal < 65536)
    {
        aryWidth[z++]++;
        nTotal++;
        if (z == MODEL_ELEMENTS) z = 0;
    }

    int aryTotal[256]; APE_CLEAR(aryTotal);
    for (z = 0; z < MODEL_ELEMENTS; z++)
    {
        for (int q = 0; q < z; q++)
        {
            aryTotal[z] += aryWidth[q];
        }
    }

    TCHAR buf[1024];
    _stprintf(buf, _T("const uint32 RANGE_TOTAL[%d] = {"), MODEL_ELEMENTS);
    APE_ODS(buf);
    for (z = 0; z < MODEL_ELEMENTS; z++)
    {
        _stprintf(buf, _T("%d,"), aryTotal[z]);
        OutputDebugString(buf);
    }
    APE_ODS(_T("};\n"));

    _stprintf(buf, _T("const uint32 RANGE_WIDTH[%d] = {"), MODEL_ELEMENTS);
    APE_ODS(buf);
    for (z = 0; z < MODEL_ELEMENTS; z++)
    {
        _stprintf(buf, _T("%d,"), aryWidth[z]);
        OutputDebugString(buf);
    }
    APE_ODS(_T("};\n\n"));
}
#endif // #ifdef BUILD_RANGE_TABLE

}
