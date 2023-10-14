#include "All.h"
#define APE_ENABLE_MD5_ADD_DATA
#include "MACLib.h"
#include "APECompress.h"
#include "APEDecompress.h"
#include "APELink.h"
#include "GlobalFunctions.h"
#include "MACProgressHelper.h"
#include "CharacterHelper.h"
#include "WAVInputSource.h"
#include "MD5.h"
#ifdef APE_BACKWARDS_COMPATIBILITY
    #include "Old/APEDecompressOld.h"
#endif

using namespace APE;

#define UNMAC_DECODER_OUTPUT_NONE       0
#define UNMAC_DECODER_OUTPUT_WAV        1
#define UNMAC_DECODER_OUTPUT_APE        2

#define BLOCKS_PER_DECODE               9216

IAPEDecompress * CreateIAPEDecompressCore(CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock, int * pErrorCode);
int DecompressCore(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nOutputMode, int nCompressionLevel, IAPEProgressCallback * pProgressCallback, IAPEDecompress * pDecompress);

/**************************************************************************************************
Functions to create the interfaces
**************************************************************************************************/
IAPEDecompress * CreateIAPEDecompressCore(CAPEInfo * pAPEInfo, int nStartBlock, int nFinishBlock, int * pErrorCode)
{
    // create the decompressor (this eats the CAPEInfo object)
    CSmartPtr<IAPEDecompress> spAPEDecompress;

    // proceed if we have an info object
    if (pAPEInfo != APE_NULL)
    {
        // proceed if there's no error with the info object
        if (*pErrorCode == ERROR_SUCCESS)
        {
            try
            {
                // create
                const int nVersion = static_cast<int>(pAPEInfo->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION));
                if ((nVersion >= 3930) && (nVersion <= APE_FILE_VERSION_NUMBER))
                    spAPEDecompress.Assign(new CAPEDecompress(pErrorCode, pAPEInfo, nStartBlock, nFinishBlock));
#ifdef APE_BACKWARDS_COMPATIBILITY
                else if (nVersion < 3930)
                    spAPEDecompress.Assign(new CAPEDecompressOld(pErrorCode, pAPEInfo, nStartBlock, nFinishBlock));
#endif
                else
                    *pErrorCode = ERROR_UNSUPPORTED_FILE_VERSION;

                // error check
                if (spAPEDecompress == APE_NULL || *pErrorCode != ERROR_SUCCESS)
                {
                    spAPEDecompress.Delete();
                }
            }
            catch(...)
            {
                spAPEDecompress.Delete();
                *pErrorCode = ERROR_UNDEFINED;
            }
        }
        else
        {
            // eat the CAPEInfo object if we didn't create a decompressor
            APE_SAFE_DELETE(pAPEInfo)
        }
    }

    // return
    spAPEDecompress.SetDelete(false);
    return spAPEDecompress.GetPtr();
}

IAPEDecompress * __stdcall CreateIAPEDecompress(const str_utfn * pFilename, int * pErrorCode, bool bReadOnly, bool bAnalyzeTagNow, bool bReadWholeFile)
{
    // error check the parameters
    if ((pFilename == APE_NULL) || (wcslen(pFilename) == 0))
    {
        if (pErrorCode) *pErrorCode = ERROR_BAD_PARAMETER;
        return APE_NULL;
    }

    // variables
    int nErrorCode = ERROR_UNDEFINED;
    CAPEInfo * pAPEInfo = APE_NULL;
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
            if (nErrorCode != ERROR_SUCCESS)
            {
                APE_SAFE_DELETE(pAPEInfo)
                if (pErrorCode) *pErrorCode = nErrorCode;
                return APE_NULL;
            }
            nStartBlock = APELink.GetStartBlock(); nFinishBlock = APELink.GetFinishBlock();
        }
    }
    else if (StringIsEqual(pExtension, L".mac", false) || StringIsEqual(pExtension, L".ape", false))
    {
        // plain .ape file
        pAPEInfo = new CAPEInfo(&nErrorCode, pFilename, APE_NULL, false, bReadOnly, bAnalyzeTagNow, bReadWholeFile);
        if (nErrorCode != ERROR_SUCCESS)
        {
            APE_SAFE_DELETE(pAPEInfo)
            if (pErrorCode) *pErrorCode = nErrorCode;
            return APE_NULL;
        }
    }

    // fail if we couldn't get the file information
    if (pAPEInfo == APE_NULL)
    {
        if (pErrorCode) *pErrorCode = ERROR_INVALID_INPUT_FILE;
        return APE_NULL;
    }

    // create
    nErrorCode = ERROR_SUCCESS;
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

/**************************************************************************************************
Fill wave headers
**************************************************************************************************/
int __stdcall FillWaveFormatEx(APE::WAVEFORMATEX * pWaveFormatEx, int nFormatTag, int nSampleRate, int nBitsPerSample, int nChannels)
{
    pWaveFormatEx->cbSize = 0;
    pWaveFormatEx->nSamplesPerSec = static_cast<uint32>(nSampleRate);
    pWaveFormatEx->wBitsPerSample = static_cast<WORD>(nBitsPerSample);
    pWaveFormatEx->nChannels = static_cast<WORD>(nChannels);
    pWaveFormatEx->wFormatTag = static_cast<WORD>(nFormatTag);

    pWaveFormatEx->nBlockAlign = static_cast<WORD>((pWaveFormatEx->wBitsPerSample / 8) * pWaveFormatEx->nChannels);
    pWaveFormatEx->nAvgBytesPerSec = pWaveFormatEx->nBlockAlign * pWaveFormatEx->nSamplesPerSec;

    return ERROR_SUCCESS;
}

int __stdcall FillWaveHeader(WAVE_HEADER * pWAVHeader, APE::int64 nAudioBytes, const APE::WAVEFORMATEX * pWaveFormatEx, APE::intn nTerminatingBytes)
{
    try
    {
        // RIFF header
        memcpy(pWAVHeader->cRIFFHeader, "RIFF", 4);
        pWAVHeader->nRIFFBytes = static_cast<unsigned int>((nAudioBytes + 44) - 8 + nTerminatingBytes);

        // format header
        memcpy(pWAVHeader->cDataTypeID, "WAVE", 4);
        memcpy(pWAVHeader->cFormatHeader, "fmt ", 4);

        // the format chunk is the first 16 bytes of a waveformatex
        pWAVHeader->nFormatBytes = 16;
        memcpy(&pWAVHeader->nFormatTag, pWaveFormatEx, 16);

        // the data header
        memcpy(pWAVHeader->cDataHeader, "data", 4);
        if (nAudioBytes >= 0xFFFFFFFF)
            pWAVHeader->nDataBytes = static_cast<unsigned int>(-1);
        else
            pWAVHeader->nDataBytes = static_cast<unsigned int>(nAudioBytes);

        return ERROR_SUCCESS;
    }
    catch(...) { return ERROR_UNDEFINED; }
}

int __stdcall FillRF64Header(RF64_HEADER * pWAVHeader, APE::int64 nAudioBytes, const APE::WAVEFORMATEX * pWaveFormatEx)
{
    try
    {
        // RIFF header
        memcpy(pWAVHeader->cRIFFHeader, "RF64", 4);
        pWAVHeader->nRIFFBytes = static_cast<unsigned int>(-1); // only used for big files

        // WAV header
        memcpy(pWAVHeader->cDataTypeID, "WAVE", 4);

        // DS64 header
        memcpy(pWAVHeader->cDS64, "ds64", 4);
        pWAVHeader->nDSHeaderSize = static_cast<int>(reinterpret_cast<char *>(&pWAVHeader->cFormatHeader) - reinterpret_cast<char *>(&pWAVHeader->nRIFFSize)); // size between nRIFFSize and nTableLength (cFormatHeader is just after nTableLength)
        pWAVHeader->nRIFFSize = static_cast<int64>(sizeof(RF64_HEADER)) + nAudioBytes - 8; // size of entire ds64 chunk minus the very header
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
            pWAVHeader->nDataBytes = static_cast<unsigned int>(-1);
        else
            pWAVHeader->nDataBytes = static_cast<unsigned int>(nAudioBytes);

        return ERROR_SUCCESS;
    }
    catch(...) { return ERROR_UNDEFINED; }
}

int __stdcall GetAPEFileType(const APE::str_utfn * pInputFilename, APE::str_ansi cFileType[8])
{
    memset(&cFileType[0], 0, sizeof(APE::str_ansi) * 8);

    int nErrorCode = 0;
    CAPEInfo Info(&nErrorCode, pInputFilename, APE_NULL, false, true, false);

    // customize the type if we're AIFF, W64, etc.
    int64 nFormat = Info.GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS);
    if (nFormat & APE_FORMAT_FLAG_AIFF)
    {
        strcpy_s(cFileType, 8, ".aiff");
    }
    else if (nFormat & APE_FORMAT_FLAG_W64)
    {
        strcpy_s(cFileType, 8, ".w64");
    }
    else if (nFormat & APE_FORMAT_FLAG_SND)
    {
        strcpy_s(cFileType, 8, ".snd");
    }
    else if (nFormat & APE_FORMAT_FLAG_CAF)
    {
        strcpy_s(cFileType, 8, ".caf");
    }
    else
    {
        strcpy_s(cFileType, 8, ".wav");
    }

    return ERROR_SUCCESS;
}

void __stdcall GetAPECompressionLevelName(int nCompressionLevel, APE::str_utfn * pCompressionLevel, size_t nBufferCharacters, bool bTitleCase)
{
    if (bTitleCase)
    {
        switch (nCompressionLevel)
        {
            case 1000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"Fast"); break;
            case 2000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"Normal"); break;
            case 3000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"High"); break;
            case 4000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"Extra High"); break;
            case 5000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"Insane"); break;
            default: wcscpy_s(pCompressionLevel, nBufferCharacters, L"Unknown"); break;
        }
    }
    else
    {
        switch (nCompressionLevel)
        {
            case 1000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"fast"); break;
            case 2000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"normal"); break;
            case 3000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"high"); break;
            case 4000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"extra high"); break;
            case 5000: wcscpy_s(pCompressionLevel, nBufferCharacters, L"insane"); break;
            default: wcscpy_s(pCompressionLevel, nBufferCharacters, L"unknown"); break;
        }
    }
}

/**************************************************************************************************
Simple progress callback (for legacy support)
**************************************************************************************************/
class CAPEProgressCallbackLegacy : public IAPEProgressCallback
{
public:
    CAPEProgressCallbackLegacy(int * pProgress, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
    {
        m_pProgress = pProgress;
        m_ProgressCallback = ProgressCallback;
        m_pKillFlag = pKillFlag;
        m_cType[0] = 0;
    }

    virtual void Progress(int nPercentageDone) APE_OVERRIDE
    {
        if (m_pProgress != APE_NULL)
            *m_pProgress = nPercentageDone;

        if (m_ProgressCallback != APE_NULL)
            m_ProgressCallback(nPercentageDone);
    }

    virtual int GetKillFlag() APE_OVERRIDE
    {
        return (m_pKillFlag == APE_NULL) ? KILL_FLAG_CONTINUE : *m_pKillFlag;
    }

private:
    int * m_pProgress;
    APE_PROGRESS_CALLBACK m_ProgressCallback;
    int * m_pKillFlag;
    str_ansi m_cType[8];
};

/**************************************************************************************************
ANSI wrappers
**************************************************************************************************/
#ifdef APE_SUPPORT_COMPRESS
int __stdcall CompressFile(const APE::str_ansi * pInputFilename, const APE::str_ansi * pOutputFilename, int nCompressionLevel, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    CSmartPtr<str_utfn> spInputFile(CAPECharacterHelper::GetUTF16FromANSI(pInputFilename), true);
    CSmartPtr<str_utfn> spOutputFile(CAPECharacterHelper::GetUTF16FromANSI(pOutputFilename), true);
    return CompressFileW(spInputFile, spOutputFile, nCompressionLevel, pPercentageDone, ProgressCallback, pKillFlag);
}
#endif

int __stdcall DecompressFile(const APE::str_ansi * pInputFilename, const APE::str_ansi * pOutputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    if (pOutputFilename == APE_NULL)
    {
        CSmartPtr<str_utfn> spInputFile(CAPECharacterHelper::GetUTF16FromANSI(pInputFilename), true);
        return DecompressFileW(spInputFile, APE_NULL, pPercentageDone, ProgressCallback, pKillFlag);
    }
    else
    {
        CSmartPtr<str_utfn> spInputFile(CAPECharacterHelper::GetUTF16FromANSI(pInputFilename), true);
        CSmartPtr<str_utfn> spOutputFile(CAPECharacterHelper::GetUTF16FromANSI(pOutputFilename), true);
        return DecompressFileW(spInputFile, spOutputFile, pPercentageDone, ProgressCallback, pKillFlag);
    }
}

int __stdcall ConvertFile(const APE::str_ansi * pInputFilename, const APE::str_ansi * pOutputFilename, int nCompressionLevel, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    CSmartPtr<str_utfn> spInputFile(CAPECharacterHelper::GetUTF16FromANSI(pInputFilename), true);
    CSmartPtr<str_utfn> spOutputFile(CAPECharacterHelper::GetUTF16FromANSI(pOutputFilename), true);
    return ConvertFileW(spInputFile, spOutputFile, nCompressionLevel, pPercentageDone, ProgressCallback, pKillFlag);
}

int __stdcall VerifyFile(const APE::str_ansi * pInputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag, bool bQuickVerifyIfPossible)
{
    CSmartPtr<str_utfn> spInputFile(CAPECharacterHelper::GetUTF16FromANSI(pInputFilename), true);
    return VerifyFileW(spInputFile, pPercentageDone, ProgressCallback, pKillFlag, bQuickVerifyIfPossible);
}

/**************************************************************************************************
Legacy callback wrappers
**************************************************************************************************/
#ifdef APE_SUPPORT_COMPRESS
int __stdcall CompressFileW(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nCompressionLevel, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    CAPEProgressCallbackLegacy ProgressCallbackLegacy(pPercentageDone, ProgressCallback, pKillFlag);
    return CompressFileW2(pInputFilename, pOutputFilename, nCompressionLevel, &ProgressCallbackLegacy);
}
#endif

int __stdcall VerifyFileW(const APE::str_utfn * pInputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag, bool bQuickVerifyIfPossible)
{
    CAPEProgressCallbackLegacy ProgressCallbackLegacy(pPercentageDone, ProgressCallback, pKillFlag);
    return VerifyFileW2(pInputFilename, &ProgressCallbackLegacy, bQuickVerifyIfPossible);
}

int __stdcall DecompressFileW(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    CAPEProgressCallbackLegacy ProgressCallbackLegacy(pPercentageDone, ProgressCallback, pKillFlag);
    const int nResult = DecompressFileW2(pInputFilename, pOutputFilename, &ProgressCallbackLegacy);
    return nResult;
}

int __stdcall ConvertFileW(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nCompressionLevel, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag)
{
    CAPEProgressCallbackLegacy ProgressCallbackLegacy(pPercentageDone, ProgressCallback, pKillFlag);
    return ConvertFileW2(pInputFilename, pOutputFilename, nCompressionLevel, &ProgressCallbackLegacy);
}

/**************************************************************************************************
Compress file
**************************************************************************************************/
#ifdef APE_SUPPORT_COMPRESS
int __stdcall CompressFileW2(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nCompressionLevel, IAPEProgressCallback * pProgressCallback)
{
    // declare the variables
    int nFunctionRetVal = ERROR_SUCCESS;
    APE::WAVEFORMATEX WaveFormatEx; APE_CLEAR(WaveFormatEx);
    CSmartPtr<CMACProgressHelper> spMACProgressHelper;
    CSmartPtr<unsigned char> spBuffer;
    CSmartPtr<IAPECompress> spAPECompress;

    try
    {
        // create the input source
        int nResult = ERROR_UNDEFINED;
        int64 nAudioBlocks = 0; int64 nHeaderBytes = 0; int64 nTerminatingBytes = 0; int32 nFlags = 0;
        CSmartPtr<CInputSource> spInputSource(CreateInputSource(pInputFilename, &WaveFormatEx, &nAudioBlocks,
            &nHeaderBytes, &nTerminatingBytes, &nFlags, &nResult));

        // check header and footer sizes right away (we also check the footer at the end, but that would require compressing the whole file then rejecting)
        if ((nHeaderBytes > APE_WAV_HEADER_OR_FOOTER_MAXIMUM_BYTES) ||
            (nTerminatingBytes > APE_WAV_HEADER_OR_FOOTER_MAXIMUM_BYTES))
        {
            throw static_cast<intn>(ERROR_INPUT_FILE_TOO_LARGE);
        }

        if ((spInputSource == APE_NULL) || (nResult != ERROR_SUCCESS))
            throw static_cast<intn>(nResult);

        // create the compressor
        spAPECompress.Assign(CreateIAPECompress());
        if (spAPECompress == APE_NULL) throw static_cast<intn>(ERROR_UNDEFINED);

        // figure the audio bytes
        int64 nAudioBytes = static_cast<int64>(nAudioBlocks) * static_cast<int64>(WaveFormatEx.nBlockAlign);
        if (spInputSource->GetUnknownLengthPipe())
            nAudioBytes = MAX_AUDIO_BYTES_UNKNOWN;
        if ((nAudioBytes <= 0) && (nAudioBytes != MAX_AUDIO_BYTES_UNKNOWN))
            throw static_cast<intn>(ERROR_INPUT_FILE_TOO_SMALL);

        // start the encoder
        if (nHeaderBytes > 0) spBuffer.Assign(new unsigned char[static_cast<uint32>(nHeaderBytes)], true);
        THROW_ON_ERROR(spInputSource->GetHeaderData(spBuffer.GetPtr()))
        THROW_ON_ERROR(spAPECompress->Start(pOutputFilename, &WaveFormatEx, nAudioBytes, nCompressionLevel, spBuffer.GetPtr(), nHeaderBytes, nFlags))
        spBuffer.Delete();

        // set-up the progress
        spMACProgressHelper.Assign(new CMACProgressHelper(nAudioBytes, pProgressCallback));

        // master loop
        int64 nBytesLeft = nAudioBytes;
        const bool bUnknownLengthPipe = spInputSource->GetUnknownLengthPipe();
        while ((nBytesLeft > 0) || bUnknownLengthPipe)
        {
            // add data
            int64 nBytesAdded = 0;
            const int64 nRetVal = spAPECompress->AddDataFromInputSource(spInputSource.GetPtr(), nBytesLeft, &nBytesAdded);
            if (bUnknownLengthPipe && (nRetVal == ERROR_IO_READ))
                break; // this means we reached the end of the file
            else if (nRetVal != ERROR_SUCCESS)
                throw(static_cast<intn>(nRetVal));
            nBytesLeft -= nBytesAdded;

            // update the progress
            if (nAudioBytes != -1)
                spMACProgressHelper->UpdateProgress(nAudioBytes - nBytesLeft);

            // process the kill flag
            if (spMACProgressHelper->ProcessKillFlag() != ERROR_SUCCESS)
                throw(static_cast<intn>(ERROR_USER_STOPPED_PROCESSING));
        }

        // finalize the file
        if (nTerminatingBytes > 0)
        {
            spBuffer.Assign(new unsigned char[static_cast<uint32>(nTerminatingBytes)], true);
            THROW_ON_ERROR(spInputSource->GetTerminatingData(spBuffer.GetPtr()))
        }
        THROW_ON_ERROR(spAPECompress->Finish(spBuffer.GetPtr(), nTerminatingBytes, nTerminatingBytes))

        // update the progress to 100%
        spMACProgressHelper->UpdateProgressComplete();
    }
    catch (const intn nErrorCode)
    {
        nFunctionRetVal = (nErrorCode == 0) ? ERROR_UNDEFINED : static_cast<int>(nErrorCode);
    }
    catch (...)
    {
        nFunctionRetVal = ERROR_UNDEFINED;
    }

    // kill the compressor if we failed
    if ((nFunctionRetVal != 0) && (spAPECompress != APE_NULL))
        spAPECompress->Kill();

    // return
    return nFunctionRetVal;
}
#endif

/**************************************************************************************************
Verify file
**************************************************************************************************/
int __stdcall VerifyFileW2(const APE::str_utfn * pInputFilename, IAPEProgressCallback * pProgressCallback, bool bQuickVerifyIfPossible)
{
    // error check the function parameters
    if (pInputFilename == APE_NULL)
    {
        return ERROR_INVALID_FUNCTION_PARAMETER;
    }

    // return value
    int nResult = ERROR_UNDEFINED;

    // see if we can quick verify
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    if (bQuickVerifyIfPossible)
    {
        try
        {
            int nFunctionRetVal = ERROR_SUCCESS;

            //spAPEDecompress.Assign(CreateIAPEDecompress(pInputFilename, &nFunctionRetVal, true, false, true));
            spAPEDecompress.Assign(CreateIAPEDecompress(pInputFilename, &nFunctionRetVal, true, false, true));
            if (spAPEDecompress == APE_NULL || nFunctionRetVal != ERROR_SUCCESS) throw(static_cast<intn>(nFunctionRetVal));

            const APE_FILE_INFO * pInfo = GET_INFO(spAPEDecompress);

            // if we're an APL file, we need to slow verify since we're just a little chunk in a big file
            // in the past we would just check the whole file with a quick verify, but slow verify seems better in this case
            if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APL))
                throw(static_cast<intn>(ERROR_UNSUPPORTED_FILE_TYPE));

            // check version
            if ((pInfo->nVersion < 3980) || (pInfo->spAPEDescriptor == APE_NULL))
                throw(static_cast<intn>(ERROR_UNSUPPORTED_FILE_VERSION));

            // make sure the MD5 is valid
            if (pInfo->nMD5Invalid)
                throw(static_cast<intn>(ERROR_UNSUPPORTED_FILE_VERSION));
        }
        catch (...)
        {
            bQuickVerifyIfPossible = false;
        }
    }

    // if we can and should quick verify, then do it
    if (bQuickVerifyIfPossible)
    {
        // variable declares
        int nFunctionRetVal = ERROR_SUCCESS;

        // run the quick verify
        try
        {
            CMD5Helper MD5Helper;
            unsigned int nBytesRead = 0;

            CIO * pIO = GET_IO(spAPEDecompress);
            const APE_FILE_INFO * pInfo = GET_INFO(spAPEDecompress);

            if ((pInfo->nVersion < 3980) || (pInfo->spAPEDescriptor == APE_NULL))
                throw(static_cast<intn>(ERROR_UNSUPPORTED_FILE_VERSION));

            // read APE header
            CSmartPtr<unsigned char> spAPEHeader(new unsigned char[pInfo->spAPEDescriptor->nHeaderBytes], true);
            pIO->Seek(static_cast<int64>(pInfo->nJunkHeaderBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nDescriptorBytes), SeekFileBegin);
            pIO->Read(spAPEHeader, pInfo->spAPEDescriptor->nHeaderBytes, &nBytesRead);

            // read seek table
            CSmartPtr<unsigned char> spSeekTable(new unsigned char[pInfo->spAPEDescriptor->nSeekTableBytes], true);
            ASSERT(pIO->GetPosition() == (static_cast<int64>(pInfo->nJunkHeaderBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nDescriptorBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nHeaderBytes)));
            pIO->Read(spSeekTable, pInfo->spAPEDescriptor->nSeekTableBytes, &nBytesRead);

            // read header data
            CSmartPtr<unsigned char> spHeader(new unsigned char[pInfo->spAPEDescriptor->nHeaderDataBytes], true);
            ASSERT(pIO->GetPosition() == (static_cast<int64>(pInfo->nJunkHeaderBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nDescriptorBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nHeaderBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nSeekTableBytes)));
            pIO->Read(spHeader, static_cast<unsigned int>(pInfo->spAPEDescriptor->nHeaderDataBytes), &nBytesRead);

            // seek to the data (we should already be there)
            ASSERT(pIO->GetPosition() == (static_cast<int64>(pInfo->nJunkHeaderBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nDescriptorBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nHeaderBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nSeekTableBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nHeaderDataBytes)));

            // add the WAV header to the MD5 first
            MD5Helper.AddData(spHeader, pInfo->spAPEDescriptor->nHeaderDataBytes);

            // bytes left
            int64 nBytesLeft = (static_cast<int64>(pInfo->spAPEDescriptor->nAPEFrameDataBytesHigh) << 32) + static_cast<int64>(pInfo->spAPEDescriptor->nAPEFrameDataBytes) + static_cast<int64>(pInfo->spAPEDescriptor->nTerminatingDataBytes);
            const int64 nBytesLeftOriginal = nBytesLeft;

            // read in smaller chunks
            CSmartPtr<CMACProgressHelper> spMACProgressHelper;
            spMACProgressHelper.Assign(new CMACProgressHelper(nBytesLeft, pProgressCallback));

            CSmartPtr<unsigned char> spBuffer(new unsigned char[16384], true);
            nBytesRead = 1;
            while ((nBytesLeft > 0) && (nBytesRead > 0))
            {
                const unsigned int nBytesToRead = static_cast<unsigned int>(ape_min(16384, nBytesLeft));
                if (pIO->Read(spBuffer, nBytesToRead, &nBytesRead) != ERROR_SUCCESS)
                    throw(static_cast<intn>(ERROR_IO_READ));

                MD5Helper.AddData(spBuffer, nBytesRead);
                spMACProgressHelper->UpdateProgress(nBytesLeftOriginal - nBytesLeft);

                nBytesLeft -= nBytesRead;

                if (spMACProgressHelper->ProcessKillFlag() != ERROR_SUCCESS)
                    throw(static_cast<intn>(ERROR_USER_STOPPED_PROCESSING));
            }

            if (nBytesLeft != 0)
                throw(static_cast<intn>(ERROR_IO_READ));

            // add the header and seek table
            MD5Helper.AddData(spAPEHeader, pInfo->spAPEDescriptor->nHeaderBytes);
            MD5Helper.AddData(spSeekTable, pInfo->spAPEDescriptor->nSeekTableBytes);

            // get results
            unsigned char cResult[16];
            MD5Helper.GetResult(cResult);

            // compare to stored
            nFunctionRetVal = static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_MD5_MATCHES, POINTER_TO_INT64(&cResult[0])));

            // update the progress to 100%
            spMACProgressHelper->UpdateProgressComplete();
        }
        catch (const intn nErrorCode)
        {
            nFunctionRetVal = (nErrorCode == 0) ? ERROR_UNDEFINED : static_cast<int>(nErrorCode);
        }
        catch (...)
        {
            nFunctionRetVal = ERROR_UNDEFINED;
        }

        // return value
        nResult = nFunctionRetVal;
    }
    else
    {
        nResult = DecompressCore(pInputFilename, APE_NULL, UNMAC_DECODER_OUTPUT_NONE, -1, pProgressCallback, spAPEDecompress);
    }

    return nResult;
}

/**************************************************************************************************
Decompress file
**************************************************************************************************/
int __stdcall DecompressFileW2(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, IAPEProgressCallback * pProgressCallback)
{
    if (pOutputFilename == APE_NULL)
        return VerifyFileW2(pInputFilename, pProgressCallback);
    else
        return DecompressCore(pInputFilename, pOutputFilename, UNMAC_DECODER_OUTPUT_WAV, -1, pProgressCallback, APE_NULL);
}

/**************************************************************************************************
Convert file
**************************************************************************************************/
int __stdcall ConvertFileW2(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nCompressionLevel, IAPEProgressCallback * pProgressCallback)
{
    return DecompressCore(pInputFilename, pOutputFilename, UNMAC_DECODER_OUTPUT_APE, nCompressionLevel, pProgressCallback, APE_NULL);
}

/**************************************************************************************************
Decompress a file using the specified output method
**************************************************************************************************/
int DecompressCore(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nOutputMode, int nCompressionLevel, IAPEProgressCallback * pProgressCallback, IAPEDecompress * pDecompress)
{
    // error check the function parameters
    if (pInputFilename == APE_NULL)
    {
        return ERROR_INVALID_FUNCTION_PARAMETER;
    }

    // variable declares
    int nFunctionRetVal = ERROR_SUCCESS;
    CSmartPtr<CIO> spioOutput;
    CSmartPtr<IAPECompress> spAPECompress;
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    CSmartPtr<unsigned char> spTempBuffer;
    CSmartPtr<CMACProgressHelper> spMACProgressHelper;
    APE::WAVEFORMATEX wfeInput; APE_CLEAR(wfeInput);

    try
    {
        // create the decoder (or used the one passed in)
        if (pDecompress != APE_NULL)
        {
            spAPEDecompress.Assign(pDecompress, false, false);
        }
        else
        {
            spAPEDecompress.Assign(CreateIAPEDecompress(pInputFilename, &nFunctionRetVal, true, true, false));
        }
        if (spAPEDecompress == APE_NULL || nFunctionRetVal != ERROR_SUCCESS) throw(static_cast<intn>(nFunctionRetVal));

        // get the input format
        THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, POINTER_TO_INT64(&wfeInput)))

        // allocate space for the header
        spTempBuffer.Assign(new unsigned char[static_cast<size_t>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES))], true);
        if (spTempBuffer == APE_NULL) throw(static_cast<intn>(ERROR_INSUFFICIENT_MEMORY));

        // get the header
        const int64 nHeaderBytes = spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES);
        THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_DATA, POINTER_TO_INT64(spTempBuffer.GetPtr()), nHeaderBytes))

        // initialize the output
        if (nOutputMode == UNMAC_DECODER_OUTPUT_WAV)
        {
            // create the file
            spioOutput.Assign(CreateCIO()); THROW_ON_ERROR(spioOutput->Create(pOutputFilename))

            // output the header
            THROW_ON_ERROR(WriteSafe(spioOutput, spTempBuffer, static_cast<intn>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES))))
        }
#ifdef APE_SUPPORT_COMPRESS
        else if (nOutputMode == UNMAC_DECODER_OUTPUT_APE)
        {
            // quit if there is nothing to do
            // no longer check when processing an APL since converting an APL to APE should work no matter what
            if ((spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) == APE_FILE_VERSION_NUMBER) &&
                (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL) == nCompressionLevel) &&
                (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APL) == false))
            {
                throw(static_cast<intn>(ERROR_SKIPPED));
            }

            // flags
            const int nSourceFlags = static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS));
            const int nFlags = nSourceFlags;

            // originally just some flags were passed, but it seems like simply passing all of them is correct so that was changed on 3/6/2023
            //int nFlags = (nSourceFlags & MAC_FORMAT_FLAG_CAF) | (nSourceFlags & MAC_FORMAT_FLAG_SND) | (nSourceFlags & MAC_FORMAT_FLAG_W64) | (nSourceFlags & MAC_FORMAT_FLAG_AIFF);
            //nFlags |= (nSourceFlags & MAC_FORMAT_FLAG_SIGNED_8_BIT) | (nSourceFlags & MAC_FORMAT_FLAG_FLOATING_POINT);

            // create and start the compressor
            spAPECompress.Assign(CreateIAPECompress());
            THROW_ON_ERROR(spAPECompress->Start(pOutputFilename, &wfeInput, (spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS) * spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN)),
                nCompressionLevel, spTempBuffer, spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES), nFlags))
        }
#endif

        // allocate space for decompression
        spTempBuffer.Assign(new unsigned char[static_cast<size_t>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN)) * BLOCKS_PER_DECODE], true);
        if (spTempBuffer == APE_NULL) throw(static_cast<intn>(ERROR_INSUFFICIENT_MEMORY));

        int64 nBlocksLeft = static_cast<intn>(spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS));

        // create the progress helper
        spMACProgressHelper.Assign(new CMACProgressHelper(nBlocksLeft / BLOCKS_PER_DECODE, pProgressCallback));

        // main decoding loop
        while (nBlocksLeft > 0)
        {
            // processing flags
            IAPEDecompress::APE_GET_DATA_PROCESSING Processing;
            Processing.bApplyFloatProcessing = (nOutputMode != UNMAC_DECODER_OUTPUT_APE);
            Processing.bApplySigned8BitProcessing = (nOutputMode != UNMAC_DECODER_OUTPUT_APE);
            Processing.bApplyBigEndianProcessing = (nOutputMode != UNMAC_DECODER_OUTPUT_APE);

            // decode data
            int64 nBlocksDecoded = -1;
            const int nResult = spAPEDecompress->GetData(spTempBuffer, BLOCKS_PER_DECODE, &nBlocksDecoded, &Processing);
            if (nResult != ERROR_SUCCESS)
                throw(static_cast<intn>(nResult));

            // handle the output
            if (nOutputMode == UNMAC_DECODER_OUTPUT_WAV)
            {
                const unsigned int nBytesToWrite = static_cast<unsigned int>(nBlocksDecoded * spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN));
                unsigned int nBytesWritten = 0;
                const int nWriteResult = spioOutput->Write(spTempBuffer, nBytesToWrite, &nBytesWritten);
                if ((nWriteResult != 0) || (nBytesToWrite != nBytesWritten))
                    throw(static_cast<intn>(ERROR_IO_WRITE));
            }
            else if (nOutputMode == UNMAC_DECODER_OUTPUT_APE)
            {
                THROW_ON_ERROR(spAPECompress->AddData(spTempBuffer, (nBlocksDecoded * spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN))))
            }

            // update amount remaining
            nBlocksLeft -= nBlocksDecoded;

            // update progress and kill flag
            spMACProgressHelper->UpdateProgress();
            if (spMACProgressHelper->ProcessKillFlag() != ERROR_SUCCESS)
                throw(static_cast<intn>(ERROR_USER_STOPPED_PROCESSING));
        }

        // terminate the output
        if (nOutputMode == UNMAC_DECODER_OUTPUT_WAV)
        {
            // write any terminating WAV data
            if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES) > 0)
            {
                spTempBuffer.Assign(new unsigned char[static_cast<size_t>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES))], true);
                if (spTempBuffer == APE_NULL) throw(static_cast<intn>(ERROR_INSUFFICIENT_MEMORY));
                THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_DATA, POINTER_TO_INT64(spTempBuffer.GetPtr()), spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES)))

                const unsigned int nBytesToWrite = static_cast<unsigned int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES));
                unsigned int nBytesWritten = 0;
                const int nResult = spioOutput->Write(spTempBuffer, nBytesToWrite, &nBytesWritten);
                if ((nResult != 0) || (nBytesToWrite != nBytesWritten))
                    throw(static_cast<intn>(ERROR_IO_WRITE));
            }
        }
        else if (nOutputMode == UNMAC_DECODER_OUTPUT_APE)
        {
            // write the WAV data and any tag
            IAPETag * pAPETag = GET_TAG(spAPEDecompress);
            const int64 nTagBytes = (pAPETag != APE_NULL) ? pAPETag->GetTagBytes() : 0;
            const bool bHasTag = (nTagBytes > 0);
            int64 nTerminatingBytes = nTagBytes;
            nTerminatingBytes += static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES));

            if (nTerminatingBytes > 0)
            {
                spTempBuffer.Assign(new unsigned char[static_cast<size_t>(nTerminatingBytes)], true);
                if (spTempBuffer == APE_NULL) throw(static_cast<intn>(ERROR_INSUFFICIENT_MEMORY));

                THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_DATA, POINTER_TO_INT64(spTempBuffer.GetPtr()), nTerminatingBytes))

                if (bHasTag)
                {
                    unsigned int nBytesRead = 0;
                    THROW_ON_ERROR(GET_IO(spAPEDecompress)->Seek(-(nTagBytes), SeekFileEnd))
                    THROW_ON_ERROR(GET_IO(spAPEDecompress)->Read(&spTempBuffer[spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES)], static_cast<unsigned int>(nTagBytes), &nBytesRead))
                }

                THROW_ON_ERROR(spAPECompress->Finish(spTempBuffer, nTerminatingBytes, static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES))))
            }
            else
            {
                THROW_ON_ERROR(spAPECompress->Finish(APE_NULL, 0, 0))
            }
        }

        // fire the "complete" progress notification
        spMACProgressHelper->UpdateProgressComplete();

        // clean-up
        spAPEDecompress.Delete();
    }
    catch (const intn nErrorCode)
    {
        spAPEDecompress.Delete(); // delete explicitly (to avoid a crash on 2/23/2019 in release)
        nFunctionRetVal = (nErrorCode == 0) ? ERROR_UNDEFINED : static_cast<int>(nErrorCode);
    }
    catch (...)
    {
        nFunctionRetVal = ERROR_UNDEFINED;
    }

    // return
    return nFunctionRetVal;
}
