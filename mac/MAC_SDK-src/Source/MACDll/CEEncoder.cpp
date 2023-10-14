#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#include "Filters.h"
#include "resource.h"
#include "MACLib.h"
#include "CharacterHelper.h"

using namespace APE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The encoder structure (used as output handle)
///////////////////////////////////////////////
struct APE_ENCODER
{
    IAPECompress * pAPECompress;
    BOOL bConvert32to24;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declares to avoid Clang warnings
///////////////////////////////////////////////
void SafeDeleteAPEEncoder(APE_ENCODER * pAPEEncoder);
__declspec(dllexport) DWORD FAR PASCAL FilterWriteSpecialData(HANDLE, LPCSTR, LPCSTR, char*, DWORD);
__declspec(dllexport) HANDLE FAR PASCAL OpenFilterOutput(LPSTR lpstrFilename, long lSamprate, WORD wBitsPerSample, WORD wChannels, long lSize, long far* lpChunkSize, DWORD dwOptions);
__declspec(dllexport) DWORD FAR PASCAL WriteFilterOutput(HANDLE hOutput, unsigned char far* buf, long lBytes);
__declspec(dllexport) void FAR PASCAL CloseFilterOutput(HANDLE hOutput);

void SafeDeleteAPEEncoder(APE_ENCODER * pAPEEncoder)
{
    if (pAPEEncoder == NULL) return;

    APE_SAFE_DELETE(pAPEEncoder->pAPECompress)
    APE_SAFE_DELETE(pAPEEncoder)
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterWriteSpecialData:
/////////////////////////
// __declspec(dllexport) DWORD FAR PASCAL FilterWriteSpecialData(HANDLE hOutput, LPCSTR szListType, LPCSTR szType, char* pData, DWORD dwSize)
__declspec(dllexport) DWORD FAR PASCAL FilterWriteSpecialData(HANDLE, LPCSTR, LPCSTR, char *, DWORD)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//OpenFilterOutput: Open the file for writing
/////////////////////////////////////////////
__declspec(dllexport) HANDLE FAR PASCAL OpenFilterOutput(LPSTR lpstrFilename,long lSamprate,
                                            WORD wBitsPerSample,WORD wChannels,long lSize, long far *lpChunkSize, DWORD dwOptions)
{
    APE_ENCODER * pAPEEncoder = new APE_ENCODER;
    pAPEEncoder->bConvert32to24 = FALSE;
    pAPEEncoder->pAPECompress = CreateIAPECompress();

    if (pAPEEncoder->pAPECompress == NULL)
    {
        SafeDeleteAPEEncoder(pAPEEncoder);
        return NULL;
    }

    WAVEFORMATEX wfeAudioFormat;
    APE_CLEAR(wfeAudioFormat);
    double rate = 1;

    if (wBitsPerSample == 32)
    {
        wBitsPerSample = 24;
        rate = 3.0 / 4.0;
        pAPEEncoder->bConvert32to24 = TRUE;
    }

    FillWaveFormatEx(&wfeAudioFormat, WAVE_FORMAT_PCM, lSamprate, wBitsPerSample, wChannels);

    long nCompressLevel = APE_COMPRESSION_LEVEL_NORMAL;
    switch (dwOptions)
    {
        case 1: nCompressLevel = APE_COMPRESSION_LEVEL_FAST; break;
        case 2: nCompressLevel = APE_COMPRESSION_LEVEL_NORMAL; break;
        case 3: nCompressLevel = APE_COMPRESSION_LEVEL_HIGH; break;
        case 4: nCompressLevel = APE_COMPRESSION_LEVEL_EXTRA_HIGH; break;
        case 5: nCompressLevel = APE_COMPRESSION_LEVEL_INSANE; break;
    }

    // we should try to optimize this for speed, because MACLib.lib doesn't care
    *lpChunkSize = 16384 * wfeAudioFormat.nBlockAlign;

    CSmartPtr<wchar_t> spUTF16(CAPECharacterHelper::GetUTF16FromANSI(lpstrFilename), TRUE);
    if (pAPEEncoder->pAPECompress->Start(spUTF16, &wfeAudioFormat, static_cast<int64>(static_cast<double>(lSize) * rate), nCompressLevel, NULL, CREATE_WAV_HEADER_ON_DECOMPRESSION) != 0)
    {
        SafeDeleteAPEEncoder(pAPEEncoder);
        return NULL;
    }

    return static_cast<HANDLE>(pAPEEncoder);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//WriteFilterOutput: Effective Writing
//////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL WriteFilterOutput(HANDLE hOutput, unsigned char far *buf, long lBytes)
{
    APE_ENCODER * pAPEEncoder = static_cast<APE_ENCODER *>(hOutput);
    if (hOutput == NULL) return 0;

    if (pAPEEncoder->bConvert32to24)
    {
        ///////////////////////////////////////////////////////////////////////////////
        //24 bit to 32 bit floating (16.8) decode
        /////////////////////////////////////////
        #define c2_24            16777216.0
        #define c2_23             8388608.0
        long AudioBytesLeft = lBytes*3/4;
        long c=0;

        float far *ibuf;
        ibuf=reinterpret_cast<float far *>(buf);

        unsigned char * pbuf = new unsigned char [static_cast<size_t>(lBytes)+4];
        if (pbuf == NULL) return DWORD(-1);

        for (long v=0; v<lBytes; v+=4)
        {
            double n;
            long m;


            n=static_cast<double>(*ibuf++) * 256.0;
            if (n>+c2_23) n=+c2_23;
            if (n<-c2_23) n=-c2_23+1;

            if (n<0) n+=c2_24;
            m=static_cast<long>(n+.5);

            pbuf[c++]=m&0xff;
            pbuf[c++]=(m>>8)&0xff;
            pbuf[c++]=(m>>16)&0xff;
        }

        if (pAPEEncoder->pAPECompress->AddData(pbuf, AudioBytesLeft) != 0)
            return 0;

        APE_SAFE_ARRAY_DELETE(pbuf)
        lBytes=lBytes*3/4;
    }
    else
    {
        ///////////////////////////////////////////////////////////////////////////////
        //8 and 16 bits encode
        //////////////////////
        if (pAPEEncoder->pAPECompress->AddData(buf, lBytes) != 0)
            return 0;
    }

    return static_cast<DWORD>(lBytes);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CloseFileOutput: Closes the file after writing
///////////////////////////////////////////////
__declspec(dllexport) void FAR PASCAL CloseFilterOutput(HANDLE hOutput)
{
    APE_ENCODER * pAPEEncoder = static_cast<APE_ENCODER *>(hOutput);
    if (hOutput != NULL)
    {
        pAPEEncoder->pAPECompress->Finish(NULL, 0, 0);
        SafeDeleteAPEEncoder(pAPEEncoder);
    }
}
