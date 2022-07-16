#include "All.h"
#include "MACLib.h"
#include "APECompress.h"
#include "APEDecompress.h"
#include "APELink.h"
#include "GlobalFunctions.h"
#ifdef APE_BACKWARDS_COMPATIBILITY
    #include "Old/APEDecompressOld.h"
#endif

using namespace APE;

IAPEDecompress * CreateIAPEDecompressCore(CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock, int * pErrorCode)
{
    // create the decompressor (this eats the CAPEInfo object)
    IAPEDecompress * pAPEDecompress = NULL;

    // proceed if we have an info object
    if (pAPEInfo != NULL)
    {    
        // proceed if there's no error with the info object
        if (*pErrorCode == ERROR_SUCCESS)
        {
            try
            {
                // create
                if (pAPEInfo->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) >= 3930)
                    pAPEDecompress = new CAPEDecompress(pErrorCode, pAPEInfo, nStartBlock, nFinishBlock);
#ifdef APE_BACKWARDS_COMPATIBILITY
                else
                    pAPEDecompress = new CAPEDecompressOld(pErrorCode, pAPEInfo, nStartBlock, nFinishBlock);
#else
                else
                    *pErrorCode = ERROR_UNSUPPORTED_FILE_VERSION;
#endif

                // error check
                if (pAPEDecompress == NULL || *pErrorCode != ERROR_SUCCESS)
                {
                    SAFE_DELETE(pAPEDecompress)
                }
            }
            catch(...)
            {
                SAFE_DELETE(pAPEDecompress)
                *pErrorCode = ERROR_UNDEFINED;
            }
        }
        else
        {
            // eat the CAPEInfo object if we didn't create a decompressor
            SAFE_DELETE(pAPEInfo)
        }
    }

    // return
    return pAPEDecompress;
}

IAPEDecompress * __stdcall CreateIAPEDecompress(const str_utfn * pFilename, int * pErrorCode, bool bReadOnly, bool bAnalyzeTagNow, bool bReadWholeFile)
{
    // error check the parameters
    if ((pFilename == NULL) || (wcslen(pFilename) == 0))
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return NULL;
    }

    // variables
    int nErrorCode = ERROR_UNDEFINED;
    CAPEInfo * pAPEInfo = NULL;
    int nStartBlock = -1; int nFinishBlock = -1;

    // get the extension
    const str_utfn * pExtension = &pFilename[wcslen(pFilename)];
    while ((pExtension > pFilename) && (*pExtension != '.'))
        pExtension--;

    // take the appropriate action (based on the extension)
    if (StringIsEqual(pExtension, L".apl", false))
    {
        // "link" file (.apl linked large APE file)
        CAPELink APELink(pFilename);
        if (APELink.GetIsLinkFile())
        {
            pAPEInfo = new CAPEInfo(&nErrorCode, APELink.GetImageFilename(), new CAPETag(pFilename, true), true);
            nStartBlock = APELink.GetStartBlock(); nFinishBlock = APELink.GetFinishBlock();
        }
    }
    else if (StringIsEqual(pExtension, L".mac", false) || StringIsEqual(pExtension, L".ape", false))
    {
        // plain .ape file
        pAPEInfo = new CAPEInfo(&nErrorCode, pFilename, NULL, false, bReadOnly, bAnalyzeTagNow, bReadWholeFile);
    }

    // fail if we couldn't get the file information
    if (pAPEInfo == NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return NULL;
    }

    // create
    IAPEDecompress * pAPEDecompress = CreateIAPEDecompressCore(pAPEInfo, nStartBlock, nFinishBlock, &nErrorCode);
    if (pErrorCode) *pErrorCode = nErrorCode;

    // return
    return pAPEDecompress;
}

IAPEDecompress * __stdcall CreateIAPEDecompressEx(CIO * pIO, int * pErrorCode)
{
    // create info 
    int nErrorCode = ERROR_UNDEFINED;
    CAPEInfo * pAPEInfo = new CAPEInfo(&nErrorCode, pIO);

    // create decompress core
    IAPEDecompress * pAPEDecompress = CreateIAPEDecompressCore(pAPEInfo, -1, -1, &nErrorCode);
    if (pErrorCode) *pErrorCode = nErrorCode;

    // return
    return pAPEDecompress;
}

IAPEDecompress * __stdcall CreateIAPEDecompressEx2(CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock, int * pErrorCode)
{
    int nErrorCode = ERROR_SUCCESS;
    IAPEDecompress * pAPEDecompress = CreateIAPEDecompressCore(pAPEInfo, nStartBlock, nFinishBlock, &nErrorCode);
    if (pErrorCode) *pErrorCode = nErrorCode;

    return pAPEDecompress;
}

#ifdef APE_SUPPORT_COMPRESS
IAPECompress * __stdcall CreateIAPECompress(int * pErrorCode)
{
    if (pErrorCode)
        *pErrorCode = ERROR_SUCCESS;

    return new CAPECompress();
}
#endif

int __stdcall FillWaveFormatEx(APE::WAVEFORMATEX * pWaveFormatEx, int nFormatTag, int nSampleRate, int nBitsPerSample, int nChannels)
{
    pWaveFormatEx->cbSize = 0;
    pWaveFormatEx->nSamplesPerSec = nSampleRate;
    pWaveFormatEx->wBitsPerSample = WORD(nBitsPerSample);
    pWaveFormatEx->nChannels = WORD(nChannels);
    pWaveFormatEx->wFormatTag = (WORD) nFormatTag;

    pWaveFormatEx->nBlockAlign = (pWaveFormatEx->wBitsPerSample / 8) * pWaveFormatEx->nChannels;
    pWaveFormatEx->nAvgBytesPerSec = pWaveFormatEx->nBlockAlign * pWaveFormatEx->nSamplesPerSec;

    return ERROR_SUCCESS;
}

int __stdcall FillWaveHeader(WAVE_HEADER * pWAVHeader, APE::int64 nAudioBytes, APE::WAVEFORMATEX * pWaveFormatEx, APE::intn nTerminatingBytes)
{
    try
    {
        // RIFF header
        memcpy(pWAVHeader->cRIFFHeader, "RIFF", 4);
        pWAVHeader->nRIFFBytes = (unsigned int) ((nAudioBytes + 44) - 8 + nTerminatingBytes);

        // format header
        memcpy(pWAVHeader->cDataTypeID, "WAVE", 4);
        memcpy(pWAVHeader->cFormatHeader, "fmt ", 4);
        
        // the format chunk is the first 16 bytes of a waveformatex
        pWAVHeader->nFormatBytes = 16;
        memcpy(&pWAVHeader->nFormatTag, pWaveFormatEx, 16);

        // the data header
        memcpy(pWAVHeader->cDataHeader, "data", 4);
        if (nAudioBytes >= 0xFFFFFFFF)
            pWAVHeader->nDataBytes = (unsigned int) - 1;
        else
            pWAVHeader->nDataBytes = (unsigned int) nAudioBytes;

        return ERROR_SUCCESS;
    }
    catch(...) { return ERROR_UNDEFINED; }
}

int __stdcall FillRF64Header(RF64_HEADER * pWAVHeader, APE::int64 nAudioBytes, APE::WAVEFORMATEX * pWaveFormatEx)
{
    try
    {
        // RIFF header
        memcpy(pWAVHeader->cRIFFHeader, "RF64", 4);
        pWAVHeader->nRIFFBytes = (unsigned int) -1; // only used for big files

        // WAV header
        memcpy(pWAVHeader->cDataTypeID, "WAVE", 4);

        // DS64 header
        memcpy(pWAVHeader->cDS64, "ds64", 4);
        pWAVHeader->nDSHeaderSize = int((char *) &pWAVHeader->cFormatHeader - (char *) &pWAVHeader->nRIFFSize); // size between nRIFFSize and nTableLength (cFormatHeader is just after nTableLength)
        pWAVHeader->nRIFFSize = sizeof(RF64_HEADER) + nAudioBytes - 8; // size of entire ds64 chunk minus the very header
        pWAVHeader->nDataSize = nAudioBytes;
        pWAVHeader->nSampleCount = nAudioBytes / pWaveFormatEx->nBlockAlign; // it's called sample count, but David Bryant puts blocks in WavPack so I'll do the same
        pWAVHeader->nTableLength = 0;

        // format header
        memcpy(pWAVHeader->cFormatHeader, "fmt ", 4);
        
        // the format chunk is the first 16 bytes of a waveformatex
        pWAVHeader->nFormatBytes = 16;
        memcpy(&pWAVHeader->nFormatTag, pWaveFormatEx, 16);

        // the data header
        memcpy(pWAVHeader->cDataHeader, "data", 4);
        if (nAudioBytes >= 0xFFFFFFFF)
            pWAVHeader->nDataBytes = (unsigned int) -1;
        else
            pWAVHeader->nDataBytes = (unsigned int) nAudioBytes;

        return ERROR_SUCCESS;
    }
    catch(...) { return ERROR_UNDEFINED; }
}

