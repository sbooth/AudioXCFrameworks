#include "All.h"
#include "BufferIO.h"

namespace APE
{

CBufferIO::CBufferIO(CIO * pSource, int nBufferBytes)
{
    m_spSource.Assign(pSource);
    m_nBufferBytes = 0;
    m_nBufferTotalBytes = nBufferBytes;
    m_spBuffer.Assign(new unsigned char [static_cast<size_t>(m_nBufferTotalBytes)], true);
    m_bReadToBuffer = true;
}

CBufferIO::~CBufferIO()
{
    m_spSource->Close();
    m_spSource.Delete();
}

int CBufferIO::Open(const wchar_t * pName, bool bOpenReadOnly)
{
    return m_spSource->Open(pName, bOpenReadOnly);
}

int CBufferIO::Close()
{
    return m_spSource->Close();
}

int CBufferIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
    const int nResult = m_spSource->Read(pBuffer, nBytesToRead, pBytesRead);

    if (m_bReadToBuffer && (m_spBuffer != APE_NULL) && (*pBytesRead > 0))
    {
        const int nBufferBytes = ape_min(m_nBufferTotalBytes - m_nBufferBytes, *reinterpret_cast<int *>(pBytesRead));
        if (nBufferBytes <= 0)
        {
            m_bReadToBuffer = false;
        }
        else
        {
            memcpy(&m_spBuffer[m_nBufferBytes], pBuffer, static_cast<size_t>(nBufferBytes));
            m_nBufferBytes += *pBytesRead;
        }
    }

    return nResult;
}

int CBufferIO::Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten)
{
    return m_spSource->Write(pBuffer, nBytesToWrite, pBytesWritten);
}

int CBufferIO::Seek(int64 nPosition, SeekMethod nMethod)
{
    m_bReadToBuffer = false; // we seeked, so stop buffering

    // perform the seek on the underlying source
    return m_spSource->Seek(nPosition, nMethod);
}

int CBufferIO::SetEOF()
{
    return m_spSource->SetEOF();
}

unsigned char * CBufferIO::GetBuffer(int * pnBufferBytes)
{
    if (*pnBufferBytes > m_nBufferTotalBytes)
        return APE_NULL; // the request exceeded the size we stored, so return NULL
    ASSERT(*pnBufferBytes == m_nBufferBytes);
    *pnBufferBytes = m_nBufferBytes;
    m_bReadToBuffer = false; // no longer needed
    return m_spBuffer;
}

int64 CBufferIO::GetPosition()
{
    if (m_bReadToBuffer)
    {
        // getting the position on a pipe doesn't work, so we need to do this
        return m_nBufferBytes;
    }
    else
    {
        return m_spSource->GetPosition();
    }
}

int64 CBufferIO::GetSize()
{
    return m_spSource->GetSize();
}

int CBufferIO::GetName(wchar_t * pBuffer)
{
    return m_spSource->GetName(pBuffer);
}

int CBufferIO::Create(const wchar_t * pName)
{
    return m_spSource->Create(pName);
}

int CBufferIO::Delete()
{
    return m_spSource->Delete();
}

}
