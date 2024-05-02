//-----------------------------------------------------------------------------
//
//    RadLight APE Decoder
//
//    Original Author: Igor Janos
//    Updated by Matt Ashland at times
//
//-----------------------------------------------------------------------------

#include "All.h"
#include "MACLib.h"
#include "RegistryUtils.h"
#include <streams.h>
#include <wxutil.h>
#include <initguid.h>
#include <stdio.h>
#include "main.h"

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpReserved);
}

// {D042079E-8E02-418b-AE2F-F12E26704FCA}
DEFINE_GUID(CLSID_APEDecoder,
0xd042079e, 0x8e02, 0x418b, 0xae, 0x2f, 0xf1, 0x2e, 0x26, 0x70, 0x4f, 0xca);

//-----------------------------------------------------------------------------
//
//    Registration Stuff
//
//-----------------------------------------------------------------------------

const AMOVIESETUP_MEDIATYPE sudAPEOutPinTypes =
                                {
                                    &MEDIATYPE_Audio,        // Major type
                                    &MEDIASUBTYPE_PCM        // Minor type
                                };

const AMOVIESETUP_PIN sudAPEOutPin =
                                {
                                    NULL,                   // Pin string name (MSDN says this is obsolete, so I switched from L"PCM Out" to NULL on 3/11/2024 to avoid Clang warning)
                                    FALSE,                  // Is it rendered
                                    TRUE,                   // Is it an output
                                    FALSE,                  // Can we have none
                                    FALSE,                  // Can we have many
                                    &CLSID_NULL,            // Connects to filter
                                    NULL,                   // Connects to pin
                                    1,                      // Number of types
                                    &sudAPEOutPinTypes      // Pin details
                                };

const AMOVIESETUP_FILTER sudAPEDec =
                                {
                                    &CLSID_APEDecoder,           // Filter CLSID
                                    L"APE DirectShow Filter",    // String name
                                    MERIT_NORMAL,                // Filter merit
                                    1,                           // Number pins
                                    &sudAPEOutPin                // Pin details
                                };

CFactoryTemplate g_Templates[] = {
                                {
                                    L"APE DirectShow Filter",
                                    &CLSID_APEDecoder,
                                    CAPESource::CreateInstance,
                                    NULL,
                                    &sudAPEDec
                                }
                                };

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
    HRESULT hr = AMovieDllRegisterServer2(TRUE);
    if (FAILED(hr)) return hr;

    // register as APE Source
    TCHAR achTemp[MAX_PATH];

    OLECHAR szCLSID[CHARS_IN_GUID];
    hr = StringFromGUID2(CLSID_APEDecoder, szCLSID, CHARS_IN_GUID);
    if (FAILED(hr))
    {
        ASSERT(FALSE);
    }

    ASSERT(SUCCEEDED(hr));

    // create .APE registry keys
    HKEY hkey;

    {
        // .ape
        LONG lr = RegCreateKey(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\.ape"), &hkey);
        if (lr != ERROR_SUCCESS) return AmHresultFromWin32(lr);

        wsprintf(achTemp, _T("%ls"), szCLSID);
        lr = RegSetValueEx(hkey, _T("Source Filter"), 0, REG_SZ, reinterpret_cast<const BYTE*>(achTemp), static_cast<size_t>(2 + CHARS_IN_GUID) * sizeof(TCHAR));
        if (lr != ERROR_SUCCESS) return AmHresultFromWin32(lr);
        RegCloseKey(hkey);

        RegisterWMPExtension(_T("*.ape"), _T("Monkey's Audio Files (*.ape)"),
            _T("Monkey's Audio Files"), _T("audio"));
    }

    {
        // .apl
        LONG lr = RegCreateKey(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\.apl"), &hkey);
        if (lr != ERROR_SUCCESS) return AmHresultFromWin32(lr);

        wsprintf(achTemp, _T("%ls"), szCLSID);
        lr = RegSetValueEx(hkey, _T("Source Filter"), 0, REG_SZ, reinterpret_cast<const BYTE*>(achTemp), static_cast<size_t>(2 + CHARS_IN_GUID) * sizeof(TCHAR));
        if (lr != ERROR_SUCCESS) return AmHresultFromWin32(lr);
        RegCloseKey(hkey);

        RegisterWMPExtension(_T("*.apl"), _T("Monkey's Audio Link Files (*.apl)"),
            _T("Monkey's Audio Link Files"), _T("audio"));
    }

    return NOERROR;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr = AMovieDllRegisterServer2(FALSE);
    if (FAILED(hr)) return hr;

    UnRegisterWMPExtension(_T("*.ape"));

    // unregister
    RegDeleteKey(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\.ape"));
    RegDeleteKey(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\.apl"));

    return NOERROR;
}


//-----------------------------------------------------------------------------
//
//    CAPESource Implementation
//
//-----------------------------------------------------------------------------

CUnknown *WINAPI CAPESource::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CAPESource(lpunk, phr);
    if (punk == NULL) *phr = E_OUTOFMEMORY;
    return punk;
}

CAPESource::CAPESource(LPUNKNOWN lpunk, HRESULT *phr)
    :
    CSource(NAME("APE DirectShow Filter"), lpunk, CLSID_APEDecoder),
    m_pFileName(NULL)
{

    CAutoLock cAutoLock(&m_cStateLock);

    // output pins
    m_paStreams    = reinterpret_cast<CSourceStream **>(new CAPEStream*[1]);
    if (m_paStreams == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_paStreams[0] = new CAPEStream(phr, this, L"PCM Out");
    if (m_paStreams[0] == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
}

CAPESource::~CAPESource()
{
    if (m_pFileName != NULL) {
        free(m_pFileName);
        m_pFileName = NULL;
    }
}

STDMETHODIMP CAPESource::GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER)
    *ppszFileName = NULL;

    if (m_pFileName != NULL) {
        DWORD n = static_cast<DWORD>(sizeof(WCHAR)*(static_cast<size_t>(1)+static_cast<size_t>(lstrlenW(m_pFileName))));

        *ppszFileName = static_cast<LPOLESTR>(CoTaskMemAlloc( n ));
        if (*ppszFileName!=NULL) {
            CopyMemory(*ppszFileName, m_pFileName, n);
        }
    }

    if (pmt!=NULL) {
        CopyMediaType(pmt, &((static_cast<CAPEStream *>(m_paStreams[0]))->m_mtOut));
    }

    return NOERROR;
}


STDMETHODIMP CAPESource::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER)

    // lstrlenW is one of the few Unicode functions that works on win95
    size_t cch = static_cast<size_t>(lstrlenW(lpwszFileName)) + static_cast<size_t>(1);
    m_pFileName = new WCHAR[cch];
    if (m_pFileName!=NULL) CopyMemory(m_pFileName, lpwszFileName, cch * sizeof(WCHAR));

    // open the file
    CAPEStream * pStream = static_cast<CAPEStream *>(m_paStreams[0]);
    HRESULT hr = pStream->OpenAPEFile(m_pFileName);

    return hr;
}

STDMETHODIMP CAPESource::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER)

    if (riid == IID_IFileSourceFilter) {
        return GetInterface(static_cast<IFileSourceFilter *>(this), ppv);
    }

    return CSource::NonDelegatingQueryInterface(riid,ppv);
}


//-----------------------------------------------------------------------------
//
//    CAPEStream Implementation
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Name : CAPEStream::CAPEStream
//-----------------------------------------------------------------------------
CAPEStream::CAPEStream(HRESULT *phr, CAPESource *pParent, LPCWSTR pPinName)
    :
    CSourceStream(NAME("PCM Out"),phr, pParent, pPinName),
    CSourceSeeking(NAME("APE Seeking"), NULL, phr, &m_csSeeking),
    m_pSource(pParent),
    m_pDecoder(NULL),
    m_dDuration(0),
    m_iBlockSize(0),
    m_iBlocksDecoded(0),
    m_iTotalBlocks(0),
    m_lSampleRate(0),
    m_bDiscontinuity(FALSE)
{
    CSourceSeeking::AddRef();
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::~CAPEStream
//-----------------------------------------------------------------------------
CAPEStream::~CAPEStream()
{
    ReleaseAPEObjects();
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::NonDelegatingQueryInterface
//-----------------------------------------------------------------------------
STDMETHODIMP CAPEStream::NonDelegatingQueryInterface(REFIID riid, VOID **ppv)
{
    if (riid == IID_IMediaSeeking) {
        return CSourceSeeking::NonDelegatingQueryInterface(riid, ppv);
    }

    return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::DecideBufferSize
//-----------------------------------------------------------------------------
HRESULT CAPEStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
    ASSERT(pAlloc);
    ASSERT(pRequest);

    // must have a decoder
    if (m_pDecoder == NULL) return E_FAIL;

    // get the format
    WAVEFORMATEX *pwf = reinterpret_cast<WAVEFORMATEX *>(m_mtOut.Format());
    if (pwf == NULL) return E_FAIL;

    pRequest->cbBuffer    =    pwf->nChannels * WAVE_BUFFER_SIZE;
    pRequest->cBuffers    =    (static_cast<long>(pwf->nChannels) * static_cast<long>(pwf->nSamplesPerSec) * static_cast<long>(pwf->wBitsPerSample)) /
                            static_cast<long>(pRequest->cbBuffer * 8);

    // need at least one buffer
    if (pRequest->cBuffers < 1) pRequest->cBuffers = 1;

    // allocate some memory
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
    if (FAILED(hr)) return hr;

    // check the buffer
    if (Actual.cbBuffer < pRequest->cbBuffer) return E_FAIL;

    return NOERROR;
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::CheckMediaType
//-----------------------------------------------------------------------------
HRESULT CAPEStream::CheckMediaType(const CMediaType * pmt)
{
    if (m_pDecoder == NULL || *pmt != m_mtOut) return E_FAIL;

    return NOERROR;
}


//-----------------------------------------------------------------------------
// Name : CAPEStream::GetMediaType
//-----------------------------------------------------------------------------
HRESULT CAPEStream::GetMediaType(CMediaType * pmt)
{
    *pmt = m_mtOut;
    return NOERROR;
}


//-----------------------------------------------------------------------------
// Name : CAPEStream::ReleaseAPEObjects
//-----------------------------------------------------------------------------
void CAPEStream::ReleaseAPEObjects()
{
    if (m_pDecoder != NULL)
    {
        delete m_pDecoder;
        m_pDecoder = NULL;
    }
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::OpenAPEFile
//-----------------------------------------------------------------------------
HRESULT CAPEStream::OpenAPEFile(LPWSTR pFileName)
{
    // file already opened ?
    if (m_pDecoder != NULL) return S_FALSE;

    int nResult = ERROR_SUCCESS;
    m_pDecoder = CreateIAPEDecompress(pFileName, &nResult, true, true, false);

    // no decoder, fail
    if ((m_pDecoder == NULL) || (nResult != ERROR_SUCCESS))
        return E_FAIL;

    m_dDuration = static_cast<double>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_DECOMPRESS_LENGTH_MS));

    m_rtDuration = static_cast<__int64>(static_cast<double>(m_dDuration) * static_cast<double>(10000));
    m_iBlockSize = static_cast<unsigned int>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_CHANNELS) * (m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_BITS_PER_SAMPLE) >> 3));
    m_iTotalBlocks = static_cast<unsigned int>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS));

    // display information message
//#define DISPLAY_MESSAGE
#ifdef DISPLAY_MESSAGE
    char *str = (char *)malloc(512);
    ZeroMemory(str, 512);

    wsprintf(str, "Length in ms : %d\nBlockSize : %d\nTotal Blocks : %d\n",
             m_pDecoder->GetInfo(APE_INFO_LENGTH_MS),
             m_iBlockSize,
             m_iTotalBlocks);
    MessageBox(NULL, str, "Info", 0);

    free(str);
#endif /* DISPLAY_MESSAGE */

    // set the correct media type
    m_mtOut.SetType(&MEDIATYPE_Audio);
    m_mtOut.SetSubtype(&MEDIASUBTYPE_PCM);

    WAVEFORMATEX * pwf = reinterpret_cast<WAVEFORMATEX *>(m_mtOut.AllocFormatBuffer(sizeof(WAVEFORMATEX)));

    // build wave format
    ZeroMemory(pwf, sizeof(WAVEFORMATEX));

    pwf->nChannels = static_cast<WORD>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_CHANNELS));
    pwf->wBitsPerSample = static_cast<WORD>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_BITS_PER_SAMPLE));
    m_lSampleRate = static_cast<long>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_SAMPLE_RATE));
    pwf->nSamplesPerSec = static_cast<DWORD>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_SAMPLE_RATE));
    pwf->nBlockAlign = static_cast<WORD>(m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_BLOCK_ALIGN));
    pwf->wFormatTag = WAVE_FORMAT_PCM;
    pwf->nAvgBytesPerSec = pwf->nChannels * (pwf->wBitsPerSample>>3) * pwf->nSamplesPerSec;

    if (m_pDecoder->GetInfo(APE::IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_FLOATING_POINT)
        pwf->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;

    m_mtOut.SetFormatType(&FORMAT_WaveFormatEx);

    m_iBlocksDecoded = 0;

    return NOERROR;
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::FillBuffer
//-----------------------------------------------------------------------------
HRESULT CAPEStream::FillBuffer(IMediaSample *pSample)
{
    ASSERT(pSample);
    ASSERT(m_pDecoder);

    PBYTE    pData = NULL;
    pSample->GetPointer(&pData);
    __int64 lSize = pSample->GetSize();

    __int64 iBlocksDecoded = 0;

    {
        CAutoLock    Lck(&m_csDecoding);

        m_pDecoder->GetData(static_cast<unsigned char *>(pData), lSize / m_iBlockSize, &iBlocksDecoded);

        lSize = iBlocksDecoded * m_iBlockSize;

        pSample->SetActualDataLength(static_cast<LONG>(lSize));
        pSample->SetDiscontinuity(m_bDiscontinuity);
        m_bDiscontinuity = FALSE;

        // time stamps
        CRefTime    rtStart;
        CRefTime    rtStop;

        rtStart = static_cast<__int64>( static_cast<double>(m_iBlocksDecoded) * (static_cast<double>(10000000) / static_cast<double>(m_lSampleRate) ) );
        rtStart -= m_rtStart;
        rtStop = rtStart + 1;
        pSample->SetTime(reinterpret_cast<REFERENCE_TIME *>(&rtStart), reinterpret_cast<REFERENCE_TIME *>(&rtStop));
        m_iBlocksDecoded += static_cast<unsigned int>(iBlocksDecoded);

    }

    return (iBlocksDecoded == 0 ? S_FALSE : NOERROR);

}


//-----------------------------------------------------------------------------
// Name : CAPEStream::UpdateFromSeek
//-----------------------------------------------------------------------------
void CAPEStream::UpdateFromSeek()
{
    if (ThreadExists()) {
        DeliverBeginFlush();
        Stop();
        DeliverEndFlush();
        Run();
    }
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::OnThreadStartPlay
//-----------------------------------------------------------------------------
HRESULT CAPEStream::OnThreadStartPlay()
{
    m_bDiscontinuity = TRUE;
    return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}


//-----------------------------------------------------------------------------
// Name : CAPEStream::ChangeStart
//-----------------------------------------------------------------------------
HRESULT CAPEStream::ChangeStart()
{
    if (m_pDecoder == NULL) return S_FALSE;

    unsigned int iBlock = static_cast<unsigned int>((static_cast<double>(m_rtStart) / static_cast<double>(m_rtDuration)) * m_iTotalBlocks);
    {
        CAutoLock    lck(&m_csDecoding);
        m_pDecoder->Seek(iBlock);
        m_iBlocksDecoded = iBlock;
    }

    UpdateFromSeek();
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name : CAPEStream::ChangeStop
//-----------------------------------------------------------------------------
HRESULT CAPEStream::ChangeStop()
{
    UpdateFromSeek();
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name : CAPEStream::ChangeRate
//-----------------------------------------------------------------------------
HRESULT CAPEStream::ChangeRate()
{
    {
        // Scope for critical section lock.
        CAutoLock cAutoLockSeeking(CSourceSeeking::m_pLock);
        m_dRateSeeking = 1.0;  // only support 1.0
    }
    UpdateFromSeek();
    return S_OK;
}
