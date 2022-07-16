#include "All.h"

#ifdef APE_SUPPORT_COMPRESS

#include "APECompress.h"
#include "APECompressCreate.h"
#include "WAVInputSource.h"

namespace APE
{

CAPECompress::CAPECompress()
{
    m_nBufferHead = 0;
    m_nBufferTail = 0;
    m_nBufferSize = 0;
    m_bBufferLocked = false;
    m_bOwnsOutputIO = false;
    m_pioOutput = NULL;

    m_spAPECompressCreate.Assign(new CAPECompressCreate());
}

CAPECompress::~CAPECompress()
{
    m_spBuffer.Delete();

    if (m_bOwnsOutputIO)
    {
        SAFE_DELETE(m_pioOutput)
    }
}

int CAPECompress::Start(const wchar_t * pOutputFilename, const WAVEFORMATEX * pwfeInput, int64 nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int64 nHeaderBytes, int nFlags)
{
    if (m_pioOutput)
    {
        SAFE_DELETE(m_pioOutput)
    }

    m_pioOutput = CreateCIO();
    m_bOwnsOutputIO = true;
    
    if (m_pioOutput->Create(pOutputFilename) != 0)
    {
        return ERROR_INVALID_OUTPUT_FILE;
    }
        
    int nStartResult =  m_spAPECompressCreate->Start(m_pioOutput, pwfeInput, nMaxAudioBytes, nCompressionLevel,
        pHeaderData, nHeaderBytes, nFlags);
    
    m_spBuffer.Delete();
    m_nBufferSize = m_spAPECompressCreate->GetFullFrameBytes();
    m_spBuffer.Assign(new unsigned char [uint32(m_nBufferSize)], true);
    memcpy(&m_wfeInput, pwfeInput, sizeof(WAVEFORMATEX));

    return nStartResult;
}

int CAPECompress::StartEx(CIO * pioOutput, const WAVEFORMATEX * pwfeInput, int64 nMaxAudioBytes, int nCompressionLevel, const void * pHeaderData, int64 nHeaderBytes)
{
    m_pioOutput = pioOutput;
    m_bOwnsOutputIO = false;

    m_spAPECompressCreate->Start(m_pioOutput, pwfeInput, nMaxAudioBytes, nCompressionLevel,
        pHeaderData, nHeaderBytes);

    m_spBuffer.Delete();
    m_nBufferSize = m_spAPECompressCreate->GetFullFrameBytes();
    m_spBuffer.Assign(new unsigned char [uint32(m_nBufferSize)], true);
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
        int64 nResult = ProcessBuffer();
        if (nResult != 0) { return nResult; }
    }
    
    return ERROR_SUCCESS;
}

unsigned char * CAPECompress::LockBuffer(int64 * pBytesAvailable)
{
    if (m_spBuffer == NULL) { return NULL; }
    
    if (m_bBufferLocked)
        return NULL;
    
    m_bBufferLocked = true;
    
    if (pBytesAvailable)
        *pBytesAvailable = GetBufferBytesAvailable();
    
    return &m_spBuffer[m_nBufferTail];
}

int64 CAPECompress::AddData(unsigned char * pData, int64 nBytes)
{
    if (m_spBuffer == NULL) return ERROR_INSUFFICIENT_MEMORY;

    int64 nBytesDone = 0;
    
    while (nBytesDone < nBytes)
    {
        // lock the buffer
        int64 nBytesAvailable = 0;
        unsigned char * pBuffer = LockBuffer(&nBytesAvailable);
        if (pBuffer == NULL || nBytesAvailable <= 0)
        {
            return m_spAPECompressCreate->GetTooMuchData() ? ERROR_APE_COMPRESS_TOO_MUCH_DATA : ERROR_UNDEFINED;
        }
        
        // calculate how many bytes to copy and add that much to the buffer
        int64 nBytesToProcess = ape_min(nBytesAvailable, nBytes - nBytesDone);
        memcpy(pBuffer, &pData[nBytesDone], size_t(nBytesToProcess));
                        
        // unlock the buffer (fail if not successful)
        int64 nResult = UnlockBuffer((unsigned int) nBytesToProcess);
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
    return m_spAPECompressCreate->Finish(pTerminatingData, (int) nTerminatingBytes, (int) nWAVTerminatingBytes);
}

int CAPECompress::Kill()
{
    return ERROR_SUCCESS;
}

int CAPECompress::ProcessBuffer(bool bFinalize)
{
    if (m_spBuffer == NULL) { return ERROR_UNDEFINED; }
    
    try
    {
        // process as much as possible
        int64 nThreshold = (bFinalize) ? 0 : m_spAPECompressCreate->GetFullFrameBytes();
        
        while ((m_nBufferTail - m_nBufferHead) >= nThreshold)
        {
            int64 nFrameBytes = ape_min(m_spAPECompressCreate->GetFullFrameBytes(), m_nBufferTail - m_nBufferHead);
            
            if (nFrameBytes == 0)
                break;

            int nResult = m_spAPECompressCreate->EncodeFrame(&m_spBuffer[m_nBufferHead], (int) nFrameBytes);
            if (nResult != 0) { return nResult; }
            
            m_nBufferHead += nFrameBytes;
        }
        
        // shift the buffer
        if (m_nBufferHead != 0)
        {
            int64 nBytesLeft = m_nBufferTail - m_nBufferHead;
            
            if (nBytesLeft != 0)
                memmove(m_spBuffer, &m_spBuffer[m_nBufferHead], (size_t) nBytesLeft);
            
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
    if (pInputSource == NULL) return ERROR_BAD_PARAMETER;

    // initialize
    if (pBytesAdded) *pBytesAdded = 0;
        
    // lock the buffer
    int64 nBytesAvailable = 0;
    unsigned char * pBuffer = LockBuffer(&nBytesAvailable);
    if ((pBuffer == NULL) || (nBytesAvailable == 0))
        return ERROR_INSUFFICIENT_MEMORY;
    
    // calculate the 'ideal' number of bytes
    int64 nBytesRead = 0;

    int64 nIdealBytes = m_spAPECompressCreate->GetFullFrameBytes() - (m_nBufferTail - m_nBufferHead);
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

        int64 nBlocksToAdd = nBytesToAdd / m_wfeInput.nBlockAlign;

        // get data
        int nBlocksAdded = 0;
        int nResult = pInputSource->GetData(pBuffer, (int) nBlocksToAdd, &nBlocksAdded);
        if (nResult != 0)
            return nResult;
        else
            nBytesRead = (nBlocksAdded * m_wfeInput.nBlockAlign);
        
        // store the bytes read
        if (pBytesAdded)
            *pBytesAdded = nBytesRead;
    }
        
    // unlock the data and process
    int64 nResult = UnlockBuffer(nBytesRead, true);
    if (nResult != 0)
    {
        return nResult;
    }
    
    return ERROR_SUCCESS;
}

}

#endif