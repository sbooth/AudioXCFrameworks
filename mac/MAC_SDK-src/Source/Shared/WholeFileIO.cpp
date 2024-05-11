#include "All.h"
#include "WholeFileIO.h"

namespace APE
{

CWholeFileIO * CreateWholeFileIO(CIO * pSource, int64 nSize)
{
    // the size should be correct
    ASSERT(nSize == pSource->GetSize());

    int nResult = ERROR_SUCCESS;

    // make sure we're at the head of the file
    pSource->Seek(0, SeekFileBegin);

    // the the size as 32-bit
    const uint32 n32BitSize = static_cast<uint32>(nSize); // we established it will fit above

    unsigned char * pWholeFile = APE_NULL;
    if (nSize == n32BitSize)
    {
        // create a buffer
        try
        {
            pWholeFile = new unsigned char [n32BitSize];
            //pWholeFile = new unsigned char[0x7FFFFFFF]; // test for allocating a huge size
        }
        catch (...)
        {
        }
    }

    CWholeFileIO * pIO = APE_NULL;
    if (pWholeFile != APE_NULL)
    {
        // read
        unsigned int nBytesRead = 0;
        nResult = pSource->Read(pWholeFile, n32BitSize, &nBytesRead);
        if (nBytesRead < n32BitSize)
            nResult = ERROR_IO_READ;

        if (nResult == ERROR_SUCCESS)
        {
            // create IO object
            pIO = new CWholeFileIO(pSource, pWholeFile, nBytesRead);
        }
    }

    return pIO;
}

CWholeFileIO::CWholeFileIO(CIO * pSource, unsigned char * pBuffer, int64 nFileBytes)
{
    // store source
    m_spSource.Assign(pSource);

    // create a buffer
    m_spWholeFile.Assign(pBuffer, true);
    m_nWholeFilePointer = 0;
    m_nWholeFileSize = nFileBytes;
}

CWholeFileIO::~CWholeFileIO()
{
    m_spSource->Close();
    m_spSource.Delete();
}

int CWholeFileIO::Open(const wchar_t *, bool)
{
    return ERROR_SUCCESS;
}

int CWholeFileIO::Close()
{
    return m_spSource->Close();
}

int CWholeFileIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
    bool bRetVal = true;

    *pBytesRead = 0; // reset

    const int64 nBytesLeft = GetSize() - m_nWholeFilePointer;
    nBytesToRead = ape_min(static_cast<unsigned int> (nBytesLeft), nBytesToRead);
    memcpy(pBuffer, &m_spWholeFile[m_nWholeFilePointer], nBytesToRead);
    m_nWholeFilePointer += nBytesToRead;
    *pBytesRead = nBytesToRead;

    // succeed if we actually read data then fail next call
    if ((bRetVal == false) && (*pBytesRead > 0))
        bRetVal = true;

    return bRetVal ? ERROR_SUCCESS : ERROR_IO_READ;
}

int CWholeFileIO::Write(const void * , unsigned int , unsigned int *)
{
    return ERROR_IO_WRITE;
}

int CWholeFileIO::Seek(int64 nPosition, SeekMethod nMethod)
{
    if (nMethod == SeekFileBegin)
    {
        m_nWholeFilePointer = nPosition;
    }
    else if (nMethod == SeekFileCurrent)
    {
        m_nWholeFilePointer += nPosition;
    }
    else if (nMethod == SeekFileEnd)
    {
        m_nWholeFilePointer = GetSize() - abs(nPosition);
    }

    return ERROR_SUCCESS;
}

int CWholeFileIO::SetEOF()
{
    // handle memory buffers
    m_nWholeFileSize = m_nWholeFilePointer;

    // seek in the actual file to where we're at
    m_spSource->Seek(m_nWholeFilePointer, SeekFileBegin);

    // set the EOF in the actual file
    return m_spSource->SetEOF();
}

int64 CWholeFileIO::GetPosition()
{
    return m_nWholeFilePointer;
}

int64 CWholeFileIO::GetSize()
{
    return m_nWholeFileSize;
}

int CWholeFileIO::GetName(wchar_t *)
{
    return ERROR_UNDEFINED;
}

int CWholeFileIO::Create(const wchar_t *)
{
    return ERROR_UNDEFINED;
}

int CWholeFileIO::Delete()
{
    return ERROR_UNDEFINED;
}

}
