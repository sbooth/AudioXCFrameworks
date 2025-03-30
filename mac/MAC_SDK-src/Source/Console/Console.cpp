/**************************************************************************************************
MAC Console Frontend (MAC.exe)

Pretty simple and straightforward console front end.
**************************************************************************************************/
#pragma warning(disable: 4710) // function not inlined that happens all the time from fwprintf, etc.
#include "Console.h"
#include <stdio.h>
#include "GlobalFunctions.h"
#include "MACLib.h"
#include "APETag.h"
#include "APEInfo.h"
#include "CharacterHelper.h"
using namespace APE;

// defines
#define COMPRESS_MODE          0
#define DECOMPRESS_MODE        1
#define VERIFY_MODE            2
#define CONVERT_MODE           3
#define TAG_MODE               4
#define VERIFY_FULL_MODE       5
#define REMOVE_TAG_MODE        6
#define CONVERT_TAG_TO_LEGACY  7
#define UNDEFINED_MODE        -1

// global variables
static TICK_COUNT_TYPE g_nInitialTickCount = 0;

/**************************************************************************************************
Gets a parameter from an array
**************************************************************************************************/
static str_utfn * GetParameterFromList(const str_utfn * pList, const str_utfn * pDelimiter, int nCount)
{
    const str_utfn * pHead = pList;
    const str_utfn * pTail = wcsstr(pHead, pDelimiter);
    while (pTail != APE_NULL)
    {
        int nBufferSize = static_cast<int>(pTail - pHead);
        str_utfn * pBuffer = new str_utfn [static_cast<size_t>(nBufferSize) + 1];
        memcpy(pBuffer, pHead, static_cast<size_t>(nBufferSize) * sizeof(pBuffer[0]));
        pBuffer[nBufferSize] = 0;

        pHead = pTail + wcslen(pDelimiter);
        pTail = wcsstr(pHead, pDelimiter);

        nCount--;
        if (nCount < 0)
            return pBuffer;

        delete [] pBuffer;
    }

    if ((*pHead != 0) && (nCount == 0))
    {
        int nBufferSize = static_cast<int>(wcslen(pHead));
        str_utfn * pBuffer = new str_utfn [static_cast<size_t>(nBufferSize) + 1];
        memcpy(pBuffer, pHead, static_cast<size_t>(nBufferSize) * sizeof(pBuffer[0]));
        pBuffer[nBufferSize] = 0;
        return pBuffer;
    }

    return APE_NULL;
}

/**************************************************************************************************
Displays the proper usage for MAC.exe
**************************************************************************************************/
static void DisplayProperUsage(FILE * pFile)
{
    fwprintf(pFile, L"Proper Usage: [EXE] [Input File] [Output File] [Mode]\n\n");

    fwprintf(pFile, L"Modes:\n");
    fwprintf(pFile, L"    Compress (fast): '-c1000'\n");
    fwprintf(pFile, L"    Compress (normal): '-c2000'\n");
    fwprintf(pFile, L"    Compress (high): '-c3000'\n");
    fwprintf(pFile, L"    Compress (extra high): '-c4000'\n");
    fwprintf(pFile, L"    Compress (insane): '-c5000'\n");
    fwprintf(pFile, L"    Decompress: '-d'\n");
    fwprintf(pFile, L"    Verify: '-v'\n");
    fwprintf(pFile, L"    Full Verify: '-V'\n");
    fwprintf(pFile, L"    Convert: '-nXXXX'\n");
    fwprintf(pFile, L"    Tag: '-t'\n");
    fwprintf(pFile, L"    Remove Tag: '-r'\n");
    fwprintf(pFile, L"    Convert to ID3v1 (needed by some players, etc.): '-L'\n\n");

    fwprintf(pFile, L"Options:\n");
    fwprintf(pFile, L"    Set the number of threads when compressing or decompressing: -threads=#'\n\n");

    fwprintf(pFile, L"Examples:\n");
    fwprintf(pFile, L"    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000\n");
    fwprintf(pFile, L"    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000 -threads=16 -t \"Artist=Metallica|Album=Black|Name=One\"\n");
    fwprintf(pFile, L"    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000 -t \"Artist=Metallica|Album=Black|Name=One\"\n");
    fwprintf(pFile, L"    Compress: mac.exe \"Metallica - One.wav\" auto -c2000\n");
    fwprintf(pFile, L"    Transcode from pipe: ffmpeg.exe -i \"Metallica - One.flac\" -f wav - | mac.exe - \"Metallica - One.ape\" -c2000\n");
    fwprintf(pFile, L"    Decompress: mac.exe \"Metallica - One.ape\" \"Metallica - One.wav\" -d\n");
    fwprintf(pFile, L"    Decompress: mac.exe \"Metallica - One.ape\" auto -d\n");
    fwprintf(pFile, L"    Decompress: mac.exe \"Metallica - One.ape\" \"Metallica - One.wav\" -d -threads=16\n");
    fwprintf(pFile, L"    Verify: mac.exe \"Metallica - One.ape\" -v\n");
    fwprintf(pFile, L"    Full Verify: mac.exe \"Metallica - One.ape\" -V\n");
    fwprintf(pFile, L"    Tag: mac.exe \"Metallica - One.ape\" -t \"Artist=Metallica|Album=Black|Name=One|Comment=\\\"This is in quotes\\\"\"\n");
    fwprintf(pFile, L"    Remove tag: mac.exe \"Metallica - One.ape\" -r\n");
}

/**************************************************************************************************
Progress callback
**************************************************************************************************/
static void CALLBACK ProgressCallback(int nPercentageDone)
{
    // get the current tick count
    TICK_COUNT_TYPE nTickCount;
    TICK_COUNT_READ(nTickCount);

    // calculate the progress
    double dProgress = nPercentageDone / 1.e5;                                                    // [0...1]
    double dElapsed = static_cast<double>(nTickCount - g_nInitialTickCount) / TICK_COUNT_FREQ;    // seconds
    double dRemaining = (nPercentageDone > 0) ? dElapsed * ((1.0 / dProgress) - 1.0) : 0;         // seconds

    // output the progress
    fwprintf(stderr, L"Progress: %.1f%% (%.1f seconds remaining, %.1f seconds total)          \r",
        dProgress * 100, dRemaining, dElapsed);

    // don't forget to flush!
    fflush(stderr);

    // update the title
    #ifdef PLATFORM_WINDOWS
        TCHAR cTitle[1024];
        _stprintf_s(cTitle, L"%.1f%% - Monkey's Audio", dProgress * 100);
        SetConsoleTitle(cTitle);
    #endif
}

/**************************************************************************************************
CtrlHandler callback
**************************************************************************************************/
#ifdef PLATFORM_WINDOWS
static BOOL CALLBACK CtrlHandlerCallback(DWORD dwCtrlTyp)
{
    switch (dwCtrlTyp)
    {
    case CTRL_C_EVENT:
        _fputts(_T("\n\nCtrl+C: MAC has been interrupted !!!\n"), stderr);
        break;
    case CTRL_BREAK_EVENT:
        _fputts(_T("\n\nBreak: MAC has been interrupted !!!\n"), stderr);
        break;
    default:
        return false;
    }

    fflush(stderr);
    ExitProcess(666);
}
#endif

/**************************************************************************************************
Tag
**************************************************************************************************/
static int Tag(const str_utfn * pFilename, const str_utfn * pTagString)
{
    int nRetVal = ERROR_UNDEFINED;

    // create the decoder
    int nFunctionRetVal = ERROR_SUCCESS;
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    try
    {
        spAPEDecompress.Assign(CreateIAPEDecompress(pFilename, &nFunctionRetVal, false, true, false));
        if (spAPEDecompress == APE_NULL || nFunctionRetVal != ERROR_SUCCESS) throw(static_cast<intn>(nFunctionRetVal));

        // get the input format
        APE::IAPETag * pTag = APE_NULL;
        pTag = GET_TAG(spAPEDecompress);

        if (pTag == APE_NULL)
            throw(ERROR_UNDEFINED);

        fwprintf(stderr, L"%s\r\n", pTagString);

        // set fields
        for (int nCount = 0; true; nCount++)
        {
            CSmartPtr<str_utfn> spParameter(GetParameterFromList(pTagString, L"|", nCount), true);
            if (spParameter == APE_NULL)
                break;

            str_utfn * pEqual = wcsstr(spParameter, L"=");
            if (pEqual != APE_NULL)
            {
                int nCharacters = static_cast<int>(pEqual - spParameter);
                CSmartPtr<str_utfn> spLeft(new str_utfn [static_cast<size_t>(nCharacters) + 1], true);
                wcsncpy_s(spLeft, static_cast<size_t>(nCharacters) + 1, spParameter, static_cast<size_t>(nCharacters));
                spLeft[nCharacters] = 0;

                nCharacters = static_cast<int>(wcslen(&pEqual[1]));
                CSmartPtr<str_utfn> spRight(new str_utfn [static_cast<size_t>(nCharacters) + 1], true);
                wcscpy_s(spRight, static_cast<size_t>(nCharacters) + 1, &pEqual[1]);
                spRight[nCharacters] = 0;

                fwprintf(stderr, L"%s -> %s\r\n", spLeft.GetPtr(), spRight.GetPtr());

                if (_wcsicmp(spLeft, L"cover") == 0)
                {
                    // open the file
                    APE::CSmartPtr<CIO> spIO(CreateCIO());
                    if (spIO->Open(spRight) == ERROR_SUCCESS)
                    {
                        // get the extension
                        const str_utfn * pExtension = &spRight[wcslen(spRight)];
                        while ((pExtension > spRight) && (*pExtension != '.'))
                            pExtension--;

                        // read the file
                        size_t nBufferBytes = wcslen(pExtension) + 1 + static_cast<size_t>(spIO->GetSize());
                        APE::CSmartPtr<char> spBuffer(new char [nBufferBytes], true);

                        // copy extension (and NULL terminate)
                        for (int z = 0; z < static_cast<int>(wcslen(pExtension)) + 1; z++)
                            spBuffer[z] = static_cast<char>(pExtension[z]);

                        // read data
                        unsigned int nBytesRead = 0;
                        spIO->Read(&spBuffer[wcslen(pExtension) + 1], static_cast<unsigned int>(spIO->GetSize()), &nBytesRead);

                        // set
                        pTag->SetFieldBinary(APE_TAG_FIELD_COVER_ART_FRONT, spBuffer, static_cast<intn>(nBufferBytes), TAG_FIELD_FLAG_DATA_TYPE_BINARY);
                    }

                }
                else
                {
                    pTag->SetFieldString(spLeft, spRight);
                }
            }
        }

        // save
        if (pTag->Save(false) == ERROR_SUCCESS)
        {
            nRetVal = ERROR_SUCCESS;
        }
    }
    catch (...)
    {
        nRetVal = ERROR_UNDEFINED;
    }

    return nRetVal;
}

/**************************************************************************************************
Remove tags
**************************************************************************************************/
static int RemoveTags(const str_utfn * pFilename)
{
    int nRetVal = ERROR_UNDEFINED;

    // create the decoder
    int nFunctionRetVal = ERROR_SUCCESS;
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    try
    {
        spAPEDecompress.Assign(CreateIAPEDecompress(pFilename, &nFunctionRetVal, false, true, false));
        if (spAPEDecompress == APE_NULL || nFunctionRetVal != ERROR_SUCCESS) throw(static_cast<intn>(nFunctionRetVal));

        // get the input format
        APE::IAPETag * pTag = GET_TAG(spAPEDecompress);
        if (pTag == APE_NULL)
            throw(ERROR_UNDEFINED);

        // remove the tag
        if (pTag->Remove(true) == ERROR_SUCCESS)
        {
            nRetVal = ERROR_SUCCESS;
        }
    }
    catch (...)
    {
        nRetVal = ERROR_UNDEFINED;
    }

    return nRetVal;
}

/**************************************************************************************************
Convert to legacy
**************************************************************************************************/
static int ConvertTagsToLegacy(const str_utfn * pFilename)
{
    int nRetVal = ERROR_UNDEFINED;

    // create the decoder
    int nFunctionRetVal = ERROR_SUCCESS;
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    try
    {
        spAPEDecompress.Assign(CreateIAPEDecompress(pFilename, &nFunctionRetVal, false, true, false));
        if (spAPEDecompress == APE_NULL || nFunctionRetVal != ERROR_SUCCESS) throw(static_cast<intn>(nFunctionRetVal));

        // get the input format
        APE::IAPETag * pTag = GET_TAG(spAPEDecompress);
        if (pTag == APE_NULL)
            throw(ERROR_UNDEFINED);

        // remove the tag
        if (pTag->Save(true) == ERROR_SUCCESS)
        {
            nRetVal = ERROR_SUCCESS;
        }
    }
    catch (...)
    {
        nRetVal = ERROR_UNDEFINED;
    }

    return nRetVal;
}

/**************************************************************************************************
Handle auto for an output filename
**************************************************************************************************/
static void HandleAuto(CSmartPtr<wchar_t> & rspInputFilename, CSmartPtr<wchar_t> & rspOutputFilename, bool bAPEType)
{
    // build the output filename
    const wchar_t * AUTO = L"auto";
    if ((rspOutputFilename == NULL) || (wcslen(rspOutputFilename) == 0) || (_wcsicmp(rspOutputFilename, AUTO) == 0))
    {
        // build the output filename
        rspOutputFilename.Assign(new wchar_t[APE_MAX_PATH], true);
        wcscpy_s(rspOutputFilename, APE_MAX_PATH, rspInputFilename);

        // remove the extension
        wchar_t* pExtension = wcsrchr(rspOutputFilename, '.');
        if (pExtension != APE_NULL)
            *pExtension = 0; // switch to NULL so we don't have an extension

        // put together and fill spOutputFilename
        if (bAPEType)
        {
            // add the .ape extension
            wcscat_s(rspOutputFilename, APE_MAX_PATH, L".ape");
        }
        else
        {
            // add the new extension
            APE::str_ansi cFileType[8] = { 0 };
            GetAPEFileType(rspInputFilename, cFileType);
            CSmartPtr<APE::str_utfn> spFileTypeUTF16(CAPECharacterHelper::GetUTF16FromANSI(cFileType), true);

            // put together and fill spOutputFilename
            wcscat_s(rspOutputFilename, APE_MAX_PATH, spFileTypeUTF16);
        }
    }
}

/**************************************************************************************************
Get an argument
**************************************************************************************************/
#ifndef PLATFORM_WINDOWS
static void GetArgument(char * argv[], int nArgument, APE::CSmartPtr<wchar_t> & rspString)
#else
static void GetArgument(TCHAR * argv[], int nArgument, APE::CSmartPtr<wchar_t> & rspString)
#endif
{
    #ifdef PLATFORM_WINDOWS
        #ifdef _UNICODE
            rspString.Assign(argv[nArgument], true, false);
        #else
            rspString.Assign(CAPECharacterHelper::GetUTF16FromANSI(argv[nArgument]), true);
        #endif
    #else
        rspString.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[nArgument]), true);
    #endif
}

/**************************************************************************************************
Parse the number of threads
**************************************************************************************************/
#ifndef PLATFORM_WINDOWS
static bool GetThreads(char * argv[], int nArgument, int & rnThreads)
#else
static bool GetThreads(TCHAR * argv[], int nArgument, int & rnThreads)
#endif
{
    // get the threads argument
    CSmartPtr<wchar_t> spThreads;
    GetArgument(argv, nArgument, spThreads);

    bool bFound = false;
    if (spThreads && _wcsnicmp(spThreads, L"-threads=", 9) == 0)
    {
        rnThreads = _wtoi(&spThreads[9]);
        bFound = true;
    }

    return bFound;
}

/**************************************************************************************************
Main (the main function)
**************************************************************************************************/
#ifndef PLATFORM_WINDOWS
int main(int argc, char * argv[])
#else
int _tmain(int argc, TCHAR * argv[])
#endif
{
    // variable declares
    CSmartPtr<wchar_t> spInputFilename; CSmartPtr<wchar_t> spOutputFilename;
    int nRetVal = ERROR_UNDEFINED;
    int nMode = UNDEFINED_MODE;
    int nCompressionLevel = 0;
    int nPercentageDone;

    // initialize
    #ifdef PLATFORM_WINDOWS
        SetErrorMode(SetErrorMode(0x0003) | 0x0003);
        SetConsoleCtrlHandler(CtrlHandlerCallback, true);
    #endif

    // output the header
    fwprintf(stderr, CONSOLE_NAME);

    // make sure there are at least four arguments (could be more for EAC compatibility)
    if (argc < 3)
    {
        DisplayProperUsage(stderr);
        exit(-1);
    }

    // store the filenames
    GetArgument(argv, 1, spInputFilename);
    GetArgument(argv, 2, spOutputFilename);

    // verify that the input file exists
    if (!FileExists(spInputFilename))
    {
        fwprintf(stderr, L"Input File Not Found...\n\n");
        exit(-1);
    }

    // if the output file equals '-v', then use this as the next argument
    CSmartPtr<wchar_t> spMode;
    GetArgument(argv, 2, spMode);

    wchar_t cMode[3];
    APE_CLEAR(cMode);
    cMode[0] = spMode[0];
    cMode[1] = (spMode[0] == 0) ? wchar_t(0) : spMode[1];

    if ((_wcsicmp(cMode, L"-v") != 0) &&
        (_wcsicmp(cMode, L"-t") != 0) &&
        (_wcsicmp(cMode, L"-r") != 0) &&
        (_wcsicmp(cMode, L"-L") != 0))
    {
        // verify, tag, and remove tag don't use at least the third argument
        if (argc < 4)
        {
            DisplayProperUsage(stderr);
            exit(-1);
        }

        // check for and skip if necessary the -b XXXXXX arguments (3,4)
        GetArgument(argv, 3, spMode);
        cMode[0] = spMode[0];
        cMode[1] = (spMode[0] == 0) ? wchar_t(0) : spMode[1];
    }

    // get the mode
    nMode = UNDEFINED_MODE;
    if (_wcsicmp(cMode, L"-c") == 0)
        nMode = COMPRESS_MODE;
    else if (_wcsicmp(cMode, L"-d") == 0)
        nMode = DECOMPRESS_MODE;
    else if (wcscmp(cMode, L"-V") == 0)
        nMode = VERIFY_FULL_MODE;
    else if (wcscmp(cMode, L"-v") == 0)
        nMode = VERIFY_MODE;
    else if (_wcsicmp(cMode, L"-n") == 0)
        nMode = CONVERT_MODE;
    else if (_wcsicmp(cMode, L"-r") == 0)
        nMode = REMOVE_TAG_MODE;
    else if (_wcsicmp(cMode, L"-t") == 0)
        nMode = TAG_MODE;
    else if (_wcsicmp(cMode, L"-L") == 0)
        nMode = CONVERT_TAG_TO_LEGACY;

    // error check the mode
    if (nMode == UNDEFINED_MODE)
    {
        DisplayProperUsage(stderr);
        exit(-1);
    }

    // get threads
    int nThreads = 1;
    int nTagArgument = 5;
    if ((nMode == COMPRESS_MODE) && (argc >= 5))
    {
        // get the threads argument
        if (GetThreads(argv, 4, nThreads))
            nTagArgument = 6;
    }
    else if ((nMode == DECOMPRESS_MODE) && (argc >= 4))
    {
        // get the threads argument
        GetThreads(argv, 4, nThreads);
    }
    else if (((nMode == VERIFY_FULL_MODE) || (nMode == VERIFY_MODE)) && (argc >= 3))
    {
        // get the threads argument
        GetThreads(argv, 3, nThreads);
    }

    // get and error check the compression level
    if (nMode == COMPRESS_MODE || nMode == CONVERT_MODE)
    {
        nCompressionLevel = _wtoi(&spMode[2]);
        if (nCompressionLevel != 1000 && nCompressionLevel != 2000 &&
            nCompressionLevel != 3000 && nCompressionLevel != 4000 &&
            nCompressionLevel != 5000)
        {
            DisplayProperUsage(stderr);
            return ERROR_UNDEFINED;
        }
    }

    // set the initial tick count
    TICK_COUNT_READ(g_nInitialTickCount);

    // process
    int nKillFlag = 0;
#ifdef APE_SUPPORT_COMPRESS
    if (nMode == COMPRESS_MODE)
    {
        str_utfn cCompressionLevel[16]; APE_CLEAR(cCompressionLevel);
        GetAPECompressionLevelName(nCompressionLevel, cCompressionLevel, 16, false);

        fwprintf(stderr, L"Compressing (%ls)...\n", cCompressionLevel);

        // build the output filename
        HandleAuto(spInputFilename, spOutputFilename, true);

        // compress
        nRetVal = CompressFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag, nThreads);

        // tag
        #ifdef PLATFORM_WINDOWS
        if ((nRetVal == ERROR_SUCCESS) && (argc > nTagArgument) && (_tcsicmp(argv[nTagArgument - 1], _T("-t")) == 0))
        #else
        if ((nRetVal == ERROR_SUCCESS) && (argc > nTagArgument) && (strcasecmp(argv[nTagArgument - 1], "-t") == 0))
        #endif
        {
            fwprintf(stderr, L"\nTagging...\n");

            CSmartPtr<wchar_t> spTag;
            GetArgument(argv, nTagArgument, spTag);

            nRetVal = Tag(spOutputFilename, spTag);
        }
    }
    else
#endif
    if (nMode == DECOMPRESS_MODE)
    {
        fwprintf(stderr, L"Decompressing...\n");

        // build the output filename
        HandleAuto(spInputFilename, spOutputFilename, false);

        // decompress
        nRetVal = DecompressFileW(spInputFilename, spOutputFilename, &nPercentageDone, ProgressCallback, &nKillFlag, nThreads);
    }
    else if (nMode == VERIFY_MODE)
    {
        fwprintf(stderr, L"Verifying...\n");
        nRetVal = VerifyFileW(spInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag, true, nThreads);
    }
    else if (nMode == VERIFY_FULL_MODE)
    {
        fwprintf(stderr, L"Full verifying...\n");
        nRetVal = VerifyFileW(spInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag, false, nThreads);
    }
    else if (nMode == CONVERT_MODE)
    {
        fwprintf(stderr, L"Converting...\n");
        nRetVal = ConvertFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag, nThreads);
    }
    else if (nMode == TAG_MODE)
    {
        fwprintf(stderr, L"Tagging...\n");
        CSmartPtr<wchar_t> spTag;
        GetArgument(argv, 3, spTag);
        nRetVal = Tag(spInputFilename, spTag);
    }
    else if (nMode == REMOVE_TAG_MODE)
    {
        fwprintf(stderr, L"Removing tags...\n");
        nRetVal = RemoveTags(spInputFilename);
    }
    else if (nMode == CONVERT_TAG_TO_LEGACY)
    {
        fwprintf(stderr, L"Converting tag to ID3v1...\n");
        nRetVal = ConvertTagsToLegacy(spInputFilename);
    }

    if (nRetVal == ERROR_SUCCESS)
        fwprintf(stderr, L"\nSuccess\n");
    else
        fwprintf(stderr, L"\nError: %i\n", nRetVal);

    return nRetVal;
}
