#include "All.h"

#ifdef IO_USE_WIN_FILE_IO

#include "WinFileIO.h"
#include "CharacterHelper.h"

namespace APE
{

CIO * CreateCIO()
{
    return new CWinFileIO;
}

CWinFileIO::CWinFileIO()
{
    m_hFile = INVALID_HANDLE_VALUE;
    APE_CLEAR(m_cFileName);
    m_bReadOnly = false;
}

CWinFileIO::~CWinFileIO()
{
    APE_SAFE_FILE_CLOSE(m_hFile)
}

int CWinFileIO::Open(const wchar_t * pName, bool bOpenReadOnly)
{
    Close();

    if (wcslen(pName) >= APE_MAX_PATH)
        return ERROR_UNDEFINED;

    #ifdef _UNICODE
        wchar_t * pCopy = new wchar_t [wcslen(pName) + 1];
        memcpy(pCopy, pName, sizeof(wchar_t) * wcslen(pName));
        pCopy[wcslen(pName)] = 0;
        CSmartPtr<wchar_t> spName(pCopy, true);
    #else
        CSmartPtr<char> spName(CAPECharacterHelper::GetANSIFromUTF16(pName), true);
    #endif

    // handle pipes vs files
    if (0 == wcscmp(pName, L"-"))
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
            m_hFile = ::CreateFile(spName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, APE_NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, APE_NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            // open file (read-only)
            m_hFile = ::CreateFile(spName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, APE_NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, APE_NULL);
            if (m_hFile == INVALID_HANDLE_VALUE)
            {
                const DWORD dwError = GetLastError();
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

    wcscpy_s(m_cFileName, APE_MAX_PATH, pName);

    return ERROR_SUCCESS;
}

int CWinFileIO::Close()
{
    APE_SAFE_FILE_CLOSE(m_hFile)

    return ERROR_SUCCESS;
}

int CWinFileIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
    bool bRetVal = true;

    *pBytesRead = 0; // reset

    unsigned int nTotalBytesRead = 0;
    unsigned int nBytesLeft = nBytesToRead;
    unsigned char * pucBuffer = static_cast<unsigned char *>(pBuffer);

    *pBytesRead = 1;
    while ((nBytesLeft > 0) && (*pBytesRead > 0) && bRetVal)
    {
        unsigned long nBytesRead = 0;
        bRetVal = ::ReadFile(m_hFile, &pucBuffer[nBytesToRead - nBytesLeft], nBytesLeft, &nBytesRead, APE_NULL) ? true : false;
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

    // succeed if we actually read data then fail next call
    if ((bRetVal == false) && (*pBytesRead > 0))
        bRetVal = true;

    return bRetVal ? ERROR_SUCCESS : ERROR_IO_READ;
}

int CWinFileIO::Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten)
{
    const bool bRetVal = WriteFile(m_hFile, pBuffer, nBytesToWrite, reinterpret_cast<unsigned long *>(pBytesWritten), APE_NULL) ? true : false;
    if ((bRetVal == 0) || (*pBytesWritten != nBytesToWrite))
        return ERROR_IO_WRITE;
    else
        return ERROR_SUCCESS;
}

int CWinFileIO::Seek(int64 nPosition, SeekMethod nMethod)
{
    DWORD dwMoveMethod = 0;
    if (nMethod == SeekFileBegin)
    {
        dwMoveMethod = FILE_BEGIN;
    }
    else if (nMethod == SeekFileEnd)
    {
        nPosition = -abs(nPosition);
        dwMoveMethod = FILE_END;
    }
    else if (nMethod == SeekFileCurrent)
    {
        dwMoveMethod = FILE_CURRENT;
    }

    const LONG Low = static_cast<LONG>(nPosition & 0xFFFFFFFF);
    LONG High = static_cast<LONG>(nPosition >> 32);

    SetFilePointer(m_hFile, Low, &High, dwMoveMethod);

    return ERROR_SUCCESS;
}

int CWinFileIO::SetEOF()
{
    // set the file EOF
    return SetEndOfFile(m_hFile) ? ERROR_SUCCESS : ERROR_UNDEFINED;
}

int64 CWinFileIO::GetPosition()
{
    LONG nPositionHigh = 0;
    const DWORD dwPositionLow = SetFilePointer(m_hFile, 0, &nPositionHigh, FILE_CURRENT);
    const int64 nPosition = static_cast<int64>(dwPositionLow) + (static_cast<int64>(nPositionHigh) << 32);
    return nPosition;
}

int64 CWinFileIO::GetSize()
{
    DWORD dwFileSizeHigh = 0;
    const DWORD dwFileSizeLow = GetFileSize(m_hFile, &dwFileSizeHigh);
    return static_cast<int64>(dwFileSizeLow) + (static_cast<int64>(dwFileSizeHigh) << 32);
}

int CWinFileIO::GetName(wchar_t * pBuffer)
{
    wcscpy_s(pBuffer, APE_MAX_PATH, m_cFileName);
    return ERROR_SUCCESS;
}

int CWinFileIO::Create(const wchar_t * pName)
{
    Close();

    if (wcslen(pName) >= APE_MAX_PATH)
        return ERROR_UNDEFINED;

    #ifdef _UNICODE
        wchar_t * pCopy = new wchar_t [wcslen(pName) + 1];
        memcpy(pCopy, pName, sizeof(wchar_t) * wcslen(pName));
        pCopy[wcslen(pName)] = 0;
        CSmartPtr<wchar_t> spName(pCopy, true);
    #else
        CSmartPtr<char> spName(CAPECharacterHelper::GetANSIFromUTF16(pName), true);
    #endif

    if (0 == wcscmp(pName, L"-"))
    {
        m_hFile = GetStdHandle(STD_OUTPUT_HANDLE);
        if (m_hFile == INVALID_HANDLE_VALUE)
            return ERROR_IO_WRITE;

        m_bReadOnly = false;
    }
    else
    {
        m_hFile = CreateFile(spName, GENERIC_WRITE | GENERIC_READ, 0, APE_NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, APE_NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
            return ERROR_IO_WRITE;

        m_bReadOnly = false;
    }

    wcscpy_s(m_cFileName, APE_MAX_PATH, pName);

    return ERROR_SUCCESS;
}

int CWinFileIO::Delete()
{
    Close();

    #ifdef _UNICODE
        CSmartPtr<wchar_t> spName(m_cFileName, true, false);
    #else
        CSmartPtr<char> spName(CAPECharacterHelper::GetANSIFromUTF16(m_cFileName), true);
    #endif

    SetFileAttributes(spName, FILE_ATTRIBUTE_NORMAL);
    return DeleteFile(spName) ? ERROR_SUCCESS : ERROR_UNDEFINED;
}

}

#endif // #ifdef IO_USE_WIN_FILE_IO
