/**************************************************************************************************
MAC Console Frontend (MAC.exe)

Pretty simple and straightforward console front end.  If somebody ever wants to add 
more functionality like tagging, auto-verify, etc., that'd be excellent.

Copyrighted (c) 2000 - 2022 Matthew T. Ashland.  All Rights Reserved.
**************************************************************************************************/
#include "All.h"
#include <stdio.h>
#include "GlobalFunctions.h"
#include "MACLib.h"
#include "APETag.h"
#include "CharacterHelper.h"
using namespace APE;

// defines
#define COMPRESS_MODE          0
#define DECOMPRESS_MODE        1
#define VERIFY_MODE            2
#define CONVERT_MODE           3
#define TAG_MODE               4
#define VERIFY_FULL_MODE       5
#define UNDEFINED_MODE        -1

// global variables
TICK_COUNT_TYPE g_nInitialTickCount = 0;

/**************************************************************************************************
Gets a parameter from an array
**************************************************************************************************/
TCHAR * GetParameterFromList(LPCTSTR pList, LPCTSTR pDelimiter, int nCount)
{
    LPCTSTR pHead = pList;
    LPCTSTR pTail = _tcsstr(pHead, pDelimiter);
    while (pTail != NULL)
    {
        int nBufferSize = int(pTail - pHead);
        TCHAR * pBuffer = new TCHAR [nBufferSize + 1];
        memcpy(pBuffer, pHead, nBufferSize * sizeof(TCHAR));
        pBuffer[nBufferSize] = 0;

        pHead = pTail + _tcslen(pDelimiter);
        pTail = _tcsstr(pHead, pDelimiter);

        nCount--;
        if (nCount < 0)
        {
            return pBuffer;
        }

        delete [] pBuffer;
    }

    if ((*pHead != 0) && (nCount == 0))
    {
        int nBufferSize = (int) _tcslen(pHead);
        TCHAR * pBuffer = new TCHAR [nBufferSize + 1];
        memcpy(pBuffer, pHead, nBufferSize * sizeof(TCHAR));
        pBuffer[nBufferSize] = 0;
        return pBuffer;
    }

    return NULL;
}

/**************************************************************************************************
Displays the proper usage for MAC.exe
**************************************************************************************************/
static void DisplayProperUsage(FILE * pFile)
{
    _ftprintf(pFile, _T("Proper Usage: [EXE] [Input File] [Output File] [Mode]\n\n"));

    _ftprintf(pFile, _T("Modes: \n"));
    _ftprintf(pFile, _T("    Compress (fast): '-c1000'\n"));
    _ftprintf(pFile, _T("    Compress (normal): '-c2000'\n"));
    _ftprintf(pFile, _T("    Compress (high): '-c3000'\n"));
    _ftprintf(pFile, _T("    Compress (extra high): '-c4000'\n"));
    _ftprintf(pFile, _T("    Compress (insane): '-c5000'\n"));
    _ftprintf(pFile, _T("    Decompress: '-d'\n"));
    _ftprintf(pFile, _T("    Verify: '-v'\n"));
    _ftprintf(pFile, _T("    Full Verify: '-V'\n"));
    _ftprintf(pFile, _T("    Convert: '-nXXXX'\n"));
    _ftprintf(pFile, _T("    Tag: '-t'\n\n"));

    _ftprintf(pFile, _T("Examples:\n"));
    _ftprintf(pFile, _T("    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000\n"));
    _ftprintf(pFile, _T("    Compress: mac.exe \"Metallica - One.wav\" \"Metallica - One.ape\" -c2000 -t \"Artist=Metallica|Album=Black|Name=One\"\n"));
    _ftprintf(pFile, _T("    Decompress: mac.exe \"Metallica - One.ape\" \"Metallica - One.wav\" -d\n"));
    _ftprintf(pFile, _T("    Decompress: mac.exe \"Metallica - One.ape\" auto -d\n"));
    _ftprintf(pFile, _T("    Verify: mac.exe \"Metallica - One.ape\" -v\n"));
    _ftprintf(pFile, _T("    Full Verify: mac.exe \"Metallica - One.ape\" -V\n"));
    _ftprintf(pFile, _T("    Tag: mac.exe \"Metallica - One.ape\" -t \"Artist=Metallica|Album=Black|Name=One|Comment=\\\"This is in quotes\\\"\"\n"));
    _ftprintf(pFile, _T("    (note: int filenames must be put inside of quotations)\n"));
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
    double dProgress = nPercentageDone / 1.e5;                                          // [0...1]
    double dElapsed = (double) (nTickCount - g_nInitialTickCount) / TICK_COUNT_FREQ;    // seconds
    double dRemaining = dElapsed * ((1.0 / dProgress) - 1.0);                           // seconds

    // output the progress
    _ftprintf(stderr, _T("Progress: %.1f%% (%.1f seconds remaining, %.1f seconds total)          \r"), 
        dProgress * 100, dRemaining, dElapsed);

    // don't forget to flush!
    fflush(stderr);
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
        return FALSE;
    }

    fflush(stderr);
    ExitProcess(666);
}
#endif

int Tag(const TCHAR * pFilename, const TCHAR * pTagString)
{
    int nRetVal = ERROR_UNDEFINED;

    // create the decoder
    int nFunctionRetVal = ERROR_SUCCESS;
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    try
    {
        spAPEDecompress.Assign(CreateIAPEDecompress(pFilename, &nFunctionRetVal, false, true, false));
        if (spAPEDecompress == NULL || nFunctionRetVal != ERROR_SUCCESS) throw(intn(nFunctionRetVal));

        // get the input format
        APE::CAPETag * pTag = NULL;
        pTag = (APE::CAPETag *) spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TAG);
        if (pTag == NULL)
            throw(ERROR_UNDEFINED);

        _ftprintf(stderr, pTagString);
        _ftprintf(stderr, _T("\r\n"));

        // set fields
        for (int nCount = 0; true; nCount++)
        {
            TCHAR * pParameter = GetParameterFromList(pTagString, _T("|"), nCount);
            if (pParameter == NULL)
                break;

            TCHAR * pEqual = _tcsstr(pParameter, _T("="));
            if (pEqual != NULL)
            {
                int nCharacters = int(pEqual - pParameter);
                CSmartPtr<TCHAR> spLeft(new TCHAR[nCharacters + 1], true);
                _tcsncpy_s(spLeft, nCharacters + 1, pParameter, nCharacters);
                spLeft[nCharacters] = 0;

                nCharacters = (int) _tcslen(&pEqual[1]);
                CSmartPtr<TCHAR> spRight(new TCHAR[nCharacters + 1], true);
                _tcscpy_s(spRight, nCharacters + 1, &pEqual[1]);
                spRight[nCharacters] = 0;

                _ftprintf(stderr, spLeft);
                _ftprintf(stderr, _T(" -> "));
                _ftprintf(stderr, spRight);
                _ftprintf(stderr, _T("\r\n"));

                pTag->SetFieldString(spLeft, spRight);
            }

            delete[] pParameter;
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
    const wchar_t * AUTO; const wchar_t * WAV;
    
    // initialize
    #ifdef PLATFORM_WINDOWS
        SetErrorMode(SetErrorMode(0x0003) | 0x0003);
        SetConsoleCtrlHandler(CtrlHandlerCallback, TRUE);
    #endif

    // output the header
    _ftprintf(stderr, CONSOLE_NAME);
    
    // make sure there are at least four arguments (could be more for EAC compatibility)
    if (argc < 3) 
    {
        DisplayProperUsage(stderr);
        exit(-1);
    }

    // store the constants
    #ifdef PLATFORM_WINDOWS
        #ifdef _UNICODE
            AUTO = _T("auto");
            WAV = _T(".wav");
        #else
            AUTO = CAPECharacterHelper::GetUTF16FromANSI("auto");
            WAV = CAPECharacterHelper::GetUTF16FromANSI(".wav");
        #endif
    #else
        AUTO = CAPECharacterHelper::GetUTF16FromUTF8((str_utf8*) "auto");
        WAV = CAPECharacterHelper::GetUTF16FromUTF8((str_utf8*) ".wav");
    #endif

    // store the filenames
    #ifdef PLATFORM_WINDOWS
        #ifdef _UNICODE
            spInputFilename.Assign(argv[1], TRUE, FALSE);
            spOutputFilename.Assign(argv[2], TRUE, FALSE);
        #else
            spInputFilename.Assign(CAPECharacterHelper::GetUTF16FromANSI(argv[1]), TRUE);
            spOutputFilename.Assign(CAPECharacterHelper::GetUTF16FromANSI(argv[2]), TRUE);
        #endif
    #else
        spInputFilename.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[1]), TRUE);
        spOutputFilename.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[2]), TRUE);
    #endif

    // verify that the input file exists
    if (!FileExists(spInputFilename))
    {
        _ftprintf(stderr, _T("Input File Not Found...\n\n"));
        exit(-1);
    }

    // if the output file equals '-v', then use this as the next argument
    CSmartPtr<wchar_t> spMode;
    #ifdef PLATFORM_WINDOWS
        #ifdef _UNICODE
            spMode.Assign(argv[2], TRUE, FALSE);
        #else
            spMode.Assign(CAPECharacterHelper::GetUTF16FromANSI(argv[2]), TRUE);
        #endif
    #else
        spMode.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[2]), TRUE);
    #endif

    wchar_t cMode[3] = { 0 };
    cMode[0] = spMode[0];
    cMode[1] = (spMode[0] == 0) ? 0 : spMode[1];

    if ((_tcsicmp(cMode, _T("-v")) != 0) &&
        (_tcsicmp(cMode, _T("-q")) != 0) &&
        (_tcsicmp(cMode, _T("-t")) != 0))
    {
        // verify is the only mode that doesn't use at least the third argument
        if (argc < 4) 
        {
            DisplayProperUsage(stderr);
            exit(-1);
        }

        // check for and skip if necessary the -b XXXXXX arguments (3,4)
        spMode.Assign(new wchar_t [256], true, true);
        #ifdef PLATFORM_WINDOWS
            _tcsncpy_s(spMode, 256, argv[3], 255);
        #else
            spMode.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[3]), TRUE);
        #endif
        cMode[0] = spMode[0];
        cMode[1] = (spMode[0] == 0) ? 0 : spMode[1];
    }

    // get the mode
    nMode = UNDEFINED_MODE;
    if (_tcsicmp(cMode, _T("-c")) == 0)
        nMode = COMPRESS_MODE;
    else if (_tcsicmp(cMode, _T("-d")) == 0)
        nMode = DECOMPRESS_MODE;
    else if (_tcscmp(cMode, _T("-V")) == 0)
        nMode = VERIFY_FULL_MODE;
    else if (_tcscmp(cMode, _T("-v")) == 0)
        nMode = VERIFY_MODE;
    else if (_tcsicmp(cMode, _T("-n")) == 0)
        nMode = CONVERT_MODE;
    else if (_tcsicmp(cMode, _T("-t")) == 0)
        nMode = TAG_MODE;

    // error check the mode
    if (nMode == UNDEFINED_MODE) 
    {
        DisplayProperUsage(stderr);
        exit(-1);
    }

    // get and error check the compression level
    if (nMode == COMPRESS_MODE || nMode == CONVERT_MODE) 
    {
        nCompressionLevel = _ttoi(&spMode[2]);
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
        TCHAR cCompressionLevel[16];
        if (nCompressionLevel == 1000) { _tcscpy_s(cCompressionLevel, 16, _T("fast")); }
        if (nCompressionLevel == 2000) { _tcscpy_s(cCompressionLevel, 16, _T("normal")); }
        if (nCompressionLevel == 3000) { _tcscpy_s(cCompressionLevel, 16, _T("high")); }
        if (nCompressionLevel == 4000) { _tcscpy_s(cCompressionLevel, 16, _T("extra high")); }
        if (nCompressionLevel == 5000) { _tcscpy_s(cCompressionLevel, 16, _T("insane")); }

        _ftprintf(stderr, _T("Compressing (%s)...\n"), cCompressionLevel);
        nRetVal = CompressFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);

        #ifdef PLATFORM_WINDOWS
        if ((nRetVal == ERROR_SUCCESS) && (argc > 5) && (_tcsicmp(argv[4], _T("-t")) == 0))
        #else
        if ((nRetVal == ERROR_SUCCESS) && (argc > 5) && (strcasecmp(argv[4], "-t") == 0))
        #endif
        {
            _ftprintf(stderr, _T("\nTagging...\n"));
            #ifdef PLATFORM_WINDOWS
                nRetVal = Tag(spOutputFilename, argv[5]);
            #else
                CSmartPtr<wchar_t> spTag;
                spTag.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[5]), TRUE);
                nRetVal = Tag(spOutputFilename, spTag);
            #endif
        }
    }
    else if (nMode == DECOMPRESS_MODE) 
#else
    if (nMode == DECOMPRESS_MODE)
#endif
    {
        _ftprintf(stderr, _T("Decompressing...\n"));
        if (_tcsicmp(spOutputFilename, AUTO) == 0)
        {
            wchar_t cOutput[MAX_PATH];
            wchar_t * pExtension = wcschr(spInputFilename, '.');
            if (pExtension != NULL)
                *pExtension = 0;
            wcscpy_s(cOutput, MAX_PATH, spInputFilename);
            if (pExtension != NULL)
                *pExtension = '.';
            wcscat_s(cOutput, MAX_PATH, WAV);
            spOutputFilename.Assign(new wchar_t[wcslen(cOutput) + 1], true);
            wcscpy_s(spOutputFilename, wcslen(cOutput) + 1, cOutput);
        }
        APE::str_ansi cFileType[5] = { 0 };
        nRetVal = DecompressFileW(spInputFilename, spOutputFilename, &nPercentageDone, ProgressCallback, &nKillFlag, cFileType);

        // rename the file if we output a file type
        if (cFileType[0] != 0)
        {
            CSmartPtr<wchar_t> spFileTypeUTF16;
            spFileTypeUTF16.Assign(CAPECharacterHelper::GetUTF16FromANSI(cFileType), true);

            wchar_t cOutputNew[MAX_PATH];
            wcscpy_s(cOutputNew, MAX_PATH, spOutputFilename);
            wcscpy_s(&cOutputNew[_tcslen(cOutputNew) - 3], MAX_PATH - _tcslen(cOutputNew) - 3, spFileTypeUTF16);
            #ifdef PLATFORM_WINDOWS
                _wrename(spOutputFilename.GetPtr(), cOutputNew);
            #else
                CSmartPtr<APE::str_utf8> spOld; spOld.Assign(CAPECharacterHelper::GetUTF8FromUTF16(spOutputFilename), true);
                CSmartPtr<APE::str_utf8> spNew; spNew.Assign(CAPECharacterHelper::GetUTF8FromUTF16(cOutputNew), true);
                rename((const char *) spOld.GetPtr(), (const char *) spNew.GetPtr());
            #endif
        }
    }    
    else if (nMode == VERIFY_MODE) 
    {
        _ftprintf(stderr, _T("Verifying...\n"));
        nRetVal = VerifyFileW(spInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag, true);
    }    
    else if (nMode == VERIFY_FULL_MODE)
    {
        _ftprintf(stderr, _T("Full verifying...\n"));
        nRetVal = VerifyFileW(spInputFilename, &nPercentageDone, ProgressCallback, &nKillFlag, false);
    }
    else if (nMode == CONVERT_MODE)
    {
        _ftprintf(stderr, _T("Converting...\n"));
        nRetVal = ConvertFileW(spInputFilename, spOutputFilename, nCompressionLevel, &nPercentageDone, ProgressCallback, &nKillFlag);
    }
    else if (nMode == TAG_MODE)
    {
        _ftprintf(stderr, _T("Tagging...\n"));
        #ifdef PLATFORM_WINDOWS
            nRetVal = Tag(spInputFilename, argv[3]);
        #else
            CSmartPtr<wchar_t> spTag;
            spTag.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) argv[3]), TRUE);
            nRetVal = Tag(spInputFilename, spTag);
        #endif
    }

    if (nRetVal == ERROR_SUCCESS)
        _ftprintf(stderr, _T("\nSuccess...\n"));
    else
        _ftprintf(stderr, _T("\nError: %i\n"), nRetVal);

    return nRetVal;
}
