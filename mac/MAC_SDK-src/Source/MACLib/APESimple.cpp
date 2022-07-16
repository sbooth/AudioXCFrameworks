#include "All.h"
#include "APEInfo.h"
#include "APECompress.h"
#include "APEDecompress.h"
#include "WAVInputSource.h"
#include "IO.h"
#include "MACProgressHelper.h"
#include "GlobalFunctions.h"
#include "MD5.h"
#include "CharacterHelper.h"
using namespace APE;

#define UNMAC_DECODER_OUTPUT_NONE       0
#define UNMAC_DECODER_OUTPUT_WAV        1
#define UNMAC_DECODER_OUTPUT_APE        2

#define BLOCKS_PER_DECODE               9216

int DecompressCore(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nOutputMode, int nCompressionLevel, IAPEProgressCallback * pProgressCallback);

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

    virtual void Progress(int nPercentageDone)
    {
        if (m_pProgress != NULL)
            *m_pProgress = nPercentageDone;

        if (m_ProgressCallback != NULL)
            m_ProgressCallback(nPercentageDone);
    }

    virtual int GetKillFlag()
    {
        return (m_pKillFlag == NULL) ? KILL_FLAG_CONTINUE : *m_pKillFlag;
    }

    virtual void SetFileType(const APE::str_ansi * pType)
    {
        strcpy_s(m_cType, 8, pType);
    }

    str_ansi * GetFileType() { return m_cType; }

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

int __stdcall DecompressFile(const APE::str_ansi * pInputFilename, const APE::str_ansi * pOutputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag, APE::str_ansi cFileType[5])
{
    CSmartPtr<str_utfn> spInputFile(CAPECharacterHelper::GetUTF16FromANSI(pInputFilename), true);
    CSmartPtr<str_utfn> spOutputFile(CAPECharacterHelper::GetUTF16FromANSI(pOutputFilename), true);
    return DecompressFileW(spInputFile, pOutputFilename ? spOutputFile : NULL, pPercentageDone, ProgressCallback, pKillFlag, cFileType);
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

int __stdcall DecompressFileW(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int * pPercentageDone, APE_PROGRESS_CALLBACK ProgressCallback, int * pKillFlag, APE::str_ansi cFileType[5])
{
    CAPEProgressCallbackLegacy ProgressCallbackLegacy(pPercentageDone, ProgressCallback, pKillFlag);
    int nResult = DecompressFileW2(pInputFilename, pOutputFilename, &ProgressCallbackLegacy);
    if (ProgressCallbackLegacy.GetFileType()[0] != 0)
        strcpy_s(&cFileType[0], 5, ProgressCallbackLegacy.GetFileType());
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
    APE::WAVEFORMATEX WaveFormatEx;
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
        
        if ((spInputSource == NULL) || (nResult != ERROR_SUCCESS))
            throw intn(nResult);

        // create the compressor
        spAPECompress.Assign(CreateIAPECompress());
        if (spAPECompress == NULL) throw intn(ERROR_UNDEFINED);

        // figure the audio bytes
        int64 nAudioBytes = int64(nAudioBlocks) * int64(WaveFormatEx.nBlockAlign);
        if (spInputSource->GetUnknownLengthPipe())
            nAudioBytes = MAX_AUDIO_BYTES_UNKNOWN;
        if ((nAudioBytes <= 0) && (nAudioBytes != MAX_AUDIO_BYTES_UNKNOWN))
            return ERROR_INPUT_FILE_TOO_SMALL;

        // start the encoder
        if (nHeaderBytes > 0) spBuffer.Assign(new unsigned char [uint32(nHeaderBytes)], true);
        THROW_ON_ERROR(spInputSource->GetHeaderData(spBuffer.GetPtr()))
        THROW_ON_ERROR(spAPECompress->Start(pOutputFilename, &WaveFormatEx, nAudioBytes,
            nCompressionLevel, spBuffer.GetPtr(), nHeaderBytes, nFlags));
        spBuffer.Delete();

        // set-up the progress
        spMACProgressHelper.Assign(new CMACProgressHelper(nAudioBytes, pProgressCallback));

        // master loop
        int64 nBytesLeft = nAudioBytes;
        while ((nBytesLeft > 0) || spInputSource->GetUnknownLengthPipe())
        {
            int64 nBytesAdded = 0;
            if (spInputSource->GetUnknownLengthPipe())
            {
                int64 nRetVal = spAPECompress->AddDataFromInputSource(spInputSource.GetPtr(), nBytesLeft, &nBytesAdded);
                if (nRetVal == ERROR_IO_READ)
                    break; // this means we reached the end of the file
                else if (nRetVal != ERROR_SUCCESS)
                    throw(intn(nRetVal));
            }
            else
            {
                THROW_ON_ERROR(spAPECompress->AddDataFromInputSource(spInputSource.GetPtr(), nBytesLeft, &nBytesAdded))
            }
            nBytesLeft -= nBytesAdded;

            // update the progress
            if (nAudioBytes != -1)
                spMACProgressHelper->UpdateProgress(nAudioBytes - nBytesLeft);

            // process the kill flag
            if (spMACProgressHelper->ProcessKillFlag(true) != ERROR_SUCCESS)
               throw(intn(ERROR_USER_STOPPED_PROCESSING));
        }

        // finalize the file
        if (nTerminatingBytes > 0)
        {
            spBuffer.Assign(new unsigned char[uint32(nTerminatingBytes)], true);
            THROW_ON_ERROR(spInputSource->GetTerminatingData(spBuffer.GetPtr()));
        }
        THROW_ON_ERROR(spAPECompress->Finish(spBuffer.GetPtr(), nTerminatingBytes, nTerminatingBytes))

        // update the progress to 100%
        spMACProgressHelper->UpdateProgressComplete();
    }
    catch(intn nErrorCode)
    {
        nFunctionRetVal = (nErrorCode == 0) ? ERROR_UNDEFINED : int(nErrorCode);
    }
    catch(...)
    {
        nFunctionRetVal = ERROR_UNDEFINED;
    }
    
    // kill the compressor if we failed
    if ((nFunctionRetVal != 0) && (spAPECompress != NULL))
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
    if (pInputFilename == NULL)
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
            
            spAPEDecompress.Assign(CreateIAPEDecompress(pInputFilename, &nFunctionRetVal, true, false, true));
            if (spAPEDecompress == NULL || nFunctionRetVal != ERROR_SUCCESS) throw(intn(nFunctionRetVal));

            APE_FILE_INFO * pInfo = (APE_FILE_INFO *) spAPEDecompress->GetInfo(IAPEDecompress::APE_INTERNAL_INFO);
            
            // check version
            if ((pInfo->nVersion < 3980) || (pInfo->spAPEDescriptor == NULL))
                throw(intn(ERROR_UNSUPPORTED_FILE_VERSION));

            // make sure the MD5 is valid
            if (pInfo->nMD5Invalid)
                throw(intn(ERROR_UNSUPPORTED_FILE_VERSION));
        }
        catch(...)
        {
            bQuickVerifyIfPossible = false;
        }
    }

    // if we can and should quick verify, then do it
    if (bQuickVerifyIfPossible)
    {
        // variable declares
        int nFunctionRetVal = ERROR_SUCCESS;
        unsigned int nBytesRead = 0;

        // run the quick verify
        try
        {
            CMD5Helper MD5Helper;
            
            CIO * pIO = GET_IO(spAPEDecompress);
            APE_FILE_INFO * pInfo = (APE_FILE_INFO *) spAPEDecompress->GetInfo(IAPEDecompress::APE_INTERNAL_INFO);

            if ((pInfo->nVersion < 3980) || (pInfo->spAPEDescriptor == NULL))
                throw(intn(ERROR_UNSUPPORTED_FILE_VERSION));

            // read APE header
            CSmartPtr<unsigned char> spAPEHeader(new unsigned char[pInfo->spAPEDescriptor->nHeaderBytes], true);
            pIO->SetSeekMethod(APE_FILE_BEGIN);
            pIO->SetSeekPosition(pInfo->nJunkHeaderBytes + pInfo->spAPEDescriptor->nDescriptorBytes);
            pIO->PerformSeek();
            pIO->Read(spAPEHeader, pInfo->spAPEDescriptor->nHeaderBytes, &nBytesRead);

            // read seek table
            CSmartPtr<unsigned char> spSeekTable(new unsigned char[pInfo->spAPEDescriptor->nSeekTableBytes], true);
            ASSERT(pIO->GetPosition() == (pInfo->nJunkHeaderBytes + pInfo->spAPEDescriptor->nDescriptorBytes + pInfo->spAPEDescriptor->nHeaderBytes));
            pIO->Read(spSeekTable, pInfo->spAPEDescriptor->nSeekTableBytes, &nBytesRead);

            // read header data
            CSmartPtr<unsigned char> spHeader(new unsigned char[pInfo->spAPEDescriptor->nHeaderDataBytes], true);
            ASSERT(pIO->GetPosition() == (pInfo->nJunkHeaderBytes + pInfo->spAPEDescriptor->nDescriptorBytes + pInfo->spAPEDescriptor->nHeaderBytes + pInfo->spAPEDescriptor->nSeekTableBytes));
            pIO->Read(spHeader, (unsigned int)pInfo->spAPEDescriptor->nHeaderDataBytes, &nBytesRead);

            // seek to the data (we should already be there)
            ASSERT(pIO->GetPosition() == (pInfo->nJunkHeaderBytes + pInfo->spAPEDescriptor->nDescriptorBytes + pInfo->spAPEDescriptor->nHeaderBytes + pInfo->spAPEDescriptor->nSeekTableBytes + pInfo->spAPEDescriptor->nHeaderDataBytes));

            // add the WAV header to the MD5 first
            MD5Helper.AddData(spHeader, pInfo->spAPEDescriptor->nHeaderDataBytes);

            // bytes left
            int64 nBytesLeft = (int64(pInfo->spAPEDescriptor->nAPEFrameDataBytesHigh) << 32) + int64(pInfo->spAPEDescriptor->nAPEFrameDataBytes) + int64(pInfo->spAPEDescriptor->nTerminatingDataBytes);
            int64 nBytesLeftOriginal = nBytesLeft;

            // read in smaller chunks
            CSmartPtr<CMACProgressHelper> spMACProgressHelper;
            spMACProgressHelper.Assign(new CMACProgressHelper(nBytesLeft, pProgressCallback));

            CSmartPtr<unsigned char> spBuffer(new unsigned char[16384], true);
            nBytesRead = 1;
            while ((nBytesLeft > 0) && (nBytesRead > 0))
            {
                unsigned int nBytesToRead = (unsigned int) ape_min(16384, nBytesLeft);
                if (pIO->Read(spBuffer, nBytesToRead, &nBytesRead) != ERROR_SUCCESS)
                    throw(intn(ERROR_IO_READ));

                MD5Helper.AddData(spBuffer, nBytesRead);
                spMACProgressHelper->UpdateProgress(nBytesLeftOriginal - nBytesLeft);
                
                nBytesLeft -= nBytesRead;

                if (spMACProgressHelper->ProcessKillFlag() != ERROR_SUCCESS)
                    throw(intn(ERROR_USER_STOPPED_PROCESSING));
            }

            if (nBytesLeft != 0)
                throw(intn(ERROR_IO_READ));

            // add the header and seek table
            MD5Helper.AddData(spAPEHeader, pInfo->spAPEDescriptor->nHeaderBytes);
            MD5Helper.AddData(spSeekTable, pInfo->spAPEDescriptor->nSeekTableBytes);

            // get results
            unsigned char cResult[16];
            MD5Helper.GetResult(cResult);

            // compare to stored
            if (memcmp(cResult, pInfo->spAPEDescriptor->cFileMD5, 16) != 0)
                nFunctionRetVal = ERROR_INVALID_CHECKSUM;

            // update the progress to 100%
            spMACProgressHelper->UpdateProgressComplete();
        }
        catch(intn nErrorCode)
        {
            nFunctionRetVal = (nErrorCode == 0) ? ERROR_UNDEFINED : int(nErrorCode);
        }
        catch(...)
        {
            nFunctionRetVal = ERROR_UNDEFINED;
        }
        
        // return value
        nResult = nFunctionRetVal;
    }
    else
    {
        nResult = DecompressCore(pInputFilename, NULL, UNMAC_DECODER_OUTPUT_NONE, -1, pProgressCallback);
    }

    return nResult;
}

/**************************************************************************************************
Decompress file
**************************************************************************************************/
int __stdcall DecompressFileW2(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, IAPEProgressCallback * pProgressCallback)
{
    if (pOutputFilename == NULL)
        return VerifyFileW2(pInputFilename, pProgressCallback);
    else
        return DecompressCore(pInputFilename, pOutputFilename, UNMAC_DECODER_OUTPUT_WAV, -1, pProgressCallback);
}

/**************************************************************************************************
Convert file
**************************************************************************************************/
int __stdcall ConvertFileW2(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nCompressionLevel, IAPEProgressCallback * pProgressCallback) 
{
    return DecompressCore(pInputFilename, pOutputFilename, UNMAC_DECODER_OUTPUT_APE, nCompressionLevel, pProgressCallback);
}

/**************************************************************************************************
Decompress a file using the specified output method
**************************************************************************************************/
int DecompressCore(const APE::str_utfn * pInputFilename, const APE::str_utfn * pOutputFilename, int nOutputMode, int nCompressionLevel, IAPEProgressCallback * pProgressCallback) 
{
    // error check the function parameters
    if (pInputFilename == NULL) 
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
    APE::WAVEFORMATEX wfeInput;

    try
    {
        // create the decoder
        spAPEDecompress.Assign(CreateIAPEDecompress(pInputFilename, &nFunctionRetVal, true, true, false));
        if (spAPEDecompress == NULL || nFunctionRetVal != ERROR_SUCCESS) throw(intn(nFunctionRetVal));

        // get the input format
        THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAVEFORMATEX, (int64) &wfeInput))

        // customize the type if we're AIFF
        if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_AIFF)
        {
            pProgressCallback->SetFileType("aiff");
        }
        else if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_W64)
        {
            pProgressCallback->SetFileType("w64");
        }
        else if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_SND)
        {
            pProgressCallback->SetFileType("snd");
        }
        else if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_CAF)
        {
            pProgressCallback->SetFileType("caf");
        }

        // allocate space for the header
        spTempBuffer.Assign(new unsigned char [size_t(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES))], true);
        if (spTempBuffer == NULL) throw(intn(ERROR_INSUFFICIENT_MEMORY));

        // get the header
        int64 nHeaderBytes = spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES);
        THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_DATA, (int64) spTempBuffer.GetPtr(), nHeaderBytes));

        // initialize the output
        if (nOutputMode == UNMAC_DECODER_OUTPUT_WAV)
        {
            // create the file
            spioOutput.Assign(CreateCIO()); THROW_ON_ERROR(spioOutput->Create(pOutputFilename))
        
            // output the header
            THROW_ON_ERROR(WriteSafe(spioOutput, spTempBuffer, (intn) spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES)));
        }
#ifdef APE_SUPPORT_COMPRESS
        else if (nOutputMode == UNMAC_DECODER_OUTPUT_APE)
        {
            // quit if there is nothing to do
            // no longer check when processing an APL since converting an APL to APE should work no matter what
            if ((spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) == MAC_FILE_VERSION_NUMBER) &&
                (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL) == nCompressionLevel) &&
                (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APL) == false))
            {
                throw(intn(ERROR_SKIPPED));
            }

            // create and start the compressor
            spAPECompress.Assign(CreateIAPECompress());
            THROW_ON_ERROR(spAPECompress->Start(pOutputFilename, &wfeInput, (spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS) * spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN)),
                nCompressionLevel, spTempBuffer, spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_HEADER_BYTES)))
        }
#endif

        // allocate space for decompression
        spTempBuffer.Assign(new unsigned char [size_t(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN)) * BLOCKS_PER_DECODE], true);
        if (spTempBuffer == NULL) throw(intn(ERROR_INSUFFICIENT_MEMORY));

        int64 nBlocksLeft = intn(spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS));
        
        // create the progress helper
        spMACProgressHelper.Assign(new CMACProgressHelper((int64) (nBlocksLeft / BLOCKS_PER_DECODE), pProgressCallback));

        // main decoding loop
        while (nBlocksLeft > 0)
        {
            // decode data
            int64 nBlocksDecoded = -1;
            int nResult = spAPEDecompress->GetData((char *) spTempBuffer.GetPtr(), BLOCKS_PER_DECODE, &nBlocksDecoded);
            if (nResult != ERROR_SUCCESS) 
                throw(intn(ERROR_INVALID_CHECKSUM));

            // handle the output
            if (nOutputMode == UNMAC_DECODER_OUTPUT_WAV)
            {
                if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_BIG_ENDIAN)
                {
                    int64 nChannels = spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS);
                    int64 nBitdepth = spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE);
                    if (nBitdepth == 16)
                    {
                        for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
                        {
                            unsigned char cTemp = spTempBuffer[(nChannels * nSample) + 0];
                            spTempBuffer[(nChannels * nSample) + 0] = spTempBuffer[(nChannels * nSample) + 1];
                            spTempBuffer[(nChannels * nSample) + 1] = cTemp;
                        }
                    }
                    else if (nBitdepth == 24)
                    {
                        for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
                        {
                            unsigned char cTemp = spTempBuffer[(3 * nSample) + 0];
                            spTempBuffer[(3 * nSample) + 0] = spTempBuffer[(3 * nSample) + 2];
                            spTempBuffer[(3 * nSample) + 2] = cTemp;
                        }
                    }
                    else if (nBitdepth == 32)
                    {
                        for (int nSample = 0; nSample < nBlocksDecoded * nChannels; nSample++)
                        {
                            uint32 nValue = *((uint32 *) &spTempBuffer[(4 * nSample) + 0]);
                            uint32 nFlippedValue = (((nValue >> 0) & 0xFF) << 24) | (((nValue >> 8) & 0xFF) << 16) | (((nValue >> 16) & 0xFF) << 8) | (((nValue >> 24) & 0xFF) << 0);
                            *((uint32 *) &spTempBuffer[(4 * nSample) + 0]) = nFlippedValue;
                        }
                    }
                }

                unsigned int nBytesToWrite = (unsigned int) (nBlocksDecoded * spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN));
                unsigned int nBytesWritten = 0;
                int nWriteResult = spioOutput->Write(spTempBuffer, nBytesToWrite, &nBytesWritten);
                if ((nWriteResult != 0) || (nBytesToWrite != nBytesWritten))
                    throw(intn(ERROR_IO_WRITE));
            }
            else if (nOutputMode == UNMAC_DECODER_OUTPUT_APE)
            {
                THROW_ON_ERROR(spAPECompress->AddData(spTempBuffer, (nBlocksDecoded * spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN))))
            }

            // update amount remaining
            nBlocksLeft -= nBlocksDecoded;
        
            // update progress and kill flag
            spMACProgressHelper->UpdateProgress();
            if (spMACProgressHelper->ProcessKillFlag(true) != ERROR_SUCCESS)
                throw(intn(ERROR_USER_STOPPED_PROCESSING));
        }

        // terminate the output
        if (nOutputMode == UNMAC_DECODER_OUTPUT_WAV)
        {
            // write any terminating WAV data
            if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES) > 0)
            {
                spTempBuffer.Assign(new unsigned char[intn(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES))], true);
                if (spTempBuffer == NULL) throw(intn(ERROR_INSUFFICIENT_MEMORY));
                THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_DATA, (int64) spTempBuffer.GetPtr(), spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES)))
        
                unsigned int nBytesToWrite = (unsigned int) spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES);
                unsigned int nBytesWritten = 0;
                int nResult = spioOutput->Write(spTempBuffer, nBytesToWrite, &nBytesWritten);
                if ((nResult != 0) || (nBytesToWrite != nBytesWritten)) 
                    throw(intn(ERROR_IO_WRITE));
            }
        }
        else if (nOutputMode == UNMAC_DECODER_OUTPUT_APE)
        {
            // write the WAV data and any tag
            int64 nTagBytes = GET_TAG(spAPEDecompress)->GetTagBytes();
            bool bHasTag = (nTagBytes > 0);
            int64 nTerminatingBytes = nTagBytes;
            nTerminatingBytes += (int) spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES);

            if (nTerminatingBytes > 0) 
            {
                spTempBuffer.Assign(new unsigned char[size_t(nTerminatingBytes)], true);
                if (spTempBuffer == NULL) throw(intn(ERROR_INSUFFICIENT_MEMORY));
                
                THROW_ON_ERROR(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_DATA, (int64) spTempBuffer.GetPtr(), nTerminatingBytes))

                if (bHasTag)
                {
                    unsigned int nBytesRead = 0;
                    GET_IO(spAPEDecompress)->SetSeekMethod(APE_FILE_END);
                    GET_IO(spAPEDecompress)->SetSeekPosition(-(nTagBytes));
                    THROW_ON_ERROR(GET_IO(spAPEDecompress)->PerformSeek())
                    THROW_ON_ERROR(GET_IO(spAPEDecompress)->Read(&spTempBuffer[spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES)], (unsigned int) nTagBytes, &nBytesRead))
                }

                THROW_ON_ERROR(spAPECompress->Finish(spTempBuffer, nTerminatingBytes, (int) spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TERMINATING_BYTES)));
            }
            else 
            {
                THROW_ON_ERROR(spAPECompress->Finish(NULL, 0, 0));
            }
        }

        // fire the "complete" progress notification
        spMACProgressHelper->UpdateProgressComplete();

        // clean-up
        spAPEDecompress.Delete();
    }
    catch(intn nErrorCode)
    {
        spAPEDecompress.Delete(); // delete explicitly (to avoid a crash on 2/23/2019 in release)
        nFunctionRetVal = (nErrorCode == 0) ? ERROR_UNDEFINED : int(nErrorCode);
    }
    catch(...)
    {
        nFunctionRetVal = ERROR_UNDEFINED;
    }
    
    // return
    return nFunctionRetVal;
}
