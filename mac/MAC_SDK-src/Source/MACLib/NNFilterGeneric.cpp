#include "NNFilter.h"
#include "NNFilterCommon.h"

namespace APE
{

static void AdaptGeneric(short * pM, const short * pAdapt, int32 nDirection, int nOrder)
{
    return Adapt(pM, pAdapt, nDirection, nOrder);
}

static void AdaptGeneric(int * pM, const int * pAdapt, int64 nDirection, int nOrder)
{
    return Adapt(pM, pAdapt, nDirection, nOrder);
}

static int32 CalculateDotProductGeneric(const short * pA, const short * pB, int nOrder)
{
    return CalculateDotProduct(pA, pB, nOrder);
}

static int64 CalculateDotProductGeneric(const int * pA, const int * pB, int nOrder)
{
    return CalculateDotProduct(pA, pB, nOrder);
}

template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::CompressGeneric(INTTYPE nInput)
{
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductGeneric(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput = static_cast<INTTYPE>(nInput - ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptGeneric(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nOutput, m_nOrder);

    // update delta
    UPDATE_DELTA_NEW(nInput)

    // convert the input to a short and store it
    m_rbInput[0] = GetSaturatedShortFromInt(nInput);

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    return nOutput;
}

template int CNNFilter<int, short>::CompressGeneric(int nInput);
template int64 CNNFilter<int64, int>::CompressGeneric(int64 nInput);

template <class INTTYPE, class DATATYPE> INTTYPE CNNFilter<INTTYPE, DATATYPE>::DecompressGeneric(INTTYPE nInput)
{
    // figure a dot product
    INTTYPE nDotProduct = CalculateDotProductGeneric(&m_rbInput[-m_nOrder], &m_paryM[0], m_nOrder);

    // calculate the output
    INTTYPE nOutput;
    if (m_bInterimMode)
        nOutput = static_cast<INTTYPE>(nInput + ((static_cast<int64>(nDotProduct) + m_nOneShiftedByShift) >> m_nShift));
    else
        nOutput = static_cast<INTTYPE>(nInput + ((nDotProduct + m_nOneShiftedByShift) >> m_nShift));

    // adapt
    AdaptGeneric(&m_paryM[0], &m_rbDeltaM[-m_nOrder], nInput, m_nOrder);

    // update delta
    if ((m_nVersion == -1) || (m_nVersion >= 3980))
        UPDATE_DELTA_NEW(nOutput)
    else
        UPDATE_DELTA_OLD(nOutput)

    // update the input buffer
    m_rbInput[0] = GetSaturatedShortFromInt(nOutput);

    // increment and roll if necessary
    m_rbInput.IncrementSafe();
    m_rbDeltaM.IncrementSafe();

    return nOutput;
}

template int CNNFilter<int, short>::DecompressGeneric(int nInput);
template int64 CNNFilter<int64, int>::DecompressGeneric(int64 nInput);

}
