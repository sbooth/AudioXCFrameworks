#pragma once

namespace APE
{

class CAntiPredictor;

CAntiPredictor * CreateAntiPredictor(intn nCompressionLevel, intn nVersion);

/**************************************************************************************************
Base class for all anti-predictors
**************************************************************************************************/
class CAntiPredictor
{
public:
    CAntiPredictor();
    virtual ~CAntiPredictor();

    virtual void AntiPredict(int *, int *, int) { }
};

/**************************************************************************************************
Offset anti-predictor
**************************************************************************************************/
class CAntiPredictorOffset : public CAntiPredictor
{
public:
    void AntiPredictOffset(int * pInputArray, int * pOutputArray, int NumberOfElements, int Offset, int DeltaM);
};

#ifdef ENABLE_COMPRESSION_MODE_FAST

/**************************************************************************************************
Fast anti-predictor (from original 'fast' mode...updated for version 3.32)
**************************************************************************************************/
class CAntiPredictorFast0000To3320 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
Fast anti-predictor (new 'fast' mode release with version 3.32)
**************************************************************************************************/
class CAntiPredictorFast3320ToCurrent : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

#endif // #ifdef ENABLE_COMPRESSION_MODE_FAST

#ifdef ENABLE_COMPRESSION_MODE_NORMAL
/**************************************************************************************************
Normal anti-predictor
**************************************************************************************************/
class CAntiPredictorNormal0000To3320 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
Normal anti-predictor
**************************************************************************************************/
class CAntiPredictorNormal3320To3800 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
Normal anti-predictor
**************************************************************************************************/
class CAntiPredictorNormal3800ToCurrent : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

#endif // #ifdef ENABLE_COMPRESSION_MODE_NORMAL

#ifdef ENABLE_COMPRESSION_MODE_HIGH

/**************************************************************************************************
High anti-predictor
**************************************************************************************************/
class CAntiPredictorHigh0000To3320 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
High anti-predictor
**************************************************************************************************/
class CAntiPredictorHigh3320To3600 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
High anti-predictor
**************************************************************************************************/
class CAntiPredictorHigh3600To3700 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
High anti-predictor
**************************************************************************************************/
class CAntiPredictorHigh3700To3800 : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

/**************************************************************************************************
High anti-predictor
**************************************************************************************************/
class CAntiPredictorHigh3800ToCurrent : public CAntiPredictor
{
public:
    void AntiPredict(int * pInputArray, int * pOutputArray, int NumberOfElements) APE_OVERRIDE;
};

#endif // #ifdef ENABLE_COMPRESSION_MODE_HIGH

#ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

/**************************************************************************************************
Extra high helper
**************************************************************************************************/
class CAntiPredictorExtraHighHelper
{
public:
    int ConventionalDotProduct(short * bip, short * bbm, short * pIPAdaptFactor, int op, int nNumberOfIterations);
};

/**************************************************************************************************
Extra high anti-predictor
**************************************************************************************************/
class CAntiPredictorExtraHigh0000To3320 : public CAntiPredictor
{
public:
    void AntiPredictCustom(int * pInputArray, int * pOutputArray, int NumberOfElements, int Iterations, const int64 * pOffsetValueArrayA, const int64 * pOffsetValueArrayB);

private:
    void AntiPredictorOffset(const int * Input_Array, int * Output_Array, int Number_of_Elements, int64 g, int dm, int Max_Order);
};

/**************************************************************************************************
Extra high anti-predictor
**************************************************************************************************/
class CAntiPredictorExtraHigh3320To3600 : public CAntiPredictor
{
public:
    void AntiPredictCustom(int * pInputArray, int * pOutputArray, int NumberOfElements, int Iterations, const int64 * pOffsetValueArrayA, const int64 * pOffsetValueArrayB);

private:
    void AntiPredictorOffset(const int * Input_Array, int * Output_Array, int Number_of_Elements, int64 g, int dm, int Max_Order);
};

/**************************************************************************************************
Extra high anti-predictor
**************************************************************************************************/
class CAntiPredictorExtraHigh3600To3700 : public CAntiPredictor
{
public:
    void AntiPredictCustom(int * pInputArray, int * pOutputArray, int NumberOfElements, int Iterations, const int64 * pOffsetValueArrayA, const int64 * pOffsetValueArrayB);

private:
    void AntiPredictorOffset(const int * Input_Array, int * Output_Array, int Number_of_Elements, int64 g1, int64 g2, int Max_Order);
};

/**************************************************************************************************
Extra high anti-predictor
**************************************************************************************************/
class CAntiPredictorExtraHigh3700To3800 : public CAntiPredictor
{
public:
    void AntiPredictCustom(int * pInputArray, int * pOutputArray, int NumberOfElements, int Iterations, const int64 * pOffsetValueArrayA, const int64 * pOffsetValueArrayB);

private:
    void AntiPredictorOffset(const int * Input_Array, int * Output_Array, int Number_of_Elements, int64 g1, int64 g2, int Max_Order);
};

/**************************************************************************************************
Extra high anti-predictor
**************************************************************************************************/
class CAntiPredictorExtraHigh3800ToCurrent : public CAntiPredictor
{
public:
    void AntiPredictCustom(int * pInputArray, int * pOutputArray, int NumberOfElements, intn nVersion);
};

#endif // #ifdef ENABLE_COMPRESSION_MODE_EXTRA_HIGH

}
