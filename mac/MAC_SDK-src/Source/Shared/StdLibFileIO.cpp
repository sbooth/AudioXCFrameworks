#if defined(PLATFORM_APPLE) || defined(PLATFORM_ANDROID)
#define _FILE_OFFSET_BITS 64
#endif

#include "All.h"

#ifdef IO_USE_STD_LIB_FILE_IO

#include "StdLibFileIO.h"
#include "CharacterHelper.h"

///////////////////////////////////////////////////////

// low level I/O, where are prototypes and constants?
#if   defined _WIN32  ||  defined __TURBOC__  ||  defined __ZTC__  ||  defined _MSC_VER
# include <io.h>
# include <fcntl.h>
# include <time.h>
# include <sys/types.h>
# include <sys/stat.h>
#elif defined __unix__  ||  defined __linux__
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/time.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <sys/stat.h>
#else
# include <errno.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/ioctl.h>
# include <sys/stat.h>
#endif


#ifndef O_BINARY
# ifdef _O_BINARY
#  define O_BINARY              _O_BINARY
# else
#  define O_BINARY              0
# endif
#endif

//// Binary/Low-Level-IO ///////////////////////////////////////////
//
// All file I/O is basicly handled via an ANSI file pointer (type: FILE*) in
// FILEIO-Mode 1 and via a POSIX file descriptor (type: int) in
// FILEIO-Mode 2 and 3.
//
// Some operation are only available via the POSIX interface (fcntl, setmode,
// ...) so we need a function to get the file descriptor from a file pointer.
// In FILEIO-Mode 2 and 3 this is a dummy function because we always working
// with this file descriptors.
//

#if   defined __BORLANDC__  ||  defined _WIN32
# define FILENO(__fp)          _fileno ((__fp))
#elif defined __CYGWIN__  ||  defined __TURBOC__  ||  defined __unix__  ||  defined __EMX__  ||  defined _MSC_VER
# define FILENO(__fp)          fileno  ((__fp))
#else
# define FILENO(__fp)          fileno  ((__fp))
#endif


//
// If we have access to a file via file name, we can open the file with an
// additional "b" or a O_BINARY within the (f)open function to get a
// transparent untranslated data stream which is necessary for audio bitstream
// data and also for PCM data. If we are working with
// stdin/stdout/FILENO_STDIN/FILENO_STDOUT we can't open the file with this
// attributes, because the files are already open. So we need a non
// standardized sequence to switch to this mode (not necessary for Unix).
// Mostly the sequency is the same for incoming and outgoing streams, but only
// mostly so we need one for IN and one for OUT.
// Macros are called with the file pointer and you get back the untransalted file
// pointer which can be equal or different from the original.
//

#if   defined __EMX__
# define SETBINARY_IN(__fp)     (_fsetmode ( (__fp), "b" ), (__fp))
# define SETBINARY_OUT(__fp)    (_fsetmode ( (__fp), "b" ), (__fp))
#elif defined __TURBOC__ || defined __BORLANDC__
# define SETBINARY_IN(__fp)     (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
#elif defined __CYGWIN__
# define SETBINARY_IN(__fp)     (setmode   ( FILENO ((__fp)), _O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (setmode   ( FILENO ((__fp)), _O_BINARY ), (__fp))
#elif defined _WIN32
# define SETBINARY_IN(__fp)     (_setmode  ( FILENO ((__fp)), _O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (_setmode  ( FILENO ((__fp)), _O_BINARY ), (__fp))
#elif defined _MSC_VER
# define SETBINARY_IN(__fp)     (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
# define SETBINARY_OUT(__fp)    (setmode   ( FILENO ((__fp)),  O_BINARY ), (__fp))
#elif defined __unix__
# define SETBINARY_IN(__fp)     (__fp)
# define SETBINARY_OUT(__fp)    (__fp)
#elif 0
# define SETBINARY_IN(__fp)     (freopen   ( NULL, "rb", (__fp) ), (__fp))
# define SETBINARY_OUT(__fp)    (freopen   ( NULL, "wb", (__fp) ), (__fp))
#else
# define SETBINARY_IN(__fp)     (__fp)
# define SETBINARY_OUT(__fp)    (__fp)
#endif


namespace APE
{

CIO * CreateCIO()
{
    return new CStdLibFileIO;
}

CStdLibFileIO::CStdLibFileIO()
{
    APE_CLEAR(m_cFileName);
    m_bReadOnly = false;
    m_bPipe = false;
    m_pFile = APE_NULL;
}

CStdLibFileIO::~CStdLibFileIO()
{
    Close();
}

int CStdLibFileIO::GetHandle()
{
    return FILENO(m_pFile);
}

int CStdLibFileIO::Open(const wchar_t * pName, bool)
{
    Close();

    if (wcslen(pName) >= MAX_PATH)
        return ERROR_UNDEFINED;

    m_bReadOnly = false;

    if (0 == wcscmp(pName, L"-") || 0 == wcscmp(pName, L"/dev/stdin"))
    {
        m_pFile = SETBINARY_IN(stdin);
        m_bReadOnly = true;                                                     // ReadOnly
        m_bPipe = true;
    }
    else if (0 == wcscmp(pName, L"/dev/stdout"))
    {
        m_pFile = SETBINARY_OUT(stdout);
        m_bReadOnly = false;                                                    // WriteOnly
    }
    else
    {
        CSmartPtr<char> spFilenameUTF8((char *) CAPECharacterHelper::GetUTF8FromUTF16(pName), true);
        m_pFile = fopen(spFilenameUTF8, "r+be");
        if (m_pFile == APE_NULL && (errno == EACCES || errno == EPERM || errno == EROFS))
        {
            // failed asking for read/write mode on open, try read-only
            m_pFile = fopen(spFilenameUTF8, "rbe");
            if (m_pFile)
                m_bReadOnly = true;
        }
    }

    if (!m_pFile)
        return ERROR_UNDEFINED;

    wcscpy(m_cFileName, pName);

    return ERROR_SUCCESS;
}

int CStdLibFileIO::Close()
{
    int nResult = ERROR_UNDEFINED;

    if (m_pFile != APE_NULL)
    {
        nResult = fclose(m_pFile);
        m_pFile = APE_NULL;
    }

    return nResult;
}

int CStdLibFileIO::Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
{
    *pBytesRead = (unsigned int) fread(pBuffer, 1, nBytesToRead, m_pFile);
    if ((*pBytesRead == 0) && (nBytesToRead > 0)) return ERROR_IO_READ;
    else return ferror(m_pFile) ? ERROR_IO_READ : ERROR_SUCCESS;
}

int CStdLibFileIO::Write(const void * pBuffer, unsigned  int nBytesToWrite, unsigned int * pBytesWritten)
{
    *pBytesWritten = (unsigned int) fwrite(pBuffer, 1, nBytesToWrite, m_pFile);
    return (ferror(m_pFile) || (*pBytesWritten != nBytesToWrite)) ? ERROR_IO_WRITE : ERROR_SUCCESS;
}

int CStdLibFileIO::Seek(int64 nPosition, SeekMethod nMethod)
{
    int nOrigin = 0;
    if (nMethod == SeekFileBegin)
        nOrigin = SEEK_SET;
    else if (nMethod == SeekFileEnd)
    {
        nOrigin = SEEK_END;
        nPosition = -abs(nPosition);
    }
    else if (nMethod == SeekFileCurrent)
        nOrigin = SEEK_CUR;
#ifdef PLATFORM_ANDROID
    return fseek(m_pFile, nPosition, nOrigin);
#elif !defined(PLATFORM_WINDOWS)
    return fseeko(m_pFile, nPosition, nOrigin);
#else
    return _fseeki64(m_pFile, nPosition, nOrigin);
#endif
}

int CStdLibFileIO::SetEOF()
{
    return ftruncate(GetHandle(), GetPosition());
}

int64 CStdLibFileIO::GetPosition()
{
#ifdef PLATFORM_ANDROID
    return ftell(m_pFile);
#elif !defined(PLATFORM_WINDOWS)
    return ftello(m_pFile);
#else
    return _ftelli64(m_pFile);
#endif
}

int64 CStdLibFileIO::GetSize()
{
    if (m_bPipe)
        return APE_FILE_SIZE_UNDEFINED;
    int64 nCurrentPosition = GetPosition();
    Seek(0, SeekFileEnd);
    int64 nLength = GetPosition();
    Seek(nCurrentPosition, SeekFileBegin);
    return nLength;
}

int CStdLibFileIO::GetName(wchar_t * pBuffer)
{
    wcscpy(pBuffer, m_cFileName);
    return ERROR_SUCCESS;
}

int CStdLibFileIO::Create(const wchar_t * pName)
{
    Close();

    if (wcslen(pName) >= MAX_PATH)
        return ERROR_UNDEFINED;

    if (0 == wcscmp(pName, L"-") || 0 == wcscmp(pName, L"/dev/stdout"))
    {
        m_pFile = SETBINARY_OUT(stdout);
        m_bReadOnly = false;                            // WriteOnly
    }
    else
    {
        CSmartPtr<char> spFilenameUTF8((char *) CAPECharacterHelper::GetUTF8FromUTF16(pName), true);
        // NOTE: on Mac OSX (BSD Unix), we MUST have "w+b" if we want to read & write, with other systems "wb" seems to be fine
        m_pFile = fopen(spFilenameUTF8, "w+be");                  // Read/Write
        m_bReadOnly = false;
    }

    if (!m_pFile)
        return ERROR_UNDEFINED;

    wcscpy(m_cFileName, pName);

    return ERROR_SUCCESS;
}

int CStdLibFileIO::Delete()
{
    Close();
    CSmartPtr<char> spFilenameUTF8((char *) CAPECharacterHelper::GetUTF8FromUTF16(m_cFileName), true);
    return unlink(spFilenameUTF8);    // 0 success, -1 error
}

}

#endif // #ifdef IO_USE_STD_LIB_FILE_IO
