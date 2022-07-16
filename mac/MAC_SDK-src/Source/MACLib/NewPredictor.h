#pragma once

#include "Predictor.h"
#include "RollBuffer.h"
#include "NNFilter.h"
#include "ScaledFirstOrderFilter.h"

namespace APE
{

/**************************************************************************************************
Functions to create the interfaces
**************************************************************************************************/
#define WINDOW_BLOCKS           4096

#define HISTORY_ELEMENTS        8
#define M_COUNT                 8

template <class INTTYPE> class CPredictorCompressNormal : public IPredictorCompress
{
public:
    CPredictorCompressNormal(int nCompressionLevel, int nBitsPerSample);
    virtual ~CPredictorCompressNormal();

    int64 CompressValue(int nA, int nB = 0);
    int Flush();

protected:
    // buffer information
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 10> m_rbPrediction;
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 9> m_rbAdapt;

    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterA;
    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterB;

    // adaption
    INTTYPE m_aryM[9];
    
    // other
    int m_nCurrentIndex;
    int m_nBitsPerSample;
    CSmartPtr<CNNFilter<INTTYPE>> m_spNNFilter;
    CSmartPtr<CNNFilter<INTTYPE>> m_spNNFilter1;
    CSmartPtr<CNNFilter<INTTYPE>> m_spNNFilter2;
};

class CPredictorDecompressNormal3930to3950 : public IPredictorDecompress
{
public:
    CPredictorDecompressNormal3930to3950(int nCompressionLevel, int nVersion);
    virtual ~CPredictorDecompressNormal3930to3950();

    int DecompressValue(int64 nInput, int64);
    int Flush();
    
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
    CSmartPtr<CNNFilter<int>> m_spNNFilter;
    CSmartPtr<CNNFilter<int>> m_spNNFilter1;
};

template <class INTTYPE> class CPredictorDecompress3950toCurrent : public IPredictorDecompress
{
public:
    CPredictorDecompress3950toCurrent(int nCompressionLevel, int nVersion, int nBitsPerSample);
    virtual ~CPredictorDecompress3950toCurrent();

    int DecompressValue(int64 nA, int64 nB = 0);
    int Flush();

    void SetLegacyDecode(bool bSet);

protected:
    // adaption
    INTTYPE m_aryMA[M_COUNT];
    INTTYPE m_aryMB[M_COUNT];
    
    // buffer pointers
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbPredictionA;
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbPredictionB;

    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbAdaptA;
    CRollBufferFast<INTTYPE, WINDOW_BLOCKS, 8> m_rbAdaptB;

    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterA;
    CScaledFirstOrderFilter<INTTYPE, 31, 5> m_Stage1FilterB;

    // other
    int m_nCurrentIndex;
    INTTYPE m_nLastValueA;
    int m_nVersion;
    int m_nBitsPerSample;
    CSmartPtr<CNNFilter<INTTYPE>> m_spNNFilter;
    CSmartPtr<CNNFilter<INTTYPE>> m_spNNFilter1;
    CSmartPtr<CNNFilter<INTTYPE>> m_spNNFilter2;
    bool m_bLegacyDecode;
};

}
