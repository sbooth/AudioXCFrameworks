#ifdef IO_USE_STD_LIB_FILE_IO

#pragma once

#include "IO.h"

namespace APE
{

class CStdLibFileIO : public CIO
{
public:
    // construction / destruction
    CStdLibFileIO();
    ~CStdLibFileIO();

    // open / close
    int Open(const wchar_t * pName, bool bOpenReadOnly = false);
    int Close();

    // read / write
    int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead);
    int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten);

    // seek
    int Seek(int64 nPosition, SeekMethod nMethod);

    // other functions
    int SetEOF();
    unsigned char * GetBuffer(int *) { return APE_NULL; }

    // creation / destruction
    int Create(const wchar_t * pName);
    int Delete();

    // attributes
    int64 GetPosition();
    int64 GetSize();
    int GetName(wchar_t * pBuffer);
    int GetHandle();

private:
    wchar_t m_cFileName[MAX_PATH];
    bool m_bReadOnly;
    FILE * m_pFile;
};

}

#endif // #ifdef IO_USE_STD_LIB_FILE_IO
