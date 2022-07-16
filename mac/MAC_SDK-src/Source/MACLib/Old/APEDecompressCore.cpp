#include "All.h"
#ifdef APE_BACKWARDS_COMPATIBILITY

#include "UnMAC.h"
#include "APEDecompressCore.h"
#include "../APEInfo.h"
#include "GlobalFunctions.h"
#include "Anti-Predictor.h"
#include "../Prepare.h"

namespace APE
{

CAPEDecompressCore::CAPEDecompressCore(IAPEDecompress * pAPEDecompress)
{
    m_pAPEDecompress = pAPEDecompress;

    // initialize the bit array
    m_spUnBitArray.Assign(CreateUnBitArray(pAPEDecompress, (intn) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)));
    
    if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) >= 3930)
        throw(0);

    m_spAntiPredictorX.Assign(CreateAntiPredictor((intn) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL), (intn) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)));
    m_spAntiPredictorY.Assign(CreateAntiPredictor((intn) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL), (intn) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)));
    
    m_spDataX.Assign(new int [int(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCKS_PER_FRAME)) + 16], true);
    m_spDataY.Assign(new int [int(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCKS_PER_FRAME)) + 16], true);
    m_spTempData.Assign(new int [int(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCKS_PER_FRAME)) + 16], true);

    m_nBlocksProcessed = 0;
    m_BitArrayStateX = { 0 };
    m_BitArrayStateY = { 0 };
}

CAPEDecompressCore::~CAPEDecompressCore()
{
}

void CAPEDecompressCore::GenerateDecodedArrays(intn nBlocks, intn nSpecialCodes, intn nFrameIndex)
{
    if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) == 2)
    {
        if ((nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE) && (nSpecialCodes & SPECIAL_FRAME_RIGHT_SILENCE)) 
        {
            memset(m_spDataX, 0, nBlocks * 4);
            memset(m_spDataY, 0, nBlocks * 4);
        }
        else if (nSpecialCodes & SPECIAL_FRAME_PSEUDO_STEREO) 
        {
            GenerateDecodedArray(m_spDataX, (uint32) nBlocks, nFrameIndex, m_spAntiPredictorX);
            memset(m_spDataY, 0, nBlocks * 4);
        }
        else 
        {
            GenerateDecodedArray(m_spDataX, (uint32) nBlocks, nFrameIndex, m_spAntiPredictorX);
            GenerateDecodedArray(m_spDataY, (uint32) nBlocks, nFrameIndex, m_spAntiPredictorY);
        }
    }
    else
    {
        if (nSpecialCodes & SPECIAL_FRAME_LEFT_SILENCE)
        {
            memset(m_spDataX, 0, nBlocks * 4);
        }
        else
        {
            GenerateDecodedArray(m_spDataX, (uint32) nBlocks, nFrameIndex, m_spAntiPredictorX);
        }
    }
}


void CAPEDecompressCore::GenerateDecodedArray(int * Input_Array, uint32 Number_of_Elements, intn Frame_Index, CAntiPredictor * pAntiPredictor)
{
    const intn nFrameBytes = (intn) m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FRAME_BYTES, Frame_Index);
    if (nFrameBytes <= 0)
        throw(ERROR_INVALID_INPUT_FILE);

    // run the prediction sequence
    switch (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL))
    {

#ifdef ENABLE_COMPRESSION_MODE_FAST
        case MAC_COMPRESSION_LEVEL_FAST:
            if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) < 3320)
            {
                m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
                pAntiPredictor->AntiPredict(m_spTempData, Input_Array, Number_of_Elements);
            }
            else
            {
                m_spUnBitArray->GenerateArray(Input_Array, Number_of_Elements, nFrameBytes);
                pAntiPredictor->AntiPredict(Input_Array, NULL, Number_of_Elements);
            }

            break;
#endif // #ifdef ENABLE_COMPRESSION_MODE_FAST
            
#ifdef ENABLE_COMPRESSION_MODE_NORMAL
        
        case MAC_COMPRESSION_LEVEL_NORMAL:
        {
            // get the array from the bitstream
            m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
            pAntiPredictor->AntiPredict(m_spTempData, Input_Array, Number_of_Elements);
            break;
        }

#endif // #ifdef ENABLE_COMPRESSION_MODE_NORMAL

#ifdef ENABLE_COMPRESSION_MODE_HIGH
        case MAC_COMPRESSION_LEVEL_HIGH:
            // get the array from the bitstream
            m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
            pAntiPredictor->AntiPredict(m_spTempData, Input_Array, Number_of_Elements);
            break;
#endif // #ifdef ENABLE_COMPRESSION_MODE_HIGH

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH
        case MAC_COMPRESSION_LEVEL_EXTRA_HIGH:
            
            uint64 aryCoefficientsA[64], aryCoefficientsB[64];
            uint32 nNumberOfCoefficients;
            
            #define GET_COEFFICIENTS(NumberOfCoefficientsBits, ValueBits)                                            \
                nNumberOfCoefficients = (uint32) m_spUnBitArray->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_X_BITS, NumberOfCoefficientsBits);        \
                for (unsigned int z = 0; z <= nNumberOfCoefficients; z++)                                            \
                {                                                                                                    \
                    aryCoefficientsA[z] = m_spUnBitArray->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_X_BITS, ValueBits);        \
                    aryCoefficientsB[z] = m_spUnBitArray->DecodeValue(CUnBitArrayBase::DECODE_VALUE_METHOD_X_BITS, ValueBits);        \
                }                                                                                                    \

            if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) < 3320)
            {
                GET_COEFFICIENTS(4, 6)
                m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh0000To3320 *) pAntiPredictor)->AntiPredict(m_spTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }
            else if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) < 3600)
            {
                GET_COEFFICIENTS(3, 5)
                m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3320To3600 *) pAntiPredictor)->AntiPredict(m_spTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }
            else if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) < 3700)
            {
                GET_COEFFICIENTS(3, 6)
                m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3600To3700 *) pAntiPredictor)->AntiPredict(m_spTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }
            else if (m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) < 3800)
            {
                GET_COEFFICIENTS(3, 6)
                m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3700To3800 *) pAntiPredictor)->AntiPredict(m_spTempData, Input_Array, Number_of_Elements, nNumberOfCoefficients, &aryCoefficientsA[0], &aryCoefficientsB[0]);
            }    
            else
            {
                m_spUnBitArray->GenerateArray(m_spTempData, Number_of_Elements, nFrameBytes);
                ((CAntiPredictorExtraHigh3800ToCurrent *) pAntiPredictor)->AntiPredict(m_spTempData, Input_Array, Number_of_Elements, (intn) m_pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION));
            }
            
            break;
#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH
    }
}

}

#endif // #ifdef APE_BACKWARDS_COMPATIBILITY
