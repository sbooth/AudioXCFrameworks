#include "stdafx.h"
#include "resource.h"
#include "MACDll.h"

using namespace APE;
#include "IO.h"

#include "APEDecompress.h"
#include "APECompressCreate.h"
#include "APECompressCore.h"
#include "APECompress.h"
#include "APEInfo.h"
#include "APETag.h"
#include "CharacterHelper.h"

int __stdcall GetVersionNumber()
{
    return APE_FILE_VERSION_NUMBER;
}

#ifdef PLATFORM_WINDOWS
int __stdcall GetInterfaceCompatibility(int nVersion, BOOL bDisplayWarningsOnFailure, HWND hwndParent)
{
    int nRetVal = ERROR_SUCCESS;
    if (nVersion > APE_FILE_VERSION_NUMBER)
    {
        nRetVal = ERROR_UNDEFINED;
        if (bDisplayWarningsOnFailure)
        {
            TCHAR cMessage[1024];
            _stprintf_s(cMessage, 1024, _T("You system does not have a new enough version of Monkey's Audio installed.\n")
                _T("Please visit www.monkeysaudio.com for the latest version.\n\n(version %.2f or later required)"),
                static_cast<double>(nVersion) / static_cast<double>(1000));
            MessageBox(hwndParent, cMessage, _T("Please Update Monkey's Audio"), MB_OK | MB_ICONINFORMATION);
        }
    }
    else if (nVersion < 3940)
    {
        nRetVal = ERROR_UNDEFINED;
        if (bDisplayWarningsOnFailure)
        {
            TCHAR cMessage[1024];
            _stprintf_s(cMessage, 1024, _T("This program is trying to use an old version of Monkey's Audio.\n")
                _T("Please contact the author about updating their support for Monkey's Audio.\n\n")
                _T("Monkey's Audio currently installed: %.2f\nProgram is searching for: %.2f"),
                static_cast<double>(APE_FILE_VERSION_NUMBER) / static_cast<double>(1000), static_cast<double>(nVersion) / static_cast<double>(1000));
            MessageBox(hwndParent, cMessage, _T("Program Requires Updating"), MB_OK | MB_ICONINFORMATION);
        }
    }

    return nRetVal;
}
#endif

const char * __stdcall GetLibraryVersionString()
{
    return APE_VER_FILE_VERSION_STR_NARROW;
}

unsigned int __stdcall GetLibraryVersionNumber()
{
    return APE_VERSION_MAJOR << 16 | APE_VERSION_REVISION_NUMBER << 8;
}

unsigned int __stdcall GetLibraryInterfaceVersion()
{
    return APE_INTERFACE_VERSION;
}

int __stdcall TagFileSimple(const str_ansi * pFilename, const char * pArtist, const char * pAlbum, const char * pTitle, const char * pComment, const char * pGenre, const char * pYear, const char * pTrack, BOOL bClearFirst, BOOL bUseOldID3)
{
    CSmartPtr<wchar_t> spFilename(CAPECharacterHelper::GetUTF16FromANSI(pFilename), TRUE);

    CSmartPtr<CIO> spFileIO(CreateCIO());
    if (spFileIO->Open(spFilename) != 0)
        return ERROR_UNDEFINED;

    CAPETag APETag(spFileIO, true);

    if (bClearFirst)
        APETag.ClearFields();

    APETag.SetFieldString(APE_TAG_FIELD_ARTIST, pArtist, TRUE);
    APETag.SetFieldString(APE_TAG_FIELD_ALBUM, pAlbum, TRUE);
    APETag.SetFieldString(APE_TAG_FIELD_TITLE, pTitle, TRUE);
    APETag.SetFieldString(APE_TAG_FIELD_GENRE, pGenre, TRUE);
    APETag.SetFieldString(APE_TAG_FIELD_YEAR, pYear, TRUE);
    APETag.SetFieldString(APE_TAG_FIELD_COMMENT, pComment, TRUE);
    APETag.SetFieldString(APE_TAG_FIELD_TRACK, pTrack, TRUE);

    if (APETag.Save(bUseOldID3) != 0)
    {
        return ERROR_UNDEFINED;
    }

    return ERROR_SUCCESS;
}

int __stdcall GetID3Tag(const str_ansi * pFilename, ID3_TAG * pID3Tag)
{
    CSmartPtr<wchar_t> spFilename(CAPECharacterHelper::GetUTF16FromANSI(pFilename), TRUE);
    return GetID3TagW(spFilename, pID3Tag);
}

int __stdcall GetID3TagW(const str_utfn * pFilename, ID3_TAG* pID3Tag)
{
    CSmartPtr<CIO> spFileIO(CreateCIO());
    if (spFileIO->Open(pFilename) != 0)
        return ERROR_UNDEFINED;

    CAPETag APETag(spFileIO, TRUE);

    return APETag.CreateID3Tag(pID3Tag);
}

int __stdcall RemoveTag(const char * pFilename)
{
    CSmartPtr<wchar_t> spFilename(CAPECharacterHelper::GetUTF16FromANSI(pFilename), true);
    return RemoveTagW(spFilename);
}

int __stdcall RemoveTagW(const str_utfn * pFilename)
{
    // create a decompress object
    int nErrorCode = ERROR_SUCCESS;
    CSmartPtr<IAPEDecompress> spAPEDecompress(CreateIAPEDecompress(pFilename, &nErrorCode, false, true, false));

    // error check creation
    if (nErrorCode != ERROR_SUCCESS)
        return nErrorCode;
    if (spAPEDecompress == NULL) return ERROR_UNDEFINED;

    // remove the tag
    IAPETag * pTag = GET_TAG(spAPEDecompress);
    if (pTag != APE_NULL)
        nErrorCode = pTag->Remove(false);
    else
        nErrorCode = ERROR_UNDEFINED;

    // result
    return nErrorCode;
}

/**************************************************************************************************
CAPEDecompress wrapper(s)
**************************************************************************************************/
APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_Create(const str_ansi * pFilename, int * pErrorCode)
{
    CSmartPtr<wchar_t> spFilename(CAPECharacterHelper::GetUTF16FromANSI(pFilename), true);
    return static_cast<APE_DECOMPRESS_HANDLE>(CreateIAPEDecompress(spFilename, pErrorCode, true, true, false));
}

APE_DECOMPRESS_HANDLE __stdcall c_APEDecompress_CreateW(const str_utfn * pFilename, int * pErrorCode)
{
    return static_cast<APE_DECOMPRESS_HANDLE>(CreateIAPEDecompress(pFilename, pErrorCode, true, true, false));
}

void __stdcall c_APEDecompress_Destroy(APE_DECOMPRESS_HANDLE hAPEDecompress)
{
    IAPEDecompress * pAPEDecompress = static_cast<IAPEDecompress *>(hAPEDecompress);
    if (pAPEDecompress)
        delete pAPEDecompress;
}

int __stdcall c_APEDecompress_GetData(APE_DECOMPRESS_HANDLE hAPEDecompress, unsigned char * pBuffer, int64 nBlocks, int64 * pBlocksRetrieved)
{
    return (static_cast<IAPEDecompress *>(hAPEDecompress))->GetData(pBuffer, nBlocks, pBlocksRetrieved);
}

int __stdcall c_APEDecompress_Seek(APE_DECOMPRESS_HANDLE hAPEDecompress, int64 nBlockOffset)
{
    return (static_cast<IAPEDecompress *>(hAPEDecompress))->Seek(nBlockOffset);
}

int64 __stdcall c_APEDecompress_GetInfo(APE_DECOMPRESS_HANDLE hAPEDecompress, IAPEDecompress::APE_DECOMPRESS_FIELDS Field, int64 nParam1, int64 nParam2)
{
    return (static_cast<IAPEDecompress *>(hAPEDecompress))->GetInfo(Field, nParam1, nParam2);
}

/**************************************************************************************************
CAPECompress wrapper(s)
**************************************************************************************************/
APE_COMPRESS_HANDLE __stdcall c_APECompress_Create(int * pErrorCode)
{
    return static_cast<APE_COMPRESS_HANDLE>(CreateIAPECompress(pErrorCode));
}

void __stdcall c_APECompress_Destroy(APE_COMPRESS_HANDLE hAPECompress)
{
    IAPECompress * pAPECompress = static_cast<IAPECompress *>(hAPECompress);
    if (pAPECompress)
        delete pAPECompress;
}

int __stdcall c_APECompress_Start(APE_COMPRESS_HANDLE hAPECompress, const char * pOutputFilename, const APE::WAVEFORMATEX * pwfeInput, APE::int64 nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, APE::int64 nHeaderBytes)
{
    CSmartPtr<wchar_t> spOutputFilename(CAPECharacterHelper::GetUTF16FromANSI(pOutputFilename), TRUE);
    return (static_cast<IAPECompress *>(hAPECompress))->Start(spOutputFilename, pwfeInput, nMaxAudioBytes, nCompressionLevel, pHeaderData, nHeaderBytes);
}

int __stdcall c_APECompress_StartW(APE_COMPRESS_HANDLE hAPECompress, const str_utfn * pOutputFilename, const APE::WAVEFORMATEX * pwfeInput, APE::int64 nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, APE::int64 nHeaderBytes)
{
    return (static_cast<IAPECompress *>(hAPECompress))->Start(pOutputFilename, pwfeInput, nMaxAudioBytes, nCompressionLevel, pHeaderData, nHeaderBytes);
}

int64 __stdcall c_APECompress_AddData(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pData, int nBytes)
{
    return (static_cast<IAPECompress *>(hAPECompress))->AddData(pData, nBytes);
}

int __stdcall c_APECompress_GetBufferBytesAvailable(APE_COMPRESS_HANDLE hAPECompress)
{
    return static_cast<int>((static_cast<IAPECompress *>(hAPECompress))->GetBufferBytesAvailable());
}

unsigned char * __stdcall c_APECompress_LockBuffer(APE_COMPRESS_HANDLE hAPECompress, int64 * pBytesAvailable)
{
    return (static_cast<IAPECompress *>(hAPECompress))->LockBuffer(pBytesAvailable);
}

int __stdcall c_APECompress_UnlockBuffer(APE_COMPRESS_HANDLE hAPECompress, int nBytesAdded, BOOL bProcess)
{
    return static_cast<int>((static_cast<IAPECompress *>(hAPECompress))->UnlockBuffer(nBytesAdded, bProcess));
}

int __stdcall c_APECompress_Finish(APE_COMPRESS_HANDLE hAPECompress, unsigned char * pTerminatingData, int64 nTerminatingBytes, int64 nWAVTerminatingBytes)
{
    return (static_cast<IAPECompress *>(hAPECompress))->Finish(pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes);
}

int __stdcall c_APECompress_Kill(APE_COMPRESS_HANDLE hAPECompress)
{
    return (static_cast<IAPECompress *>(hAPECompress))->Kill();
}
