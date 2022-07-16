#include "All.h"

#ifdef IO_USE_WIN_FILE_IO

#include "WinFileIO.h"
#include <windows.h>
#include "CharacterHelper.h"

#define IO_BUFFER_BYTES 65536

namespace APE
{

CIO * CreateCIO()
{
    return new CWinFileIO;
}
    
CWinFileIO::CWinFileIO()
{
    m_hFile = INVALID_HANDLE_VALUE;
    memset(m_cFileName, 0, sizeof(m_cFileName));
    m_bReadOnly = false;
    m_bWholeFile = false;
    m_nWholeFilePointer = 0;
    m_nBufferBytes = 0;
    m_bReadToBuffer = false;
    m_nWholeFileSize = -1;
}

CWinFileIO::~CWinFileIO()
{
    Close();
}

int CWinFileIO::Open(const wchar_t * pName, bool bOpenReadOnly)
{
    Close();

    if (wcslen(pName) >= MAX_PATH)
        return ERROR_UNDEFINED;

    #ifdef _UNICODE
        CSmartPtr<wchar_t> spName((wchar_t *) pName, true, false);
    #else
        CSmartPtr<char> spName(GetANSIFromUTF16(pName), true);
    #endif

    // handle pipes vs files
    if (0 == wcscmp(pName, _T("-")))
    {
        // pipes are read-only
        m_hFile = GetStdHandle(STD_INPUT_HANDLE);
        m_bReadOnly = true;
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            return ERROR_INVALID_INPUT_FILE;
        }
    }
    else
    {
        // open file (read / write)
        if (!bOpenReadOnly && (m_hFile == INVALID_HANDLE_VALUE))
            m_hFile = ::CreateFile(spName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            // open file (read-only)
            m_hFile = ::CreateFile(spName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (m_hFile == INVALID_HANDLE_VALUE)
            {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_SHARING_VIOLATION)
                {
                    // file in use
                    return ERROR_OPENING_FILE_IN_USE;
                }
                else
                {
                    // dwError of 2 means not found
                    return ERROR_INVALID_INPUT_FILE;
                }
            }
            else
            {
                m_bReadOnly = true;
            }
        }
        else
        {
            m_bReadOnly = false;
        }
    }
    
    wcscpy_s(m_cFileName, MAX_PATH, pName);

    return ERROR_SUCCESS;
}

int CWinFileIO::Close()
{
    SAFE_FILE_CLOSE(m_hFile)
    m_spWholeFile.Delete();

    return ERROR_SUCCESS;
}
    
int CWinFileIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
    bool bRetVal = true;

    *pBytesRead = 0; // reset

    if (m_bWholeFile)
    {
        int64 nBytesLeft = GetSize() - m_nWholeFilePointer;
        nBytesToRead = ape_min((unsigned int) nBytesLeft, nBytesToRead);
        memcpy(pBuffer, &m_spWholeFile[m_nWholeFilePointer], nBytesToRead);
        m_nWholeFilePointer += nBytesToRead;
        *pBytesRead = nBytesToRead;
    }
    else
    {
        unsigned int nTotalBytesRead = 0;
        int nBytesLeft = nBytesToRead;
        unsigned char * pucBuffer = (unsigned char *) pBuffer;

        *pBytesRead = 1;
        while ((nBytesLeft > 0) && (*pBytesRead > 0) && bRetVal)
        {
            unsigned long nBytesRead = 0;
            bRetVal = ::ReadFile(m_hFile, &pucBuffer[nBytesToRead - nBytesLeft], nBytesLeft, &nBytesRead, NULL) ? true : false;
            *pBytesRead = nBytesRead;
            if (bRetVal && (*pBytesRead <= 0))
                bRetVal = false;

            if (bRetVal)
            {
                nBytesLeft -= *pBytesRead;
                nTotalBytesRead += *pBytesRead;
            }
        }

        *pBytesRead = nTotalBytesRead;
    }

    if (m_bReadToBuffer && (m_spBuffer != NULL) && (*pBytesRead > 0))
    {
        int nBufferBytes = ape_min(IO_BUFFER_BYTES - m_nBufferBytes, *(int *)pBytesRead);
        if (nBufferBytes <= 0)
        {
            m_bReadToBuffer = false;
        }
        else
        {
            memcpy(&m_spBuffer[m_nBufferBytes], pBuffer, nBufferBytes);
            m_nBufferBytes += *pBytesRead;
        }
    }

    // succeed if we actually read data then fail next call
    if ((bRetVal == false) && (*pBytesRead > 0))
        bRetVal = true;

    return bRetVal ? ERROR_SUCCESS : ERROR_IO_READ;
}

int CWinFileIO::Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten)
{
    bool bRetVal = WriteFile(m_hFile, pBuffer, nBytesToWrite, (unsigned long *) pBytesWritten, NULL) ? true : false;

    if ((bRetVal == 0) || (*pBytesWritten != nBytesToWrite))
        return ERROR_IO_WRITE;
    else
        return ERROR_SUCCESS;
}

int64 CWinFileIO::PerformSeek()
{
    if (m_bWholeFile)
    {
        if (m_nSeekMethod == APE_FILE_BEGIN)
            m_nWholeFilePointer = m_nSeekPosition;
        else if (m_nSeekMethod == APE_FILE_END)
            m_nWholeFilePointer = GetSize() - abs(m_nSeekPosition);
        else if (m_nSeekMethod == APE_FILE_CURRENT)
            m_nWholeFilePointer += m_nSeekPosition;
    }
    else
    {
        DWORD dwMoveMethod = 0;
        if (m_nSeekMethod == APE_FILE_BEGIN)
            dwMoveMethod = FILE_BEGIN;
        else if (m_nSeekMethod == APE_FILE_END)
            dwMoveMethod = FILE_END;
        else if (m_nSeekMethod == APE_FILE_CURRENT)
            dwMoveMethod = FILE_CURRENT;

        LONG Low = (m_nSeekPosition & 0xFFFFFFFF);
        LONG High = (m_nSeekPosition >> 32);

        SetFilePointer(m_hFile, Low, &High, dwMoveMethod);
    }

    return ERROR_SUCCESS;
}

int CWinFileIO::SetEOF()
{
    // handle memory buffers
    if (m_bWholeFile)
    {
        m_nWholeFileSize = m_nWholeFilePointer;

        LONG Low = (m_nWholeFilePointer & 0xFFFFFFFF);
        LONG High = (m_nWholeFilePointer >> 32);
        SetFilePointer(m_hFile, Low, &High, FILE_BEGIN);
    }        
    
    // set the file EOF
    return SetEndOfFile(m_hFile) ? ERROR_SUCCESS : ERROR_UNDEFINED;
}

int CWinFileIO::SetReadWholeFile()
{
    int nResult = ERROR_SUCCESS;

    if (m_nBufferBytes > 0)
    {
        m_bReadToBuffer = false; // turn off buffering
        unsigned int nBytesRead = 0;

        // read the header to the total buffer
        m_nWholeFileSize = m_nBufferBytes;
        m_spWholeFile.Assign(new BYTE [m_nBufferBytes], true);
        memcpy(m_spWholeFile, m_spBuffer, m_nBufferBytes);
        
        const int nReadBlockBytes = BYTES_IN_MEGABYTE * 4;
        CSmartPtr<BYTE> spBufferRead(new BYTE [nReadBlockBytes], true);
        while (Read(spBufferRead, nReadBlockBytes, &nBytesRead) == ERROR_SUCCESS)
        {
            // stop reading at 1 GB
            if ((m_nWholeFileSize + nBytesRead) >= (BYTES_IN_MEGABYTE * 1000))
                return ERROR_INPUT_FILE_TOO_LARGE;

            // make a copy
            CSmartPtr<BYTE> spBufferTotalCopy;
            spBufferTotalCopy.Assign(new BYTE [(unsigned int) m_nWholeFileSize], true);
            memcpy(spBufferTotalCopy, m_spWholeFile, (size_t) m_nWholeFileSize);

            // allocate at the new size
            m_spWholeFile.Assign(new BYTE [(unsigned int) (m_nWholeFileSize + nBytesRead)], true);

            // copy again
            memcpy(m_spWholeFile, spBufferTotalCopy, (size_t) m_nWholeFileSize);

            // put read data in
            memcpy(&m_spWholeFile[m_nWholeFileSize], spBufferRead, nBytesRead);
            m_nWholeFileSize += nBytesRead;
        }

        m_bWholeFile = true; // flag after read or else read will copy out of the buffer
    }
    else
    {
        int64 nSize = GetSize();
        if (nSize < (BYTES_IN_MEGABYTE * 100))
        {
            // make sure we're at the head of the file
            SetSeekPosition(0);
            SetSeekMethod(APE_FILE_BEGIN);
            PerformSeek();

            // the the size as 32-bit
            uint32 n32BitSize = (uint32) nSize; // we established it will fit above
            
            // create a buffer
            m_spWholeFile.Assign(new unsigned char [n32BitSize], true);

            // read
            unsigned int nBytesRead = 0;
            nResult = Read(m_spWholeFile, n32BitSize, &nBytesRead);
            if (nBytesRead < n32BitSize)
                nResult = ERROR_IO_READ;
            m_nWholeFilePointer = 0;
            m_nWholeFileSize = n32BitSize;
            m_bWholeFile = true; // flag after read or else read will copy out of the buffer
        }
    }

    return nResult;
}

void CWinFileIO::SetReadToBuffer()
{
    m_nBufferBytes = 0;
    m_spBuffer.Assign(new unsigned char [IO_BUFFER_BYTES], true);
    m_bReadToBuffer = true;
}

unsigned char * CWinFileIO::GetBuffer(int * pnBufferBytes)
{
    if (*pnBufferBytes > IO_BUFFER_BYTES)
        return NULL; // the request exceeded the size we stored, so return NULL
    *pnBufferBytes = m_nBufferBytes;
    m_bReadToBuffer = false; // no longer needed
    return m_spBuffer;
}

int64 CWinFileIO::GetPosition()
{
    if (m_bWholeFile)
    {
        return m_nWholeFilePointer;
    }
    else
    {
        if (m_bReadToBuffer)
        {
            // getting the position on a pipe doesn't work, so we need to do this
            return m_nBufferBytes;
        }
        else
        {
            LONG nPositionHigh = 0;
            DWORD dwPositionLow = SetFilePointer(m_hFile, 0, &nPositionHigh, FILE_CURRENT);
            int64 nPosition = int64(dwPositionLow) + (int64(nPositionHigh) << 32);
            return nPosition;
        }
    }
}

int64 CWinFileIO::GetSize()
{
    if (m_nWholeFileSize != -1)
        return m_nWholeFileSize;

    DWORD dwFileSizeHigh = 0;
    DWORD dwFileSizeLow = GetFileSize(m_hFile, &dwFileSizeHigh);
    return int64(dwFileSizeLow) + (int64(dwFileSizeHigh) << 32);
}

int CWinFileIO::GetName(wchar_t * pBuffer)
{
    wcscpy_s(pBuffer, MAX_PATH, m_cFileName);
    return ERROR_SUCCESS;
}

int CWinFileIO::Create(const wchar_t * pName)
{
    Close();

    if (wcslen(pName) >= MAX_PATH)
        return ERROR_UNDEFINED;

    #ifdef _UNICODE
        CSmartPtr<wchar_t> spName((wchar_t *) pName, true, false);
    #else
        CSmartPtr<char> spName(GetANSIFromUTF16(pName), true);
    #endif

    if (0 == wcscmp(pName, _T("-")))
    {
        m_hFile = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_hFile == INVALID_HANDLE_VALUE)
            return ERROR_IO_WRITE;

        m_bReadOnly = false;
    }
    else
    {
        m_hFile = CreateFile(spName, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
            return ERROR_IO_WRITE;

        m_bReadOnly = false;
    }
    
    wcscpy_s(m_cFileName, MAX_PATH, pName);

    return ERROR_SUCCESS;
}

int CWinFileIO::Delete()
{
    Close();

    #ifdef _UNICODE
        CSmartPtr<wchar_t> spName(m_cFileName, true, false);
    #else
        CSmartPtr<char> spName(GetANSIFromUTF16(m_cFileName), true);
    #endif

    SetFileAttributes(spName, FILE_ATTRIBUTE_NORMAL);
    return DeleteFile(spName) ? ERROR_SUCCESS : ERROR_UNDEFINED;
}

}

#endif // #ifdef IO_USE_WIN_FILE_IO
