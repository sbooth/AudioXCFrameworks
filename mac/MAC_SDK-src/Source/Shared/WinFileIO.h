#ifdef IO_USE_WIN_FILE_IO

#pragma once

#include "IO.h"

namespace APE
{

#pragma pack(push, 1)

class CWinFileIO : public CIO
{
public:
    // construction / destruction
    CWinFileIO();
    ~CWinFileIO();

    // open / close
    int Open(const wchar_t * pName, bool bOpenReadOnly = false) APE_OVERRIDE;
    int Close() APE_OVERRIDE;

    // read / write
    int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead) APE_OVERRIDE;
    int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten) APE_OVERRIDE;

    // seek
    int Seek(int64 nPosition, SeekMethod nMethod) APE_OVERRIDE;

    // other functions
    int SetEOF() APE_OVERRIDE;
    unsigned char * GetBuffer(int *)  APE_OVERRIDE { return APE_NULL; }

    // creation / destruction
    int Create(const wchar_t * pName) APE_OVERRIDE;
    int Delete() APE_OVERRIDE;

    // attributes
    int64 GetPosition() APE_OVERRIDE;
    int64 GetSize() APE_OVERRIDE;
    int GetName(wchar_t * pBuffer) APE_OVERRIDE;

private:
    CSmartPtr<unsigned char> m_spBuffer;
    HANDLE      m_hFile;
    wchar_t     m_cFileName[APE_MAX_PATH];
    bool        m_bReadOnly;
};

#pragma pack(pop)

}

#endif //IO_USE_WIN_FILE_IO
