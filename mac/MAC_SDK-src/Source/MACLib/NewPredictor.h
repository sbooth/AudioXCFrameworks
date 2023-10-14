#pragma once

#include "Predictor.h"
#include "RollBuffer.h"
#include "NNFilter.h"
#include "ScaledFirstOrderFilter.h"

namespace APE
{

#pragma pack(push, 1)

/**************************************************************************************************
Functions to create the interfaces
**************************************************************************************************/
#define WINDOW_BLOCKS           256
#define M_COUNT                 8

template <class INTTYPE, class DATATYPE> class CPredictorCompressNormal : public IPredictorCompress
{
public:
    CPredictorCompressNormal(int nCompressionLevel, int nBitsPerSample);
    virtual ~CPredictorCompressNormal() APE_OVERRIDE;

    int64 CompressValue(int nA, int nB = 0) APE_OVERRIDE;
    int Flush() APE_OVERRIDE;

protected:
    // buffer information
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 10> m_rbPrediction;
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 9> m_rbAdapt;

    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterA;
    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterB;

    // other
    int m_nCurrentIndex;
    int m_nBitsPerSample;
    CSmartPtr< CNNFilter<INTTYPE, DATATYPE> > m_spNNFilter;
    CSmartPtr< CNNFilter<INTTYPE, DATATYPE> > m_spNNFilter1;
    CSmartPtr< CNNFilter<INTTYPE, DATATYPE> > m_spNNFilter2;

    // adaption
    INTTYPE m_aryM[9];
};

class CPredictorDecompressNormal3930to3950 : public IPredictorDecompress
{
public:
    CPredictorDecompressNormal3930to3950(int nCompressionLevel, int nVersion);
    virtual ~CPredictorDecompressNormal3930to3950() APE_OVERRIDE;

    int DecompressValue(int64 nInput, int64) APE_OVERRIDE;
    int Flush() APE_OVERRIDE;

protected:
    // buffer information
    CSmartPtr<int> m_spBuffer;

    // adaption
    int m_aryM[M_COUNT];

    // buffer pointers
    int * m_pInputBuffer;

    // other
    int m_nCurrentIndex;
    int m_nLastValue;
    CSmartPtr< CNNFilter<int, short> > m_spNNFilter;
    CSmartPtr< CNNFilter<int, short> > m_spNNFilter1;
};

template <class INTTYPE, class DATATYPE> class CPredictorDecompress3950toCurrent : public IPredictorDecompress
{
public:
    CPredictorDecompress3950toCurrent(int nCompressionLevel, int nVersion, int nBitsPerSample);
    virtual ~CPredictorDecompress3950toCurrent() APE_OVERRIDE;

    int DecompressValue(int64 nA, int64 nB = 0) APE_OVERRIDE;
    int Flush() APE_OVERRIDE;

    void SetInterimMode(bool bSet) APE_OVERRIDE;

protected:
    // buffer pointers
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbPredictionA;
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbPredictionB;

    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbAdaptA;
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbAdaptB;

    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterA;
    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterB;

    // pointers
    CSmartPtr< CNNFilter<INTTYPE, DATATYPE> > m_spNNFilter;
    CSmartPtr< CNNFilter<INTTYPE, DATATYPE> > m_spNNFilter1;
    CSmartPtr< CNNFilter<INTTYPE, DATATYPE> > m_spNNFilter2;

    // adaption
    INTTYPE m_aryMA[M_COUNT];
    INTTYPE m_aryMB[M_COUNT];

    // other
    INTTYPE m_nLastValueA;
    int m_nCurrentIndex;
    int m_nVersion;
    int m_nBitsPerSample;
    int m_bInterimMode;

    // alignment
    INTTYPE m_nPadding;
};

// forward declare the classes because it helps with a Clang warning
#ifdef _MSC_VER
extern template class CPredictorCompressNormal<int, short>;
extern template class CPredictorCompressNormal<int64, int>;
extern template class CPredictorDecompress3950toCurrent<int, short>;
extern template class CPredictorDecompress3950toCurrent<int64, int>;
#endif

#pragma pack(pop)

}
