#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#include "Filters.h"
#include "resource.h"
#include "MACLib.h"
#include "CharacterHelper.h"
#include "APEInfo.h"

using namespace APE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declare to prevent Clang warnings
////////////////////////////////////////////
__declspec(dllexport) int FAR PASCAL FilterGetFileSize(HANDLE hInput);
__declspec(dllexport) HANDLE FAR PASCAL OpenFilterInput(LPSTR lpstrFilename, int far * lSamprate, WORD far * wBitsPerSample, WORD far * wChannels, HWND, int far * lChunkSize);
__declspec(dllexport) DWORD FAR PASCAL ReadFilterInput(HANDLE hInput, unsigned char far * buf, int lBytes);
__declspec(dllexport) void FAR PASCAL CloseFilterInput(HANDLE hInput);
__declspec(dllexport) DWORD FAR PASCAL FilterOptions(HANDLE hInput);
__declspec(dllexport) DWORD FAR PASCAL FilterOptionsString(HANDLE hInput, LPSTR szString);
__declspec(dllexport) DWORD FAR PASCAL FilterGetFirstSpecialData(HANDLE, SPECIALDATA *);
__declspec(dllexport) DWORD FAR PASCAL FilterGetNextSpecialData(HANDLE, SPECIALDATA *);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OpenFilterInput: Open the file for reading
////////////////////////////////////////////
__declspec(dllexport) int FAR PASCAL FilterGetFileSize(HANDLE hInput)
{
    IAPEDecompress* pAPEDecompress = static_cast<IAPEDecompress *>(hInput);
    if (hInput == APE_NULL) return 0;

    int64 nBytesPerSample = pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE);
    if (nBytesPerSample == 3) nBytesPerSample = 4;
    return static_cast<int>(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TOTAL_BLOCKS) * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * nBytesPerSample);
}

__declspec(dllexport) HANDLE FAR PASCAL OpenFilterInput(LPSTR lpstrFilename, int far * lSamprate, WORD far * wBitsPerSample, WORD far * wChannels, HWND, int far * lChunkSize)
{

    ///////////////////////////////////////////////////////////////////////////////
    // open the APE file
    ///////////////////////////////////////////////////////////////////////////////
    CSmartPtr<wchar_t> spUTF16(CAPECharacterHelper::GetUTF16FromANSI(lpstrFilename), TRUE);
    IAPEDecompress * pAPEDecompress = CreateIAPEDecompress(spUTF16, APE_NULL, true, true, false);
    if (pAPEDecompress == APE_NULL)
        return APE_NULL;

    *lSamprate= static_cast<int>(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE));
    *wChannels = static_cast<WORD>(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS));
    *wBitsPerSample = static_cast<WORD>(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE));

    if (*wBitsPerSample == 24) *wBitsPerSample = 32;

    // this doesn't matter (but must be careful to avoid alignment problems)
    *lChunkSize = 16384 * (*wBitsPerSample / 8) * *wChannels;

    return static_cast<HANDLE>(pAPEDecompress);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReadFilterInput: Effective Reading
////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL ReadFilterInput(HANDLE hInput, unsigned char far * buf, int lBytes)
{
    IAPEDecompress * pAPEDecompress = static_cast<IAPEDecompress *>(hInput);
    if (hInput == APE_NULL) return 0;

    int64 nBlocksDecoded = 0;
    int64 nBlocksToDecode = lBytes / pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN);

    if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE) == 3)
    {
        ///////////////////////////////////////////////////////////////////////////////
        // 24 bit decode (convert to 32 bit in Cool Edit format)
        //////////////////////

        nBlocksToDecode = (nBlocksToDecode * 3) / 4;
        if (pAPEDecompress->GetData(reinterpret_cast<unsigned char *>(buf), nBlocksToDecode, &nBlocksDecoded) != ERROR_SUCCESS)
            return 0;

        // expand to 32 bit
        unsigned char * p24Bit = static_cast<unsigned char *>(&buf[(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN)) - 3]);
        float * p32Bit = reinterpret_cast<float *>(&buf[(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * 4) - 4]);

        const float fDivisor = 65536.0f; // 2147483648.0f / 32768.0f;
        while (p32Bit >= reinterpret_cast<float *>(&buf[0]))
        {
            int nValue = static_cast<int>((p24Bit[0] << 8) | (p24Bit[1] << 16) | (p24Bit[2] << 24)); // get the integer value (this keeps the sign by shifting 24)
            float fValue = static_cast<float>(nValue) / fDivisor; // convert to Cool Edit format (the steps are listed below commented out, but it's faster to do the single divide)
            //float fValue = float(nValue) / 2147483648.0f; // convert to -1 to 1 float
            //fValue *= 32768; // scale back to the Cool Edit format
            *p32Bit = fValue; // output

            p24Bit -= 3;
            p32Bit--;
        }
    }
    else if ((pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE) == 4) && (pAPEDecompress->GetInfo(APE::IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_FLOATING_POINT))
    {
        ///////////////////////////////////////////////////////////////////////////////
        // 32 bit float decode (convert to 32 bit in Cool Edit format)
        //////////////////////

        IAPEDecompress::APE_GET_DATA_PROCESSING GetDataProcessing = { true, false, false };
        if (pAPEDecompress->GetData(reinterpret_cast<unsigned char*>(buf), nBlocksToDecode, &nBlocksDecoded, &GetDataProcessing) != ERROR_SUCCESS)
            return 0;

        // expand to 32 bit
        float* p32Bit = reinterpret_cast<float*>(&buf[(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * 4) - 4]);
        float* p32BitFloat = reinterpret_cast<float*>(p32Bit);

        const float fDivisor = float(256 * 128);
        while (p32Bit >= reinterpret_cast<float*>(&buf[0]))
        {
            float fValue = static_cast<float>(float(*p32BitFloat) * fDivisor);
            *p32Bit = fValue;

            p32BitFloat--;
            p32Bit--;
        }
    }
    else if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE) == 4)
    {
        ///////////////////////////////////////////////////////////////////////////////
        // 32 bit decode (convert to 32 bit in Cool Edit format)
        //////////////////////

        if (pAPEDecompress->GetData(reinterpret_cast<unsigned char *>(buf), nBlocksToDecode, &nBlocksDecoded) != ERROR_SUCCESS)
            return 0;

        // expand to 32 bit
        float * p32Bit = reinterpret_cast<float *>(&buf[(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * 4) - 4]);
        int * p32BitInt = reinterpret_cast<int *>(p32Bit);

        const float fDivisor = float(256 * 256);
        while (p32Bit >= reinterpret_cast<float *>(&buf[0]))
        {
            float fValue = static_cast<float>(float(*p32BitInt) / fDivisor);
            *p32Bit = fValue;

            p32BitInt--;
            p32Bit--;
        }
    }
    else
    {
        ///////////////////////////////////////////////////////////////////////////////
        // 8 and 16 bits decode
        //////////////////////
        if (pAPEDecompress->GetData(reinterpret_cast<unsigned char *>(buf), nBlocksToDecode, &nBlocksDecoded) != ERROR_SUCCESS)
            return 0;
    }

    int64 BytesPerSample = pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE);
    if (BytesPerSample == 3) BytesPerSample = 4;
    return DWORD(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * BytesPerSample);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CloseFileInput: Closes the file after reading
///////////////////////////////////////////////
__declspec(dllexport) void FAR PASCAL CloseFilterInput(HANDLE hInput)
{
    IAPEDecompress * pAPEDecompress = static_cast<IAPEDecompress *>(hInput);
    if (pAPEDecompress != APE_NULL)
    {
        delete pAPEDecompress;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FilterOptions: Returns the compression level for this wave
////////////////////////////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterOptions(HANDLE hInput)
{
    int nCompressionLevel = 2;

    IAPEDecompress * pAPEDecompress = static_cast<IAPEDecompress *>(hInput);
    if (pAPEDecompress != APE_NULL)
    {
        switch (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL))
        {
        case APE_COMPRESSION_LEVEL_FAST: nCompressionLevel = 1; break;
        case APE_COMPRESSION_LEVEL_NORMAL: nCompressionLevel = 2; break;
        case APE_COMPRESSION_LEVEL_HIGH: nCompressionLevel = 3; break;
        case APE_COMPRESSION_LEVEL_EXTRA_HIGH: nCompressionLevel = 4; break;
        case APE_COMPRESSION_LEVEL_INSANE: nCompressionLevel = 5; break;
        }
    }

    return static_cast<DWORD>(nCompressionLevel);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FilterOptionsString: Fills compression level in "File Info"
/////////////////////////////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterOptionsString(HANDLE hInput, LPSTR szString)
{
    // the safe buffer size was found to be around 120 with the Cool Edit Pro 2 installed on my computer
    // so that's the copy limit size -- it should be way more than we ever put in the buffer anyway

    // default
    strncpy_s(szString, 120, "Compression Level: Normal", _TRUNCATE);

    // fill in from decoder
    IAPEDecompress * pAPEDecompress = static_cast<IAPEDecompress *>(hInput);
    if (pAPEDecompress != APE_NULL)
    {
        char Title[256];
        APE_CLEAR(Title);
        strcpy_s(Title, 256, "Compression Level: ");

        str_utfn cCompressionLevel[16]; APE_CLEAR(cCompressionLevel);
        GetAPECompressionLevelName(static_cast<int>(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL)), cCompressionLevel, 16, true);
        str_ansi * pCompressionLevelANSI = CAPECharacterHelper::GetANSIFromUTF16(cCompressionLevel);
        strcat_s(Title, 256, pCompressionLevelANSI);

        strncpy_s(szString, 120, Title, _TRUNCATE);

        delete [] pCompressionLevelANSI;
    }

    // return compression level
    return FilterOptions(hInput);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FilterGetFirstSpecialData:
////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterGetFirstSpecialData(HANDLE, SPECIALDATA *)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FilterGetNextSpecialData:
///////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterGetNextSpecialData(HANDLE, SPECIALDATA *)
{
    return 0;
}
