#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#include "filters.h" 
#include "resource.h"
#include "MACLib.h"
#include "CharacterHelper.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//OpenFilterInput: Open the file for reading
////////////////////////////////////////////
__declspec(dllexport) int FAR PASCAL FilterGetFileSize(HANDLE hInput)
{    
    IAPEDecompress* pAPEDecompress = (IAPEDecompress *) hInput;
    if (hInput == NULL) return 0;
    
    int64 nBytesPerSample = pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE);
    if (nBytesPerSample == 3) nBytesPerSample = 4;
    return int(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TOTAL_BLOCKS) * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * nBytesPerSample);
}

__declspec(dllexport) HANDLE FAR PASCAL OpenFilterInput(LPSTR lpstrFilename, int far * lSamprate, WORD far * wBitsPerSample, WORD far * wChannels, HWND, int far * lChunkSize)
{
    ///////////////////////////////////////////////////////////////////////////////
    // open the APE file
    ///////////////////////////////////////////////////////////////////////////////    
    CSmartPtr<wchar_t> spUTF16(CAPECharacterHelper::GetUTF16FromANSI(lpstrFilename), TRUE);
    IAPEDecompress * pAPEDecompress = CreateIAPEDecompress(spUTF16, NULL, true, true, false);
    if (pAPEDecompress == NULL)
        return NULL;
    
    *lSamprate= (int) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE);
    *wChannels = (WORD) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS);
    *wBitsPerSample = (WORD) pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE);
    
    if (*wBitsPerSample == 24) *wBitsPerSample = 32;
    
    // this doesn't matter (but must be careful to avoid alignment problems)
    *lChunkSize = 16384 * (*wBitsPerSample / 8) * *wChannels;

    return (HANDLE) pAPEDecompress;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ReadFilterInput: Effective Reading
////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL ReadFilterInput(HANDLE hInput, unsigned char far * buf, int lBytes)
{
    IAPEDecompress * pAPEDecompress = (IAPEDecompress *) hInput;
    if (hInput == NULL) return 0;

    int64 nBlocksDecoded = 0;
    int64 nBlocksToDecode = lBytes / pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN);

    if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE) == 3)
    {
        ///////////////////////////////////////////////////////////////////////////////
        // 24 bit decode (convert to 32 bit)
        //////////////////////
        
        nBlocksToDecode = (nBlocksToDecode * 3) / 4;
        if (pAPEDecompress->GetData((char*) buf, nBlocksToDecode, &nBlocksDecoded) != ERROR_SUCCESS)
            return 0;

        // expand to 32 bit
        unsigned char * p24Bit = (unsigned char *) &buf[(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BLOCK_ALIGN)) - 3];
        float * p32Bit = (float *) &buf[(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * 4) - 4];

        while (p32Bit >= (float *) &buf[0])
        {
            float fValue = (float) (*p24Bit + *(p24Bit + 1) * 256 + *(p24Bit + 2) * 65536) / 256;
            if (fValue > 32768) fValue -= 65536;
            *p32Bit = fValue;

            p24Bit -= 3;
            p32Bit--;
        }
    }
    else
    {
        ///////////////////////////////////////////////////////////////////////////////
        // 8 and 16 bits decode
        //////////////////////
        if (pAPEDecompress->GetData((char *) buf, nBlocksToDecode, &nBlocksDecoded) != ERROR_SUCCESS)
            return 0;
    }

    int64 BytesPerSample = pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE);
    if (BytesPerSample == 3) BytesPerSample = 4;
    return DWORD(nBlocksDecoded * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * BytesPerSample);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CloseFileInput: Closes the file after reading
///////////////////////////////////////////////
__declspec(dllexport) void FAR PASCAL CloseFilterInput(HANDLE hInput)
{
    IAPEDecompress * pAPEDecompress = (IAPEDecompress *) hInput;
    if (pAPEDecompress != NULL) 
    {
        delete pAPEDecompress;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterOptions: Returns the compression level for this wave
////////////////////////////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterOptions(HANDLE hInput)
{ 
    int nCompressionLevel = 2;

    IAPEDecompress * pAPEDecompress = (IAPEDecompress *) hInput;
    if (pAPEDecompress != NULL) 
    {
        switch (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL))
        {
        case MAC_COMPRESSION_LEVEL_FAST: nCompressionLevel = 1; break;
        case MAC_COMPRESSION_LEVEL_NORMAL: nCompressionLevel = 2; break;
        case MAC_COMPRESSION_LEVEL_HIGH: nCompressionLevel = 3; break;
        case MAC_COMPRESSION_LEVEL_EXTRA_HIGH: nCompressionLevel = 4; break;
        case MAC_COMPRESSION_LEVEL_INSANE: nCompressionLevel = 5; break;
        }
    }
    
    return nCompressionLevel;
}        

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterOptionsString: Fills compression level in "File Info"
/////////////////////////////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterOptionsString(HANDLE hInput, LPSTR szString)
{ 
    // the safe buffer size was found to be around 120 with the Cool Edit Pro 2 installed on my computer
    // so that's the copy limit size -- it should be way more than we ever put in the buffer anyway
    
    // default
    strncpy_s(szString, 120, "Compression Level: Normal", _TRUNCATE);

    // fill in from decoder
    IAPEDecompress * pAPEDecompress = (IAPEDecompress *) hInput;
    if (pAPEDecompress != NULL) 
    {
        char Title[256] = { 0 }; 
        strcpy_s(Title, 256, "Compression Level: ");
        
        switch (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL))
        {
        case MAC_COMPRESSION_LEVEL_FAST: strcat_s(Title, 256, "Fast"); break;
        case MAC_COMPRESSION_LEVEL_NORMAL: strcat_s(Title, 256, "Normal"); break;
        case MAC_COMPRESSION_LEVEL_HIGH: strcat_s(Title, 256, "High"); break;
        case MAC_COMPRESSION_LEVEL_EXTRA_HIGH: strcat_s(Title, 256, "Extra High"); break;
        case MAC_COMPRESSION_LEVEL_INSANE: strcat_s(Title, 256, "Insane"); break;
        }

        strncpy_s(szString, 120, Title, _TRUNCATE);
    }

    // return compression level
    return FilterOptions(hInput);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterGetFirstSpecialData:
////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterGetFirstSpecialData(HANDLE, SPECIALDATA *)
{    
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterGetNextSpecialData:
///////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterGetNextSpecialData(HANDLE, SPECIALDATA *)
{    
    return 0;
}

