#pragma once

#include "IO.h"

namespace APE
{
#pragma pack(push, 1)

/**************************************************************************************************
CBufferIO
**************************************************************************************************/
class CBufferIO : public CIO
{
public:
    // construction / destruction
    CBufferIO(CIO * pSource, int nBufferBytes);
    ~CBufferIO();

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
    unsigned char * GetBuffer(int * pnBufferBytes) APE_OVERRIDE;

    // creation / destruction
    int Create(const wchar_t * pName) APE_OVERRIDE;
    int Delete() APE_OVERRIDE;

    // attributes
    int64 GetPosition() APE_OVERRIDE;
    int64 GetSize() APE_OVERRIDE;
    int GetName(wchar_t * pBuffer) APE_OVERRIDE;

private:
    CSmartPtr<CIO> m_spSource;
    CSmartPtr<unsigned char> m_spBuffer;
    int m_nBufferBytes;
    int m_nBufferTotalBytes;
    bool m_bReadToBuffer;
};

/**************************************************************************************************
CHeaderIO
**************************************************************************************************/
class CHeaderIO : public CIO
{
public:
    // construction / destruction
    CHeaderIO(CIO * pSource);
    ~CHeaderIO();

    // read the header
    bool ReadHeader(BYTE * paryHeader);

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
    CSmartPtr<CIO> m_spSource;
    int64 m_nHeaderBytes;
    BYTE m_aryHeader[64];
    int64 m_nPosition;
};

#pragma pack(pop)

}
