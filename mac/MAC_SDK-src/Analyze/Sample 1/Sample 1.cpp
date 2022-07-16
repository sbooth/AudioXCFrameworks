/***************************************************************************************
Analyze - Sample 1
Copyright (C) 2000-2022 by Matthew T. Ashland   All Rights Reserved.
Feel free to use this code in any way that you like.

This example opens an APE file and displays some basic information about it. To use it,
just type Sample 1.exe followed by a file name and it'll display information about that
file.

Notes for use in a new project:
    -you need to include "MACLib.lib" in the included libraries list
    -life will be easier if you set the [MAC SDK]\\Shared directory as an include 
    directory and an additional library input path in the project settings
    -set the runtime library to "Mutlithreaded"

WARNING:
    -This class driven system for using Monkey's Audio is still in development, so
    I can't make any guarantees that the classes and libraries won't change before
    everything gets finalized.  Use them at your own risk.
***************************************************************************************/

// includes
#include "All.h"
#include "stdio.h"
#include "MACLib.h"
#include "APETag.h"
#include "CharacterHelper.h"
using namespace APE;

int wmain(int argc, wchar_t* argv[])
{
    ///////////////////////////////////////////////////////////////////////////////
    // error check the command line parameters
    ///////////////////////////////////////////////////////////////////////////////
    if (argc != 2) 
    {
        _tprintf(_T("~~~Improper Usage~~~\r\n\r\n"));
        _tprintf(_T("Usage Example: Sample 1.exe 'c:\\1.ape'\r\n\r\n"));
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // variable declares
    ///////////////////////////////////////////////////////////////////////////////
    int                    nRetVal = 0;                                        // generic holder for return values
    wchar_t                cTempBuffer[256]; ZeroMemory(&cTempBuffer[0], 256 * sizeof(wchar_t));    // generic buffer for string stuff
    wchar_t*            pFilename = argv[1];                                // the file to open
    IAPEDecompress *    pAPEDecompress = NULL;                                // APE interface
        
    ///////////////////////////////////////////////////////////////////////////////
    // open the file and error check
    ///////////////////////////////////////////////////////////////////////////////
    pAPEDecompress = CreateIAPEDecompress(pFilename, &nRetVal, false, true, false);
    if (pAPEDecompress == NULL)
    {
        _tprintf(_T("Error opening APE file. (error code %d)\r\n\r\n"), nRetVal);
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // display some information about the file
    ///////////////////////////////////////////////////////////////////////////////
    _tprintf(_T("Displaying information about '%s':\r\n\r\n"), pFilename);

    // file format information
    _tprintf(_T("File Format:\r\n"));
    _tprintf(_T("\tVersion: %.2f\r\n"), float(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)) / float(1000));
    switch (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL))
    {
        case MAC_COMPRESSION_LEVEL_FAST: printf("\tCompression level: Fast\r\n\r\n"); break;
        case MAC_COMPRESSION_LEVEL_NORMAL: printf("\tCompression level: Normal\r\n\r\n"); break;
        case MAC_COMPRESSION_LEVEL_HIGH: printf("\tCompression level: High\r\n\r\n"); break;
        case MAC_COMPRESSION_LEVEL_EXTRA_HIGH: printf("\tCompression level: Extra High\r\n\r\n"); break;
    }

    // audio format information
    _tprintf(_T("Audio Format:\r\n"));
    _tprintf(_T("\tSamples per second: %I64d\r\n"), pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE));
    _tprintf(_T("\tBits per sample: %I64d\r\n"), pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE));
    _tprintf(_T("\tNumber of channels: %I64d\r\n"), pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS));
    _tprintf(_T("\tPeak level: %I64d\r\n\r\n"), pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_PEAK_LEVEL));

    // size and duration information
    _tprintf(_T("Size and Duration:\r\n"));
    _tprintf(_T("\tLength of file (s): %I64d\r\n"), pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_LENGTH_MS) / 1000);
    _tprintf(_T("\tFile Size (kb): %I64d\r\n\r\n"), pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APE_TOTAL_BYTES) / 1024);
    
    // tag information
    _tprintf(_T("Tag Information:\r\n"));
    
    APE::CAPETag * pAPETag = (APE::CAPETag *) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TAG);
    BOOL bHasID3Tag = pAPETag->GetHasID3Tag();
    BOOL bHasAPETag = pAPETag->GetHasAPETag();
    
    if (bHasID3Tag || bHasAPETag)
    {
        // iterate through all the tag fields
        for (int nField = 0; true; nField++)
        { 
            APE::CAPETagField * pTagField = pAPETag->GetTagField(nField);
            if (pTagField == NULL)
                break;

            const wchar_t * pFieldName = (const wchar_t*) pTagField->GetFieldName();
            int a = pTagField->GetFieldSize();
            //const APE::str_utfn* pFieldName = NULL;
            //pFieldName = pTagField->GetFieldName();
            
            //const char * p = pTagField->GetFieldValue();

            // output the tag field properties (don't output huge fields like images, etc.)
            if (pTagField->GetFieldValueSize() > 128)
            {
                _tprintf(_T("\t%s: --- too much data to display ---\r\n"), pFieldName);
            }
            else
            {
                CSmartPtr<str_utfn> spUTF16;
                spUTF16.Assign(CAPECharacterHelper::GetUTF16FromUTF8((str_utf8 *) &pTagField->GetFieldValue()[0]), true);
                _tprintf(_T("\t%s: %s\r\n"), pFieldName, spUTF16);
            }
        }
    }
    else 
    {
        _tprintf(_T("\tNot tagged\r\n\r\n"));
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    // cleanup (just delete the object
    ///////////////////////////////////////////////////////////////////////////////
    delete pAPEDecompress;
    
    ///////////////////////////////////////////////////////////////////////////////
    // quit
    ///////////////////////////////////////////////////////////////////////////////
    return 0;
}
