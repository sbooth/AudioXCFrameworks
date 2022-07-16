#ifdef IO_USE_WIN_FILE_IO

#pragma once

#include "IO.h"

namespace APE
{

class CWinFileIO : public CIO
{
public:
    // construction / destruction
    CWinFileIO();
    ~CWinFileIO();

    // open / close
    int Open(const wchar_t * pName, bool bOpenReadOnly = false);
    int Close();
    
    // read / write
    int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead);
    int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten);
    
    // seek
    int64 PerformSeek();
    
    // other functions
    int SetEOF();
    int SetReadWholeFile();
    void SetReadToBuffer();
    unsigned char * GetBuffer(int * pnBufferBytes);

    // creation / destruction
    int Create(const wchar_t * pName);
    int Delete();

    // attributes
    int64 GetPosition();
    int64 GetSize();
    int GetName(wchar_t * pBuffer);

private:
    
    HANDLE      m_hFile;
    wchar_t     m_cFileName[MAX_PATH];
    bool        m_bReadOnly;
    bool        m_bWholeFile;
    CSmartPtr<unsigned char> m_spWholeFile;
    int64       m_nWholeFilePointer;
    CSmartPtr<unsigned char> m_spBuffer;
    int         m_nBufferBytes;
    bool        m_bReadToBuffer;
    int64       m_nWholeFileSize;
};

}

#endif //IO_USE_WIN_FILE_IO