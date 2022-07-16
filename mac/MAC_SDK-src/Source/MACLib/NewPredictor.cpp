#include "All.h"
#include "NewPredictor.h"
#include "MACLib.h"
#include "GlobalFunctions.h"

namespace APE
{

/**************************************************************************************************
CPredictorCompressNormal
**************************************************************************************************/
template <class INTTYPE> CPredictorCompressNormal<INTTYPE>::CPredictorCompressNormal(int nCompressionLevel, int nBitsPerSample)
{
    m_nBitsPerSample = nBitsPerSample;

    // initialize (to avoid warnings)
    memset(&m_aryM[0], 0, sizeof(m_aryM));
    m_nCurrentIndex = 0;

    if (nCompressionLevel == MAC_COMPRESSION_LEVEL_FAST)
    {
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_NORMAL)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(16, 11, MAC_FILE_VERSION_NUMBER));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_HIGH)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(64, 11, MAC_FILE_VERSION_NUMBER));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_EXTRA_HIGH)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(256, 13, MAC_FILE_VERSION_NUMBER));
        m_spNNFilter1.Assign(new CNNFilter<INTTYPE>(32, 10, MAC_FILE_VERSION_NUMBER));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_INSANE)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(1024 + 256, 15, MAC_FILE_VERSION_NUMBER));
        m_spNNFilter1.Assign(new CNNFilter<INTTYPE>(256, 13, MAC_FILE_VERSION_NUMBER));
        m_spNNFilter2.Assign(new CNNFilter<INTTYPE>(16, 11, MAC_FILE_VERSION_NUMBER));
    }
    else
    {
        throw(1);
    }
}

template <class INTTYPE> CPredictorCompressNormal<INTTYPE>::~CPredictorCompressNormal()
{
    m_spNNFilter.Delete();
    m_spNNFilter1.Delete();
    m_spNNFilter2.Delete();
}
    
template <class INTTYPE> int CPredictorCompressNormal<INTTYPE>::Flush()
{
    if (m_spNNFilter) m_spNNFilter->Flush();
    if (m_spNNFilter1) m_spNNFilter1->Flush();
    if (m_spNNFilter2) m_spNNFilter2->Flush();

    m_rbPrediction.Flush();
    m_rbAdapt.Flush();
    m_Stage1FilterA.Flush(); m_Stage1FilterB.Flush();

    memset(&m_aryM[0], 0, sizeof(m_aryM));

    INTTYPE * paryM = &m_aryM[8];
    paryM[0] = 360;
    paryM[-1] = 317;
    paryM[-2] = -109;
    paryM[-3] = 98;

    m_nCurrentIndex = 0;

    return ERROR_SUCCESS;
}

template <class INTTYPE> int64 CPredictorCompressNormal<INTTYPE>::CompressValue(int _nA, int _nB)
{
    // roll the buffers if necessary
    if (m_nCurrentIndex == WINDOW_BLOCKS)
    {
        m_rbPrediction.Roll(); m_rbAdapt.Roll();
        m_nCurrentIndex = 0;
    }

    // stage 1: simple, non-adaptive order 1 prediction
    INTTYPE nA = m_Stage1FilterA.Compress(_nA);
    INTTYPE nB = m_Stage1FilterB.Compress(_nB);

    // stage 2: adaptive offset filter(s)
    m_rbPrediction[0] = nA;
    m_rbPrediction[-2] = m_rbPrediction[-1] - m_rbPrediction[-2];
    
    m_rbPrediction[-5] = nB;
    m_rbPrediction[-6] = m_rbPrediction[-5] - m_rbPrediction[-6];

    INTTYPE * paryM = &m_aryM[8];
    INTTYPE nOutput;
    if ((m_nBitsPerSample <= 16) || (sizeof(INTTYPE) == 8))
    {
        INTTYPE nPredictionA = (m_rbPrediction[-1] * paryM[0]) + (m_rbPrediction[-2] * paryM[-1]) + (m_rbPrediction[-3] * paryM[-2]) + (m_rbPrediction[-4] * paryM[-3]);
        INTTYPE nPredictionB = (m_rbPrediction[-5] * paryM[-4]) + (m_rbPrediction[-6] * paryM[-5]) + (m_rbPrediction[-7] * paryM[-6]) + (m_rbPrediction[-8] * paryM[-7]) + (m_rbPrediction[-9] * paryM[-8]);

        nOutput = nA - INTTYPE((nPredictionA + (nPredictionB >> 1)) >> 10);
    }
    else
    {
        int64 nPredictionA = (int64(m_rbPrediction[-1]) * paryM[0]) + (int64(m_rbPrediction[-2]) * paryM[-1]) + (int64(m_rbPrediction[-3]) * paryM[-2]) + (int64(m_rbPrediction[-4]) * paryM[-3]);
        int64 nPredictionB = (int64(m_rbPrediction[-5]) * paryM[-4]) + (int64(m_rbPrediction[-6]) * paryM[-5]) + (int64(m_rbPrediction[-7]) * paryM[-6]) + (int64(m_rbPrediction[-8]) * paryM[-7]) + (int64(m_rbPrediction[-9]) * paryM[-8]);

    #ifdef LEGACY_ENCODE
        nOutput = nA - INTTYPE((int(nPredictionA) + (int(nPredictionB) >> 1)) >> 10);
    #else
        nOutput = nA - INTTYPE((nPredictionA + (nPredictionB >> 1)) >> 10);
    #endif
    }

    // adapt
    m_rbAdapt[0] = (m_rbPrediction[-1]) ? ((m_rbPrediction[-1] >> 30) & 2) - 1 : 0;
    m_rbAdapt[-1] = (m_rbPrediction[-2]) ? ((m_rbPrediction[-2] >> 30) & 2) - 1 : 0;
    m_rbAdapt[-4] = (m_rbPrediction[-5]) ? ((m_rbPrediction[-5] >> 30) & 2) - 1 : 0;
    m_rbAdapt[-5] = (m_rbPrediction[-6]) ? ((m_rbPrediction[-6] >> 30) & 2) - 1 : 0;

    if (nOutput > 0) 
    {
        INTTYPE * pM = &paryM[-8]; INTTYPE * pAdapt = &m_rbAdapt[-8];
        EXPAND_9_TIMES(*pM++ -= *pAdapt++;)
    }
    else if (nOutput < 0) 
    {
        INTTYPE * pM = &paryM[-8]; INTTYPE * pAdapt = &m_rbAdapt[-8];
        EXPAND_9_TIMES(*pM++ += *pAdapt++;)
    }

    // stage 3: NNFilters
    if (m_spNNFilter)
    {
        nOutput = m_spNNFilter->Compress(nOutput);

        if (m_spNNFilter1)
        {
            nOutput = m_spNNFilter1->Compress(nOutput);

            if (m_spNNFilter2)
                nOutput = m_spNNFilter2->Compress(nOutput);
        }
    }

    m_rbPrediction.IncrementFast(); m_rbAdapt.IncrementFast();
    m_nCurrentIndex++;

    return nOutput;
}

template class CPredictorCompressNormal<int>;
template class CPredictorCompressNormal<int64>;

/**************************************************************************************************
CPredictorDecompressNormal3930to3950
**************************************************************************************************/
CPredictorDecompressNormal3930to3950::CPredictorDecompressNormal3930to3950(int nCompressionLevel, int nVersion)
{
    // initialize (to avoid warnings)
    memset(&m_aryM[0], 0, sizeof(m_aryM));

    m_spBuffer.Assign(new int [HISTORY_ELEMENTS + WINDOW_BLOCKS], true);

    if (nCompressionLevel == MAC_COMPRESSION_LEVEL_FAST)
    {
        // no filters
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_NORMAL)
    {
        m_spNNFilter.Assign(new CNNFilter<int>(16, 11, nVersion));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_HIGH)
    {
        m_spNNFilter.Assign(new CNNFilter<int>(64, 11, nVersion));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_EXTRA_HIGH)
    {
        m_spNNFilter.Assign(new CNNFilter<int>(256, 13, nVersion));
        m_spNNFilter1.Assign(new CNNFilter<int>(32, 10, nVersion));
    }
    else
    {
        throw(1);
    }
}

CPredictorDecompressNormal3930to3950::~CPredictorDecompressNormal3930to3950()
{
    m_spNNFilter.Delete();
    m_spNNFilter1.Delete();
    m_spBuffer.Delete();
}
    
int CPredictorDecompressNormal3930to3950::Flush()
{
    if (m_spNNFilter) m_spNNFilter->Flush();
    if (m_spNNFilter1) m_spNNFilter1->Flush();

    ZeroMemory(m_spBuffer, (HISTORY_ELEMENTS + 1) * sizeof(int));
    ZeroMemory(&m_aryM[0], M_COUNT * sizeof(int));

    m_aryM[0] = 360;
    m_aryM[1] = 317;
    m_aryM[2] = -109;
    m_aryM[3] = 98;

    m_pInputBuffer = &m_spBuffer[HISTORY_ELEMENTS];
    
    m_nLastValue = 0;
    m_nCurrentIndex = 0;

    return ERROR_SUCCESS;
}

int CPredictorDecompressNormal3930to3950::DecompressValue(int64 _nInput, int64)
{
    if (m_nCurrentIndex == WINDOW_BLOCKS)
    {
        // copy forward and adjust pointers
        memcpy(&m_spBuffer[0], &m_spBuffer[WINDOW_BLOCKS], HISTORY_ELEMENTS * sizeof(int64));
        m_pInputBuffer = &m_spBuffer[HISTORY_ELEMENTS];

        m_nCurrentIndex = 0;
    }

    int nInput = int(_nInput);

    // stage 2: NNFilter
    if (m_spNNFilter1)
        nInput = m_spNNFilter1->Decompress(nInput);
    if (m_spNNFilter)
        nInput = m_spNNFilter->Decompress(nInput);

    // stage 1: multiple predictors (order 2 and offset 1)
    int p1 = m_pInputBuffer[-1];
    int p2 = m_pInputBuffer[-1] - m_pInputBuffer[-2];
    int p3 = m_pInputBuffer[-2] - m_pInputBuffer[-3];
    int p4 = m_pInputBuffer[-3] - m_pInputBuffer[-4];
    
    m_pInputBuffer[0] = nInput + (((p1 * m_aryM[0]) + (p2 * m_aryM[1]) + (p3 * m_aryM[2]) + (p4 * m_aryM[3])) >> 9);
    
    if (nInput > 0) 
    {
        m_aryM[0] -= ((p1 >> 30) & 2) - 1;
        m_aryM[1] -= ((p2 >> 30) & 2) - 1;
        m_aryM[2] -= ((p3 >> 30) & 2) - 1;
        m_aryM[3] -= ((p4 >> 30) & 2) - 1;
    }
    else if (nInput < 0) 
    {
        m_aryM[0] += ((p1 >> 30) & 2) - 1;
        m_aryM[1] += ((p2 >> 30) & 2) - 1;
        m_aryM[2] += ((p3 >> 30) & 2) - 1;
        m_aryM[3] += ((p4 >> 30) & 2) - 1;
    }

    int nResult = m_pInputBuffer[0] + ((m_nLastValue * 31) >> 5);
    m_nLastValue = nResult;

    m_nCurrentIndex++;
    m_pInputBuffer++;

    return nResult;
}

/**************************************************************************************************
CPredictorDecompress3950toCurrent
**************************************************************************************************/
template <class INTTYPE> CPredictorDecompress3950toCurrent<INTTYPE>::CPredictorDecompress3950toCurrent(int nCompressionLevel, int nVersion, int nBitsPerSample)
{
    m_nVersion = nVersion;
    m_nBitsPerSample = nBitsPerSample;
    m_bLegacyDecode = false;

    // initialize (to avoid warnings)
    memset(&m_aryMA[0], 0, sizeof(m_aryMA));
    memset(&m_aryMB[0], 0, sizeof(m_aryMB));

    if (nCompressionLevel == MAC_COMPRESSION_LEVEL_FAST)
    {
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_NORMAL)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(16, 11, nVersion));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_HIGH)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(64, 11, nVersion));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_EXTRA_HIGH)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(256, 13, nVersion));
        m_spNNFilter1.Assign(new CNNFilter<INTTYPE>(32, 10, nVersion));
    }
    else if (nCompressionLevel == MAC_COMPRESSION_LEVEL_INSANE)
    {
        m_spNNFilter.Assign(new CNNFilter<INTTYPE>(1024 + 256, 15, nVersion));
        m_spNNFilter1.Assign(new CNNFilter<INTTYPE>(256, 13, nVersion));
        m_spNNFilter2.Assign(new CNNFilter<INTTYPE>(16, 11, nVersion));
    }
    else
    {
        throw(1);
    }
}

template <class INTTYPE> CPredictorDecompress3950toCurrent<INTTYPE>::~CPredictorDecompress3950toCurrent()
{
    m_spNNFilter.Delete();
    m_spNNFilter1.Delete();
    m_spNNFilter2.Delete();
}

template <class INTTYPE> void CPredictorDecompress3950toCurrent<INTTYPE>::SetLegacyDecode(bool bSet)
{
    m_bLegacyDecode = bSet;

    if (m_spNNFilter) m_spNNFilter->SetLegacyDecode(bSet);
    if (m_spNNFilter1) m_spNNFilter1->SetLegacyDecode(bSet);
    if (m_spNNFilter2) m_spNNFilter2->SetLegacyDecode(bSet);
}

template <class INTTYPE> int CPredictorDecompress3950toCurrent<INTTYPE>::Flush()
{
    if (m_spNNFilter) m_spNNFilter->Flush();
    if (m_spNNFilter1) m_spNNFilter1->Flush();
    if (m_spNNFilter2) m_spNNFilter2->Flush();

    ZeroMemory(m_aryMA, sizeof(m_aryMA));
    ZeroMemory(m_aryMB, sizeof(m_aryMB));

    m_rbPredictionA.Flush();
    m_rbPredictionB.Flush();
    m_rbAdaptA.Flush();
    m_rbAdaptB.Flush();

    m_aryMA[0] = 360;
    m_aryMA[1] = 317;
    m_aryMA[2] = -109;
    m_aryMA[3] = 98;

    m_Stage1FilterA.Flush();
    m_Stage1FilterB.Flush();
    
    m_nLastValueA = 0;
    
    m_nCurrentIndex = 0;

    return ERROR_SUCCESS;
}

template <class INTTYPE> int CPredictorDecompress3950toCurrent<INTTYPE>::DecompressValue(int64 _nA, int64 _nB)
{
    if (m_nCurrentIndex == WINDOW_BLOCKS)
    {
        // copy forward and adjust pointers
        m_rbPredictionA.Roll(); m_rbPredictionB.Roll();
        m_rbAdaptA.Roll(); m_rbAdaptB.Roll();

        m_nCurrentIndex = 0;
    }

    INTTYPE nA = INTTYPE(_nA);
    INTTYPE nB = INTTYPE(_nB);

    // stage 2: NNFilter
    if (m_spNNFilter2)
        nA = m_spNNFilter2->Decompress(nA);
    if (m_spNNFilter1)
        nA = m_spNNFilter1->Decompress(nA);
    if (m_spNNFilter)
        nA = m_spNNFilter->Decompress(nA);

    // stage 1: multiple predictors (order 2 and offset 1)
    m_rbPredictionA[0] = m_nLastValueA;
    m_rbPredictionA[-1] = m_rbPredictionA[0] - m_rbPredictionA[-1];
    
    m_rbPredictionB[0] = m_Stage1FilterB.Compress(int32(nB));
    m_rbPredictionB[-1] = m_rbPredictionB[0] - m_rbPredictionB[-1];

    INTTYPE nCurrentA;
    if ((m_nBitsPerSample <= 16) || (sizeof(INTTYPE) == 8))
    {
        INTTYPE nPredictionA = (m_rbPredictionA[0] * m_aryMA[0]) + (m_rbPredictionA[-1] * m_aryMA[1]) + (m_rbPredictionA[-2] * m_aryMA[2]) + (m_rbPredictionA[-3] * m_aryMA[3]);
        INTTYPE nPredictionB = (m_rbPredictionB[0] * m_aryMB[0]) + (m_rbPredictionB[-1] * m_aryMB[1]) + (m_rbPredictionB[-2] * m_aryMB[2]) + (m_rbPredictionB[-3] * m_aryMB[3]) + (m_rbPredictionB[-4] * m_aryMB[4]);

        nCurrentA = nA + ((nPredictionA + (nPredictionB >> 1)) >> 10);
    }
    else
    {
        int64 nPredictionA = (int64(m_rbPredictionA[0]) * m_aryMA[0]) + (int64(m_rbPredictionA[-1]) * m_aryMA[1]) + (int64(m_rbPredictionA[-2]) * m_aryMA[2]) + (int64(m_rbPredictionA[-3]) * m_aryMA[3]);
        int64 nPredictionB = (int64(m_rbPredictionB[0]) * m_aryMB[0]) + (int64(m_rbPredictionB[-1]) * m_aryMB[1]) + (int64(m_rbPredictionB[-2]) * m_aryMB[2]) + (int64(m_rbPredictionB[-3]) * m_aryMB[3]) + (int64(m_rbPredictionB[-4]) * m_aryMB[4]);

        if (m_bLegacyDecode)
            nCurrentA = nA + INTTYPE((int(nPredictionA) + (int(nPredictionB) >> 1)) >> 10);
        else
            nCurrentA = nA + INTTYPE((nPredictionA + (nPredictionB >> 1)) >> 10);
    }

    m_rbAdaptA[0] = (m_rbPredictionA[0]) ? ((m_rbPredictionA[0] >> 30) & 2) - 1 : 0;
    m_rbAdaptA[-1] = (m_rbPredictionA[-1]) ? ((m_rbPredictionA[-1] >> 30) & 2) - 1 : 0;
    
    m_rbAdaptB[0] = (m_rbPredictionB[0]) ? ((m_rbPredictionB[0] >> 30) & 2) - 1 : 0;
    m_rbAdaptB[-1] = (m_rbPredictionB[-1]) ? ((m_rbPredictionB[-1] >> 30) & 2) - 1 : 0;

    if (nA > 0) 
    {
        m_aryMA[0] -= m_rbAdaptA[0];
        m_aryMA[1] -= m_rbAdaptA[-1];
        m_aryMA[2] -= m_rbAdaptA[-2];
        m_aryMA[3] -= m_rbAdaptA[-3];

        m_aryMB[0] -= m_rbAdaptB[0];
        m_aryMB[1] -= m_rbAdaptB[-1];
        m_aryMB[2] -= m_rbAdaptB[-2];
        m_aryMB[3] -= m_rbAdaptB[-3];
        m_aryMB[4] -= m_rbAdaptB[-4];
    }
    else if (nA < 0) 
    {
        m_aryMA[0] += m_rbAdaptA[0];
        m_aryMA[1] += m_rbAdaptA[-1];
        m_aryMA[2] += m_rbAdaptA[-2];
        m_aryMA[3] += m_rbAdaptA[-3];

        m_aryMB[0] += m_rbAdaptB[0];
        m_aryMB[1] += m_rbAdaptB[-1];
        m_aryMB[2] += m_rbAdaptB[-2];
        m_aryMB[3] += m_rbAdaptB[-3];
        m_aryMB[4] += m_rbAdaptB[-4];
    }

    int nResult = m_Stage1FilterA.Decompress(nCurrentA);
    m_nLastValueA = nCurrentA;
    
    m_rbPredictionA.IncrementFast(); m_rbPredictionB.IncrementFast();
    m_rbAdaptA.IncrementFast(); m_rbAdaptB.IncrementFast();

    m_nCurrentIndex++;

    return nResult;
}

template class CPredictorDecompress3950toCurrent<int>;
template class CPredictorDecompress3950toCurrent<int64>;

}