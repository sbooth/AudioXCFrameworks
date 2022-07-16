#include "All.h"
#ifdef APE_BACKWARDS_COMPATIBILITY

#include "../MACLib.h"
#include "Anti-Predictor.h"

namespace APE
{

CAntiPredictor * CreateAntiPredictor(intn nCompressionLevel, intn nVersion)
{
    CAntiPredictor * pAntiPredictor = NULL;

    switch (nCompressionLevel) 
    {
#ifdef ENABLE_COMPRESSION_MODE_FAST
        case MAC_COMPRESSION_LEVEL_FAST:
            if (nVersion < 3320)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorFast0000To3320;
            }
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorFast3320ToCurrent;
            }
            break;
#endif //ENABLE_COMPRESSION_MODE_FAST
            
#ifdef ENABLE_COMPRESSION_MODE_NORMAL
        
        case MAC_COMPRESSION_LEVEL_NORMAL:
            if (nVersion < 3320)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorNormal0000To3320;
            }                
            else if (nVersion < 3800)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorNormal3320To3800;
            }
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorNormal3800ToCurrent;
            }
            break;

#endif //ENABLE_COMPRESSION_MODE_NORMAL

#ifdef ENABLE_COMPRESSION_MODE_HIGH
        case MAC_COMPRESSION_LEVEL_HIGH:
            if (nVersion < 3320)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh0000To3320;
            }
            else if (nVersion < 3600)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3320To3600;
            }
            else if (nVersion < 3700)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3600To3700;
            }
            else if (nVersion < 3800)
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3700To3800;
            }
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorHigh3800ToCurrent;
            }
            break;
#endif //ENABLE_COMPRESSION_MODE_HIGH

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH
        case MAC_COMPRESSION_LEVEL_EXTRA_HIGH:
            if (nVersion < 3320) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh0000To3320;
            }
            else if (nVersion < 3600) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3320To3600;
            }
            else if (nVersion < 3700) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3600To3700;
            }
            else if (nVersion < 3800) 
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3700To3800;
            }    
            else
            {
                pAntiPredictor = (CAntiPredictor *) new CAntiPredictorExtraHigh3800ToCurrent;
            }
            break;
#endif //ENABLE_COMPRESSION_MODE_EXTRA_HIGH
    }

    return pAntiPredictor;
}

CAntiPredictor::CAntiPredictor() 
{
}

CAntiPredictor::~CAntiPredictor() 
{
}

void CAntiPredictorOffset::AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements, int Offset, int DeltaM) 
{
    memcpy(pOutputArray, pInputArray, Offset * 4);

    int * ip = &pInputArray[Offset];
    int * ipo = &pOutputArray[0];
    int * op = &pOutputArray[Offset];
    int m = 0;
    
    for (; op < &pOutputArray[NumberOfElements]; ip++, ipo++, op++) 
    {
        *op = *ip + ((*ipo * m) >> 12);

        (*ipo ^ *ip) > 0 ? m += DeltaM : m -= DeltaM;
    }
}

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

int CAntiPredictorExtraHighHelper::ConventionalDotProduct(short *bip, short *bbm, short *pIPAdaptFactor, int op, int nNumberOfIterations) 
{
    // dot product
    int nDotProduct = 0;
    short *pMaxBBM = &bbm[nNumberOfIterations];
    
    if (op == 0)
    {
        while(bbm < pMaxBBM)
        {
            EXPAND_32_TIMES(nDotProduct += *bip++ * *bbm++;)
        }
    }
    else if (op > 0) 
    {
        while(bbm < pMaxBBM)
        {
            EXPAND_32_TIMES(nDotProduct += *bip++ * *bbm; *bbm++ += *pIPAdaptFactor++;)
        }
    }
    else
    {
        while(bbm < pMaxBBM)
        {
            EXPAND_32_TIMES(nDotProduct += *bip++ * *bbm; *bbm++ -= *pIPAdaptFactor++;)
        }
    }
    
    // use the dot product
    return nDotProduct;
}

#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

}

#endif // #ifdef APE_BACKWARDS_COMPATIBILITY
