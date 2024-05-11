#include "All.h"

#ifdef APE_SUPPORT_COMPRESS

#include "APECompress.h"
#include "APECompressCreate.h"
#include "WAVInputSource.h"
#include "FloatTransform.h"

namespace APE
{

CAPECompress::CAPECompress()
{
    m_nBufferHead = 0;
    m_nBufferTail = 0;
    m_nBufferSize = 0;
    m_bBufferLocked = false;
    m_bFloat = false;
    APE_CLEAR(m_wfeInput);

    m_spAPECompressCreate.Assign(new CAPECompressCreate());
}

CAPECompress::~CAPECompress()
{
    m_spBuffer.Delete();
    m_spioOutput.Delete();
}

int CAPECompress::Start(const wchar_t * pOutputFilename, const WAVEFORMATEX * pwfeInput, bool bFloat, int64 nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int64 nHeaderBytes, int nFlags)
{
    m_spioOutput.Delete();

    m_spioOutput.Assign(CreateCIO());

    // update float
    HandleFloat(bFloat, pwfeInput);

    // create
    if (m_spioOutput->Create(pOutputFilename) != 0)
    {
        return ERROR_INVALID_OUTPUT_FILE;
    }

    // start
    const int nStartResult = m_spAPECompressCreate->Start(m_spioOutput, pwfeInput, nMaxAudioBytes, nCompressionLevel,
        pHeaderData, nHeaderBytes, nFlags);

    // create buffer
    m_spBuffer.Delete();
    m_nBufferSize = m_spAPECompressCreate->GetFullFrameBytes();
    m_spBuffer.Assign(new unsigned char [static_cast<size_t>(m_nBufferSize)], true);

    // store format
    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));

    return nStartResult;
}

int CAPECompress::StartEx(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, bool bFloat, int64 nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int64 nHeaderBytes)
{
    // store information (we don't own the I/O object so don't delete it)
    m_spioOutput.Assign(pioOutput, false, false);

    // update float
    HandleFloat(bFloat, pwfeInput);

    // start
    m_spAPECompressCreate->Start(m_spioOutput, pwfeInput, nMaxAudioBytes, nCompressionLevel,
        pHeaderData, nHeaderBytes);

    // create buffer
    m_spBuffer.Delete();
    m_nBufferSize = m_spAPECompressCreate->GetFullFrameBytes();
    m_spBuffer.Assign(new unsigned char [static_cast<size_t>(m_nBufferSize)], true);

    // store format
    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));

    return ERROR_SUCCESS;
}

int64 CAPECompress::GetBufferBytesAvailable()
{
    return m_nBufferSize - m_nBufferTail;
}

int64 CAPECompress::UnlockBuffer(int64 nBytesAdded, bool bProcess)
{
    if (!m_bBufferLocked)
        return ERROR_UNDEFINED;

    m_nBufferTail += nBytesAdded;
    m_bBufferLocked = false;

    if (bProcess)
    {
        const int64 nResult = ProcessBuffer();
        if (nResult != 0) { return nResult; }
    }

    return ERROR_SUCCESS;
}

unsigned char * CAPECompress::LockBuffer(int64 * pBytesAvailable)
{
    if (m_spBuffer == APE_NULL) { return APE_NULL; }

    if (m_bBufferLocked)
        return APE_NULL;

    m_bBufferLocked = true;

    if (pBytesAvailable)
        *pBytesAvailable = GetBufferBytesAvailable();

    return &m_spBuffer[m_nBufferTail];
}

int64 CAPECompress::AddData(unsigned char * pData, int64 nBytes)
{
    // the input size should be block aligned or else the processing functions will fall apart
    ASSERT((nBytes % m_wfeInput.nBlockAlign) == 0);

    // check the buffer
    if (m_spBuffer == APE_NULL) return ERROR_INSUFFICIENT_MEMORY;

    // loop
    int64 nBytesDone = 0;
    while (nBytesDone < nBytes)
    {
        // lock the buffer
        int64 nBytesAvailable = 0;
        unsigned char * pBuffer = LockBuffer(&nBytesAvailable);
        if (pBuffer == APE_NULL || nBytesAvailable <= 0)
        {
            return m_spAPECompressCreate->GetTooMuchData() ? ERROR_APE_COMPRESS_TOO_MUCH_DATA : ERROR_UNDEFINED;
        }

        // calculate how many bytes to copy and add that much to the buffer
        const int64 nBytesToProcess = ape_min(nBytesAvailable, nBytes - nBytesDone);
        memcpy(pBuffer, &pData[nBytesDone], static_cast<size_t>(nBytesToProcess));

        // unlock the buffer (fail if not successful)
        const int64 nResult = UnlockBuffer(static_cast<unsigned int>(nBytesToProcess));
        if (nResult != ERROR_SUCCESS)
            return nResult;

        // update our progress
        nBytesDone += nBytesToProcess;
    }

    return ERROR_SUCCESS;
}

int CAPECompress::Finish(unsigned char * pTerminatingData, int64 nTerminatingBytes, int64 nWAVTerminatingBytes)
{
    RETURN_ON_ERROR(ProcessBuffer(true))
    return m_spAPECompressCreate->Finish(pTerminatingData, nTerminatingBytes, nWAVTerminatingBytes);
}

int CAPECompress::Kill()
{
    return ERROR_SUCCESS;
}

int CAPECompress::ProcessBuffer(bool bFinalize)
{
    if (m_spBuffer == APE_NULL) { return ERROR_UNDEFINED; }

    try
    {
        // process as much as possible
        const int64 nThreshold = (bFinalize) ? 0 : m_spAPECompressCreate->GetFullFrameBytes();

        while ((m_nBufferTail - m_nBufferHead) >= nThreshold)
        {
            int64 nFrameBytes = ape_min(m_spAPECompressCreate->GetFullFrameBytes(), m_nBufferTail - m_nBufferHead);

            // truncate to the size of a floating point number in float mode (since CFloatTransform::Process can only work on full samples)
            if (m_bFloat)
                nFrameBytes = (nFrameBytes / static_cast<int64>(sizeof(float))) * static_cast<int64>(sizeof(float));

            // break if we have no size
            if (nFrameBytes == 0)
                break;

            // float process
            if (m_bFloat)
                CFloatTransform::Process(reinterpret_cast<uint32 *>(&m_spBuffer[m_nBufferHead]), nFrameBytes / static_cast<int64>(sizeof(float)));

            // encode
            const int nResult = m_spAPECompressCreate->EncodeFrame(&m_spBuffer[m_nBufferHead], static_cast<int>(nFrameBytes));
            if (nResult != 0) { return nResult; }

            m_nBufferHead += nFrameBytes;
        }

        // shift the buffer
        if (m_nBufferHead != 0)
        {
            const int64 nBytesLeft = m_nBufferTail - m_nBufferHead;

            if (nBytesLeft != 0)
                memmove(m_spBuffer, &m_spBuffer[m_nBufferHead], static_cast<size_t>(nBytesLeft));

            m_nBufferTail -= m_nBufferHead;
            m_nBufferHead = 0;
        }
    }
    catch(...)
    {
        return ERROR_UNDEFINED;
    }

    return ERROR_SUCCESS;
}

int64 CAPECompress::AddDataFromInputSource(CInputSource * pInputSource, int64 nMaxBytes, int64 * pBytesAdded)
{
    // error check the parameters
    if (pInputSource == APE_NULL) return ERROR_BAD_PARAMETER;

    // initialize
    if (pBytesAdded) *pBytesAdded = 0;

    // lock the buffer
    int64 nBytesAvailable = 0;
    unsigned char * pBuffer = LockBuffer(&nBytesAvailable);
    if ((pBuffer == APE_NULL) || (nBytesAvailable == 0))
        return ERROR_INSUFFICIENT_MEMORY;

    // calculate the 'ideal' number of bytes
    int64 nBytesRead = 0;

    const int64 nIdealBytes = m_spAPECompressCreate->GetFullFrameBytes() - (m_nBufferTail - m_nBufferHead);
    if (nIdealBytes > 0)
    {
        // get the data
        int64 nBytesToAdd = nBytesAvailable;

        if (nMaxBytes > 0)
        {
            if (nBytesToAdd > nMaxBytes)
                nBytesToAdd = nMaxBytes;
        }

        if (nBytesToAdd > nIdealBytes) nBytesToAdd = nIdealBytes;

        // always make requests along block boundaries
        while ((nBytesToAdd % m_wfeInput.nBlockAlign) != 0)
            nBytesToAdd--;

        const int64 nBlocksToAdd = nBytesToAdd / m_wfeInput.nBlockAlign;

        // get data
        int nBlocksAdded = 0;
        const int nResult = pInputSource->GetData(pBuffer, static_cast<int>(nBlocksToAdd), &nBlocksAdded);
        if (nResult != 0)
            return nResult;
        else
            nBytesRead = static_cast<int64>(nBlocksAdded) * static_cast<int64>(m_wfeInput.nBlockAlign);

        // store the bytes read
        if (pBytesAdded)
            *pBytesAdded = nBytesRead;
    }

    // unlock the data and process
    const int64 nResult = UnlockBuffer(nBytesRead, true);
    if (nResult != 0)
    {
        return nResult;
    }

    return ERROR_SUCCESS;
}

void CAPECompress::HandleFloat(bool bFloat, const WAVEFORMATEX * pwfeInput)
{
    // set the float flag if we're WAVE_FORMAT_IEEE_FLOAT but it should already be set by the caller
    // we pass a flag because WAVE_FORMAT_EXTENSIBLE files can also be float, so we need to look at something beyond the format
    if (pwfeInput->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        ASSERT(bFloat != false);
        bFloat = true;
    }

    // store the floating point
    m_bFloat = bFloat;
}

}

#endif
