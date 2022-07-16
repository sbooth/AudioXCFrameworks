#include "All.h"
#include "GlobalFunctions.h"
#include "IO.h"
#include "CharacterHelper.h"
#ifdef _MSC_VER
    #include <intrin.h>
#endif
#ifdef PLATFORM_ANDROID
    #include <android/api-level.h>
#endif

namespace APE
{

int ReadSafe(CIO * pIO, void * pBuffer, int nBytes)
{
    unsigned int nBytesRead = 0;
    int nResult = pIO->Read(pBuffer, nBytes, &nBytesRead);
    if (nResult == ERROR_SUCCESS)
    {
        if (nBytes != int(nBytesRead))
            nResult = ERROR_IO_READ;
    }

    return nResult;
}

intn WriteSafe(CIO * pIO, void * pBuffer, intn nBytes)
{
    unsigned int nBytesWritten = 0;
    intn nResult = pIO->Write(pBuffer, (unsigned int) nBytes, &nBytesWritten);
    if (nResult == ERROR_SUCCESS)
    {
        if (nBytes != int(nBytesWritten))
            nResult = ERROR_IO_WRITE;
    }

    return nResult;
}

bool FileExists(wchar_t * pFilename)
{    
    if (0 == wcscmp(pFilename, L"-")  ||  0 == wcscmp(pFilename, L"/dev/stdin"))
        return true;

#ifdef _WIN32

    bool bFound = false;

    WIN32_FIND_DATA WFD;
    HANDLE hFind = FindFirstFile(pFilename, &WFD);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        bFound = true;
        FindClose(hFind);
    }

    return bFound;

#else

    CSmartPtr<char> spFilenameUTF8((char *) CAPECharacterHelper::GetUTF8FromUTF16(pFilename), true);

    struct stat b;

    if (stat(spFilenameUTF8, &b) != 0)
        return false;

    if (!S_ISREG(b.st_mode))
        return false;

    return true;

#endif
}

void * AllocateAligned(intn nBytes, intn nAlignment)
{
#ifdef _WIN32
    return _aligned_malloc(nBytes, nAlignment);
#elif defined(__APPLE__)
    return malloc(nBytes);
#elif defined(PLATFORM_ANDROID) && (__ANDROID_API__ < 21)
    return memalign(nAlignment, nBytes);
#else
    void * pMemory = NULL;
    if (posix_memalign(&pMemory, nAlignment, nBytes))
        return NULL;
    return pMemory;
#endif
}

void FreeAligned(void * pMemory)
{
#ifdef _WIN32
    _aligned_free(pMemory);
#else
    free(pMemory);
#endif
}

bool GetSSEAvailable(bool bTestForSSE41)
{
#if defined(__SSE41__)
    return true;
#elif defined(__SSE2__)
    return !bTestForSSE41;
#else
    bool bSSE = false;
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #define CPU_SSE2 (1 << 26)
    #define CPU_SSE41 (1 << 19)

    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);

    int nIds = cpuInfo[0];
    if (nIds >= 1)
    {
        __cpuid(cpuInfo, 1);
        if (bTestForSSE41)
        {
            if (cpuInfo[3] & CPU_SSE41)
                bSSE = true;
        }
        else
        {
            if (cpuInfo[3] & CPU_SSE2)
                bSSE = true;
        }
    }
#endif
    return bSSE;
#endif
}

bool GetAVX2Available()
{
#if defined(__AVX2__)
    return true;
#else
    bool bAVX = false;
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #define CPU_AVX2 (1 << 5)

    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);

    int nIds = cpuInfo[0];
    if (nIds >= 7)
    {
        __cpuid(cpuInfo, 7);
        if (cpuInfo[1] & CPU_AVX2)
            bAVX = true;
    }
#endif
    return bAVX;
#endif
}

bool StringIsEqual(const str_utfn * pString1, const str_utfn * pString2, bool bCaseSensitive, int nCharacters)
{
    // wcscasecmp isn't available on OSX 10.6 and sometimes it's hard to find other string comparisons that work reliably on different
    // platforms, so we'll just roll our own simple version here (performance of this function isn't critical to APE performance)

    // default to 'true' so that comparing two empty strings will be a match
    bool bResult = true;

    // if -1 is passed in, compare the entire string
    if (nCharacters == -1)
        nCharacters = 0x7FFFFFFF;

    if (nCharacters > 0)
    {
        // walk string
        str_utfn f, l;
        do
        {
            f = *pString1++;
            l = *pString2++;
            if (!bCaseSensitive)
            {
                f = _totlower(f);
                l = _totlower(l);
            }
        }
        while ((--nCharacters) && (f != 0) && (f == l));

        // if we're still equal, it's a match
        bResult = (f == l);
    }

    return bResult;
}

}
