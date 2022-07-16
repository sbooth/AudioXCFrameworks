#include "All.h"
#include "CircleBuffer.h"
#include "CRC.h"

namespace APE
{

CCircleBuffer::CCircleBuffer()
{
    m_pBuffer = NULL;
    m_nTotal = 0;
    m_nHead = 0;
    m_nTail = 0;
    m_nEndCap = 0;
    m_nMaxDirectWriteBytes = 0;
}

CCircleBuffer::~CCircleBuffer()
{
    SAFE_ARRAY_DELETE(m_pBuffer)
}

void CCircleBuffer::CreateBuffer(uint32 nBytes, uint32 nMaxDirectWriteBytes)
{
    SAFE_ARRAY_DELETE(m_pBuffer)
    
    m_nMaxDirectWriteBytes = nMaxDirectWriteBytes;
    m_nTotal = nBytes + 1 + nMaxDirectWriteBytes;
    m_pBuffer = new unsigned char [(unsigned int) m_nTotal];
    m_nHead = 0;
    m_nTail = 0;
    m_nEndCap = m_nTotal;
}

uint32 CCircleBuffer::MaxAdd()
{
    uint32 nMaxAdd = (m_nTail >= m_nHead) ? (m_nTotal - 1 - m_nMaxDirectWriteBytes) - (m_nTail - m_nHead) : m_nHead - m_nTail - 1;
    return nMaxAdd;
}

uint32 CCircleBuffer::MaxGet()
{
    return (m_nTail >= m_nHead) ? m_nTail - m_nHead : (m_nEndCap - m_nHead) + m_nTail;
}

uint32 CCircleBuffer::UpdateCRC(uint32 nCRC, uint32 nBytes)
{
    uint32 nFrontBytes = ape_min(m_nTail, nBytes);
    uint32 nHeadBytes = nBytes - nFrontBytes;

    if (nHeadBytes > 0)
        nCRC = CRC_update(nCRC, &m_pBuffer[m_nEndCap - nHeadBytes], nHeadBytes);

    nCRC = CRC_update(nCRC, &m_pBuffer[m_nTail - nFrontBytes], nFrontBytes);

    return nCRC;
}

uint32 CCircleBuffer::Get(unsigned char * pBuffer, uint32 nBytes)
{
    uint32 nTotalGetBytes = 0;

    if (pBuffer != NULL && nBytes > 0)
    {
        uint32 nHeadBytes = ape_min(m_nEndCap - m_nHead, nBytes);
        uint32 nFrontBytes = nBytes - nHeadBytes;

        memcpy(&pBuffer[0], &m_pBuffer[m_nHead], (size_t) nHeadBytes);
        nTotalGetBytes = nHeadBytes;

        if (nFrontBytes > 0)
        {
            memcpy(&pBuffer[nHeadBytes], &m_pBuffer[0], (size_t) nFrontBytes);
            nTotalGetBytes += nFrontBytes;
        }

        RemoveHead(nBytes);
    }

    return nTotalGetBytes;
}

void CCircleBuffer::Empty()
{
    m_nHead = 0;
    m_nTail = 0;
    m_nEndCap = m_nTotal;
}

uint32 CCircleBuffer::RemoveHead(uint32 nBytes)
{
    nBytes = ape_min(MaxGet(), nBytes);
    m_nHead += nBytes;
    if (m_nHead >= m_nEndCap)
        m_nHead -= m_nEndCap;
    return nBytes;
}

uint32 CCircleBuffer::RemoveTail(uint32 nBytes)
{
    nBytes = ape_min(MaxGet(), nBytes);
    if (m_nTail < nBytes)
        m_nTail += m_nEndCap;
    m_nTail -= nBytes;
    return nBytes;
}

}