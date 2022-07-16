#pragma once

namespace APE
{

template <class TYPE> class CRollBuffer
{
public:
    CRollBuffer()
    {
        m_pData = NULL;
        m_pCurrent = NULL;
        m_nHistoryElements = 0;
        m_nTotalElements = 0;
    }

    ~CRollBuffer()
    {
        SAFE_ARRAY_DELETE(m_pData)
    }

    int Create(int nWindowElements, int nHistoryElements)
    {
        SAFE_ARRAY_DELETE(m_pData)
        m_nHistoryElements = nHistoryElements;
        m_nTotalElements = nWindowElements + m_nHistoryElements;

        m_pData = new TYPE[m_nTotalElements];
        if (m_pData == NULL)
            return ERROR_INSUFFICIENT_MEMORY;

        Flush();
        return ERROR_SUCCESS;
    }

    void Flush()
    {
        ZeroMemory(m_pData, (m_nHistoryElements + 1) * int(sizeof(TYPE)));
        m_pCurrent = &m_pData[m_nHistoryElements];
    }

    void Roll()
    {
        memmove(&m_pData[0], &m_pCurrent[-m_nHistoryElements], m_nHistoryElements * int(sizeof(TYPE)));
        m_pCurrent = &m_pData[m_nHistoryElements];
    }

    __forceinline void IncrementSafe()
    {
        m_pCurrent++;
        if (m_pCurrent == &m_pData[m_nTotalElements])
            Roll();
    }

    __forceinline void IncrementFast()
    {
        m_pCurrent++;
    }

    __forceinline TYPE & operator[](const int nIndex) const
    {
        return m_pCurrent[nIndex];
    }

protected:
    TYPE * m_pData;
    TYPE * m_pCurrent;
    int m_nHistoryElements;
    int m_nTotalElements;
};

template <class TYPE, int WINDOW_ELEMENTS, int HISTORY_ELEMENTS> class CRollBufferFast
{
public:
    CRollBufferFast()
    {
        m_pData = new TYPE[WINDOW_ELEMENTS + HISTORY_ELEMENTS];
        Flush();
    }

    ~CRollBufferFast()
    {
        SAFE_ARRAY_DELETE(m_pData)
    }

    void Flush()
    {
        ZeroMemory(m_pData, (HISTORY_ELEMENTS + 1) * sizeof(TYPE));
        m_pCurrent = &m_pData[HISTORY_ELEMENTS];
    }

    void Roll()
    {
        memmove(&m_pData[0], &m_pCurrent[-HISTORY_ELEMENTS], HISTORY_ELEMENTS * sizeof(TYPE));
        m_pCurrent = &m_pData[HISTORY_ELEMENTS];
    }

    __forceinline void IncrementSafe()
    {
        m_pCurrent++;
        if (m_pCurrent == &m_pData[WINDOW_ELEMENTS + HISTORY_ELEMENTS])
            Roll();
    }

    __forceinline void IncrementFast()
    {
        m_pCurrent++;
    }

    __forceinline TYPE & operator[](const int nIndex) const
    {
        return m_pCurrent[nIndex];
    }

protected:
    TYPE * m_pData;
    TYPE * m_pCurrent;
};

}
