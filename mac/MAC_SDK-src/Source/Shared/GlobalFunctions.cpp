#include "All.h"
#include "GlobalFunctions.h"
#include "IO.h"
#include "CharacterHelper.h"

#ifdef _MSC_VER
    #include <intrin.h>
#endif

#ifdef PLATFORM_APPLE
    #include <AvailabilityMacros.h>
#endif

#ifdef PLATFORM_ANDROID
    #include <android/api-level.h>
#endif

namespace APE
{

int ReadSafe(CIO * pIO, void * pBuffer, int nBytes)
{
    unsigned int nBytesRead = 0;
    int nResult = pIO->Read(pBuffer, static_cast<unsigned int>(nBytes), &nBytesRead);
    if (nResult == ERROR_SUCCESS)
    {
        if (nBytes != static_cast<int>(nBytesRead))
            nResult = ERROR_IO_READ;
    }

    return nResult;
}

intn WriteSafe(CIO * pIO, void * pBuffer, intn nBytes)
{
    unsigned int nBytesWritten = 0;
    intn nResult = pIO->Write(pBuffer, static_cast<unsigned int>(nBytes), &nBytesWritten);
    if (nResult == ERROR_SUCCESS)
    {
        if (nBytes != static_cast<int>(nBytesWritten))
            nResult = ERROR_IO_WRITE;
    }

    return nResult;
}

bool FileExists(const wchar_t * pFilename)
{
    if (pFilename == APE_NULL)
        return false;
    if (0 == wcscmp(pFilename, L"-")  ||  0 == wcscmp(pFilename, L"/dev/stdin"))
        return true;

#ifdef _WIN32

    bool bFound = false;

    WIN32_FIND_DATA WFD;
#ifdef UNICODE
    HANDLE hFind = FindFirstFile(pFilename, &WFD);
#else
    HANDLE hFind = FindFirstFile(CAPECharacterHelper::GetANSIFromUTF16(pFilename), &WFD);
#endif
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
#if defined(PLATFORM_WINDOWS)
    return _aligned_malloc(static_cast<size_t>(nBytes), static_cast<size_t>(nAlignment));
#elif defined(PLATFORM_APPLE) && (MAC_OS_X_VERSION_MIN_REQUIRED < 1060)
    if (nAlignment <= 16)
        return malloc(nBytes);
    return valloc(nBytes);
#elif defined(PLATFORM_ANDROID) && (__ANDROID_API__ < 21)
    return memalign(nAlignment, nBytes);
#else
    void * pMemory = APE_NULL;
    if (posix_memalign(&pMemory, nAlignment, nBytes))
        return APE_NULL;
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
                f = towlower(f);
                l = towlower(l);
            }
        }
        while ((--nCharacters) && (f != 0) && (f == l));

        // if we're still equal, it's a match
        bResult = (f == l);
    }

    return bResult;
}

}
