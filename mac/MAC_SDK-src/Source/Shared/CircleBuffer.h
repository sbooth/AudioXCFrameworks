#pragma once

namespace APE
{

#pragma pack(push, 1)

class CCircleBuffer
{
public:
    // construction / destruction
    CCircleBuffer();
    virtual ~CCircleBuffer();

    // create the buffer
    void CreateBuffer(uint32 nBytes, uint32 nMaxDirectWriteBytes);

    // query
    uint32 MaxAdd();
    uint32 MaxGet();

    // direct writing
#ifdef APE_ENABLE_CIRCLE_BUFFER_WRITE
    __forceinline unsigned char * GetDirectWritePointer()
    {
        // return a pointer to the tail -- note that it will always be safe to write
        // at least m_nMaxDirectWriteBytes since we use an end cap region
        return &m_spBuffer[m_nTail];
    }

    __forceinline void UpdateAfterDirectWrite(uint32 nBytes)
    {
        // update the tail
        m_nTail += nBytes;

        // if the tail enters the "end cap" area, set the end cap and loop around
        if (m_nTail >= (m_nTotal - m_nMaxDirectWriteBytes))
        {
            m_nEndCap = m_nTail;
            m_nTail = 0;
        }
    }
#endif

    // update CRC for last nBytes bytes
    uint32 UpdateCRC(uint32 nCRC, uint32 nBytes);

    // get data
    uint32 Get(unsigned char * pBuffer, uint32 nBytes);

    // remove / empty
    void Empty();
    uint32 RemoveHead(uint32 nBytes);
    uint32 RemoveTail(uint32 nBytes);

private:
    uint32 m_nTotal;
    uint32 m_nMaxDirectWriteBytes;
    uint32 m_nEndCap;
    uint32 m_nHead;
    uint32 m_nTail;
    CSmartPtr<unsigned char> m_spBuffer;
};

#pragma pack(pop)

}
