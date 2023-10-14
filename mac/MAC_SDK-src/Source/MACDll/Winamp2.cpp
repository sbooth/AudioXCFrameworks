/**************************************************************************************************
Includes
**************************************************************************************************/
#include "stdafx.h"
#include "MACDllApp.h"
#include "APETag.h"
#include "APELink.h"
#include "Winamp2.h"
#include "WinampSettingsDlg.h"
#include "In2.h"
#include "APELink.h"
#include "CharacterHelper.h"
#include <afxmt.h>
#include "wasabi/Wasabi.h"
#include "WinFileIO.h"

using namespace APE;

/**************************************************************************************************
Defines
**************************************************************************************************/
// post this to the main window at end of file (after playback has stopped)
#define WM_WA_MPEG_EOF    (WM_USER + 2)

// scaled bits
#define SCALED_BITS        16

// size of GetFileInformation(...) calls
#define GETFILEINFO_TITLE_LENGTH 2048

// extended info structure
struct extendedFileInfoStruct
{
    char * pFilename;
    char * pMetaData;
    char * pReturn;
    int nReturnBytes;
};

// define to avoid Clang warnings
extern In_Module g_APEWinampPluginModule;

/**************************************************************************************************
The input module (publicly defined)
**************************************************************************************************/
In_Module g_APEWinampPluginModule =
{
    IN_VER,                                                        // the version (defined in in2.h)
    PLUGIN_NAME,                                                   // the name of the plugin (defined in all.h)
    0,                                                             // handle to the main window
    0,                                                             // handle to the dll instance
    "APE\0Monkey's Audio File (*.APE)\0"                           // the file type(s) supported
    "MAC\0Monkey's Audio File (*.MAC)\0"
    "APL\0Monkey's Audio File (*.APL)\0",
    1,                                                             // seekable
    1,                                                             // uses output
    CAPEWinampPlugin::ShowConfigurationDialog,                     // all of the functions...
    CAPEWinampPlugin::ShowAboutDialog,
    CAPEWinampPlugin::InitializePlugin,
    CAPEWinampPlugin::UninitializePlugin,
    CAPEWinampPlugin::GetFileInformation,
    CAPEWinampPlugin::ShowFileInformationDialog,
    CAPEWinampPlugin::IsOurFile,
    CAPEWinampPlugin::Play,
    CAPEWinampPlugin::Pause,
    CAPEWinampPlugin::Unpause,
    CAPEWinampPlugin::IsPaused,
    CAPEWinampPlugin::Stop,
    CAPEWinampPlugin::GetFileLength,
    CAPEWinampPlugin::GetOutputTime,
    CAPEWinampPlugin::SetOutputTime,
    CAPEWinampPlugin::SetVolume,
    CAPEWinampPlugin::SetPan,
    0, 0, 0, 0, 0, 0, 0, 0, 0,                                     // vis stuff
    0, 0,                                                          // dsp stuff
    0,                                                             // Set_EQ function
    NULL,                                                          // setinfo
    0                                                              // out_mod
};

/**************************************************************************************************
Global variables -- shoot me now
**************************************************************************************************/
TCHAR CAPEWinampPlugin::m_cCurrentFilename[APE_MAX_PATH] = { 0 };
int CAPEWinampPlugin::m_nDecodePositionMS = -1;
int CAPEWinampPlugin::m_nPaused = 0;
int CAPEWinampPlugin::m_nSeekNeeded = -1;
int CAPEWinampPlugin::m_nKillDecodeThread = 0;
HANDLE CAPEWinampPlugin::m_hDecodeThread = INVALID_HANDLE_VALUE;
long CAPEWinampPlugin::m_nScaledBitsPerSample = 0;
long CAPEWinampPlugin::m_nScaledBytesPerSample = 0;
CSmartPtr<IAPEDecompress> CAPEWinampPlugin::m_spAPEDecompress;
long CAPEWinampPlugin::m_nLengthMS = 0;
CWinampSettingsDlg CAPEWinampPlugin::m_WinampSettingsDlg;

/**************************************************************************************************
Plays a file (called once on the start of a file)
**************************************************************************************************/
int CAPEWinampPlugin::Play(char * pFilename)
{
    // reset or initialize any public variables
    CSmartPtr<str_utfn> spFilename(CAPECharacterHelper::GetUTF16FromANSI(pFilename), TRUE);
    _tcscpy_s(m_cCurrentFilename, APE_MAX_PATH, spFilename);

    m_nPaused = 0;
    m_nDecodePositionMS = 0;
    m_nSeekNeeded = -1;

    // open the file
    int nErrorCode = 0;
    m_spAPEDecompress.Assign(CreateIAPEDecompress(m_cCurrentFilename, &nErrorCode, true, true, false));
    if ((m_spAPEDecompress == NULL) || (nErrorCode != ERROR_SUCCESS))
        return -1;

    // quit if it's a zero length file
    if (m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS) == 0)
    {
        m_spAPEDecompress.Delete();
        return -1;
    }

    // version check
    if (m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION) > APE_FILE_VERSION_NUMBER)
    {
        TCHAR cAPEFileVersion[32]; _stprintf_s(cAPEFileVersion, 32, _T("%.2f"), static_cast<double>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)) / static_cast<double>(1000));

        TCHAR cMessage[1024];
        _stprintf_s(cMessage, 1024, _T("You are attempting to play an APE file that was encoded with a version of Monkey's Audio which is newer than the installed APE plug-in.  There is a very high likelyhood that this will not work properly.  Please download and install the newest Monkey's Audio plug-in to remedy this problem.\r\n\r\nPlug-in version: %s\r\nAPE file version: %s"), 
            APE_VERSION_STRING, cAPEFileVersion);
        ::MessageBox(g_APEWinampPluginModule.hMainWindow, cMessage, _T("Update APE Plugin"), MB_OK | MB_ICONERROR);
    }

    // see if it's a stream
    g_APEWinampPluginModule.is_seekable = TRUE;

    // set the "scaled" bps
    if (GetSettings()->m_bScaleOutput == TRUE)
    {
        m_nScaledBitsPerSample = SCALED_BITS;
        m_nScaledBytesPerSample = (SCALED_BITS / 8);
    }
    else
    {
        m_nScaledBitsPerSample = static_cast<long>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE));
        m_nScaledBytesPerSample = static_cast<long>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BYTES_PER_SAMPLE));
    }

    // set the length
    double dBlocks = static_cast<double>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS));
    double dMilliseconds = (dBlocks * static_cast<double>(1000)) / static_cast<double>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE));
    m_nLengthMS = static_cast<long>(dMilliseconds);

    // open the output module
    int nMaxLatency = g_APEWinampPluginModule.outMod->Open(static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), m_nScaledBitsPerSample, -1,-1);
    if (nMaxLatency < 0)
    {
        m_spAPEDecompress.Delete();
        return -1;
    }

    // initialize the visualization stuff
    g_APEWinampPluginModule.SAVSAInit(nMaxLatency, static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)));
    g_APEWinampPluginModule.VSASetInfo(static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)));

    // set the default volume
    g_APEWinampPluginModule.outMod->SetVolume(-666);

    // set the Winamp info (bitrate, channels, etc.)
    g_APEWinampPluginModule.SetInfo(static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_AVERAGE_BITRATE)), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE) / 1000), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), 1);

    // create the new thread
    m_nKillDecodeThread = 0;
    unsigned long nThreadID;
    m_hDecodeThread = static_cast<HANDLE>(CreateThread(NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(DecodeThread), static_cast<void *>(&m_nKillDecodeThread), 0, &nThreadID));
    if (m_hDecodeThread == NULL)
    {
        m_spAPEDecompress.Delete();
        return -1;
    }

    // set the thread priority
    if (SetThreadPriority(m_hDecodeThread, GetSettings()->m_nThreadPriority) == 0)
    {
        m_spAPEDecompress.Delete();
        return -1;
    }

    return 0;
}

/**************************************************************************************************
Stops the file (called anytime a file is stopped, or restarted)
**************************************************************************************************/
void CAPEWinampPlugin::Stop()
{
    // if the decode thread is active, kill it
    if (m_hDecodeThread != INVALID_HANDLE_VALUE)
    {
        // set the flag to kill the thread and then wait
        m_nKillDecodeThread = 1;

        // this used to wait INFINITE, but then check for WAIT_TIMEOUT and error message and kill the thread
        // that doesn't make sense to me because how could it ever timeout on an infinite wait
        // so we just removed the return value check instead
        WaitForSingleObject(m_hDecodeThread, INFINITE);

        // close the thread
        CloseHandle(m_hDecodeThread);
        m_hDecodeThread = INVALID_HANDLE_VALUE;
    }

    // cleanup global objects
    m_spAPEDecompress.Delete();
    m_spAPEDecompress.Delete();

    // close the output module
    g_APEWinampPluginModule.outMod->Close();

    // uninitialize the visualization stuff
    g_APEWinampPluginModule.SAVSADeInit();
}

/**************************************************************************************************
Check a buffer for silence
**************************************************************************************************/
BOOL CAPEWinampPlugin::CheckBufferForSilence(void * pBuffer, const uint32 nSamples)
{
    uint32 nSum = 0;

    if (m_nScaledBitsPerSample == 8)
    {
        unsigned char * pData = static_cast<unsigned char *>(pBuffer);
        for (uint32 z = 0; z < nSamples; z++, pData++)
            nSum += static_cast<uint32>(abs(*pData - 128));

        nSum <<= 8;
    }
    else if (m_nScaledBitsPerSample == 16)
    {
        int16 * pData = static_cast<int16 *>(pBuffer);
        for (uint32 z = 0; z < nSamples; z++, pData++)
            nSum += static_cast<uint32>(abs(*pData));
    }

    nSum /= ape_max(nSamples, 1);

    if (nSum > 64)
        return FALSE;
    else
        return TRUE;
}

/**************************************************************************************************
Scale a buffer
**************************************************************************************************/
long CAPEWinampPlugin::ScaleBuffer(IAPEDecompress * pAPEDecompress, unsigned char * pBuffer, long nBlocks)
{
    if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE) == 8)
    {
        unsigned char * pBuffer8 = &pBuffer[nBlocks * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) - 1];
        __int16 * pBuffer16 = reinterpret_cast<__int16 *>(&pBuffer[nBlocks * pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * 2 - 2]);
        while (pBuffer8 >= pBuffer)
        {
            *pBuffer16-- = static_cast<__int16>((long(*pBuffer8--) - 128) << 8);
        }
    }
    else if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE) == 24)
    {
        unsigned char * pBuffer24 = pBuffer;
        __int16 * pBuffer16 = reinterpret_cast<__int16 *>(pBuffer);
        long nElements = nBlocks * long(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS));
        for (long z = 0; z < nElements; z++, pBuffer16++, pBuffer24 += 3)
        {
            *pBuffer16 = static_cast<__int16>(*(reinterpret_cast<long *>(pBuffer24)) >> 8);
        }
    }

    return 0;
}

long CAPEWinampPlugin::ScaleFloatBuffer(IAPEDecompress * pAPEDecompress, unsigned char * pBuffer, long nBlocks)
{
    if (pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE) == 32)
    {
        float * pBufferFloat = reinterpret_cast<float *>(pBuffer);
        __int32 * pBuffer32 = reinterpret_cast<__int32 *>(pBuffer);
        long nElements = nBlocks * long(pAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS));
        for (long z = 0; z < nElements; z++, pBuffer32++, pBufferFloat++)
        {
            float fValue = *pBufferFloat;
            fValue *= 2147483647.0f;
            *pBuffer32 = static_cast<__int32>(fValue);
        }
    }

    return 0;
}

/**************************************************************************************************
The decode thread
**************************************************************************************************/
DWORD WINAPI __stdcall CAPEWinampPlugin::DecodeThread(void * pbKillSwitch)
{
    // variable declares
    BOOL bDone = FALSE;
    long nSilenceMS = 0;

    // the sample buffer...must be able to hold twice the original 1152 samples for DSP
    CSmartPtr<unsigned char> spSampleBuffer(new unsigned char [static_cast<unsigned int>(1152 * 2 * m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS) * m_nScaledBytesPerSample)], TRUE);

    // start the decoding loop
    while (!*(reinterpret_cast<int *>(pbKillSwitch)))
    {
        // seek if necessary
        if (m_nSeekNeeded != -1)
        {
            // update the decode position and reset the seek needed flag
            m_nDecodePositionMS = m_nSeekNeeded;
            m_nSeekNeeded = -1;

            // need to use doubles to avoid overflows (at around 10 minutes)
            double dLengthMS = (static_cast<double>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS)) * static_cast<double>(1000)) / static_cast<double>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE));
            double dSeekPercentage = static_cast<double>(m_nDecodePositionMS) / dLengthMS;
            double dSeekBlock = static_cast<double>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS)) * dSeekPercentage;

            // seek
            int64 nSeekBlock = static_cast<int64>(dSeekBlock + 0.5);
            m_spAPEDecompress->Seek(nSeekBlock);

            // flush out the output module of data already in it
            g_APEWinampPluginModule.outMod->Flush(m_nDecodePositionMS);

            Sleep(20);
        }

        // quit if the 'bDone' flag is set
        if (bDone)
        {
            g_APEWinampPluginModule.outMod->CanWrite();
            if (!g_APEWinampPluginModule.outMod->IsPlaying())
            {
                PostMessage(g_APEWinampPluginModule.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
                return 0;
            }

            Sleep(10);
        }
        // write data into the output stream if there is enough room for one full sample buffer
        else if (g_APEWinampPluginModule.outMod->CanWrite() >= (((576 * m_nScaledBytesPerSample) * static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS))) << (g_APEWinampPluginModule.dsp_isactive() ? 1 : 0)))
        {
            // decompress the data
            int64 nBlocksDecoded = 0;
            BOOL bSynched = TRUE;
            try
            {
                // get the data
                if (m_spAPEDecompress->GetData(spSampleBuffer, 576, &nBlocksDecoded) != ERROR_SUCCESS)
                    throw(1);
            }
            catch(...)
            {
                bSynched = FALSE;

                if (GetSettings()->m_bIgnoreBitstreamErrors == FALSE)
                {
                    TCHAR cErrorTime[64];
                    int nSeconds = static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_CURRENT_MS) / 1000); int nMinutes = nSeconds / 60; nSeconds = nSeconds % 60; int nHours = nMinutes / 60; nMinutes = nMinutes % 60;
                    if (nHours > 0)    _stprintf_s(cErrorTime, 64, _T("%d:%02d:%02d"), nHours, nMinutes, nSeconds);
                    else if (nMinutes > 0) _stprintf_s(cErrorTime, 64, _T("%d:%02d"), nMinutes, nSeconds);
                    else _stprintf_s(cErrorTime, 64, _T("0:%02d"), nSeconds);

                    TCHAR cErrorMessage[1024];
                    _stprintf_s(
                        cErrorMessage,
                        1024,
                        _T("Monkey's Audio encountered an error at %s while decompressing the file '%s'.\r\n\r\n")
                        _T("Please ensure that you are using the latest version of Monkey's Audio.  ")
                        _T("If this error persists using the latest version of Monkey's Audio, it is likely that the file has become corrupted.\r\n\r\n")
                        _T("Use the option 'Ignore Bitstream Errors' in the plug-in settings to not recieve this warning when Monkey's Audio encounters an error while decompressing."),
                        cErrorTime, m_cCurrentFilename
                    );

                    MessageBox(g_APEWinampPluginModule.hMainWindow, cErrorMessage, _T("Monkey's Audio Decompression Error"), MB_OK | MB_ICONERROR);

                    bDone = TRUE;
                    continue;
                }
            }

            // set the done flag if there was nothing decompressed
            if (nBlocksDecoded == 0)
            {
                bDone = TRUE;
                continue;
            }

            // do any MAC processing
            if (GetSettings()->m_bScaleOutput == TRUE)
            {
                ScaleBuffer(m_spAPEDecompress, spSampleBuffer, long(nBlocksDecoded));
            }
            else if (m_spAPEDecompress->GetInfo(APE::IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_FLOATING_POINT)
            {
                ScaleFloatBuffer(m_spAPEDecompress, spSampleBuffer, long(nBlocksDecoded));
            }

            int64 nBytesDecodedN = nBlocksDecoded * m_nScaledBytesPerSample * m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS);
            long nBytesDecoded = long(nBytesDecodedN);

            // pass the samples through the dsp if it's running
            if (g_APEWinampPluginModule.dsp_isactive())
            {
                nBlocksDecoded = g_APEWinampPluginModule.dsp_dosamples(reinterpret_cast<short *>(spSampleBuffer.GetPtr()), static_cast<int>(nBlocksDecoded), m_nScaledBitsPerSample, static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)));
                nBytesDecoded = static_cast<long>(nBlocksDecoded) * m_nScaledBytesPerSample * static_cast<long>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS));
            }

            if (GetSettings()->m_bSuppressSilence == TRUE)
            {
                if (CheckBufferForSilence(spSampleBuffer, static_cast<uint32>(nBytesDecoded / m_nScaledBytesPerSample)) == FALSE)
                    nSilenceMS = 0;
                else
                    nSilenceMS += static_cast<long>(nBlocksDecoded * 1000) / static_cast<long>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE));
            }
            else
            {
                nSilenceMS = 0;
            }

            if (nSilenceMS < 1000)
            {
                // add the data to the visualization
                g_APEWinampPluginModule.SAAddPCMData(reinterpret_cast<char *>(spSampleBuffer.GetPtr()), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), m_nScaledBitsPerSample, m_nDecodePositionMS);
                g_APEWinampPluginModule.VSAAddPCMData(reinterpret_cast<char *>(spSampleBuffer.GetPtr()), static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)), m_nScaledBitsPerSample, m_nDecodePositionMS);

                // write the data to the output stream
                g_APEWinampPluginModule.outMod->Write(reinterpret_cast<char *>(spSampleBuffer.GetPtr()), nBytesDecoded);
            }
            else
            {
                bSynched = FALSE;
            }

            // update the VBR display
            g_APEWinampPluginModule.SetInfo(static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_CURRENT_BITRATE)), -1, -1, bSynched);

            // increment the decode position
            m_nDecodePositionMS += (static_cast<int>(nBlocksDecoded * 1000) / static_cast<int>(m_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)));
        }

        // if it wasn't done, and there wasn't room in the output stream, just wait and try again
        else
        {
            Sleep(20);
        }

        Sleep(0);
    }

    return 0;
}

/**************************************************************************************************
Returns the length of the current file in ms
**************************************************************************************************/
int CAPEWinampPlugin::GetFileLength()
{
    int nRetVal = m_nLengthMS;

    return nRetVal;
}

/**************************************************************************************************
Returns the output time in ms
**************************************************************************************************/
int CAPEWinampPlugin::GetOutputTime()
{
    int nRetVal = m_nDecodePositionMS + (g_APEWinampPluginModule.outMod->GetOutputTime() - g_APEWinampPluginModule.outMod->GetWrittenTime());
    return nRetVal;
}

/**************************************************************************************************
Sets the output time in ms
**************************************************************************************************/
void CAPEWinampPlugin::SetOutputTime(int nNewPositionMS)
{
    m_nSeekNeeded = nNewPositionMS;
}

/**************************************************************************************************
Show the the file info dialog
**************************************************************************************************/
int CAPEWinampPlugin::ShowFileInformationDialog(char *, HWND)
{
    // we now use the default dialog
    return 0;
}

/**************************************************************************************************
File info helpers
**************************************************************************************************/
void CAPEWinampPlugin::BuildDescriptionStringFromFilename(CString & strBuffer, const str_utfn * pFilename)
{
    const str_utfn * p = pFilename + _tcslen(pFilename);
    while (*p != '\\' && p >= pFilename)
        p--;

    strBuffer = ++p;

    if (strBuffer.GetLength() >= 4)
        strBuffer = strBuffer.Left(strBuffer.GetLength() - 4);
}

void CAPEWinampPlugin::BuildDescriptionString(CString & strBuffer, IAPETag * pAPETag, const str_utfn * pFilename)
{
    if (pAPETag == NULL)
    {
        BuildDescriptionStringFromFilename(strBuffer, pFilename);
        return;
    }

    if (pAPETag->GetHasID3Tag() == FALSE && pAPETag->GetHasAPETag() == FALSE)
    {
        BuildDescriptionStringFromFilename(strBuffer, pFilename);
        return;
    }

    TCHAR cBuffer[256];
    int nBufferBytes;

    #define REPLACE_TOKEN_WITH_TAG_FIELD(TAG_FIELD, TOKEN) \
        nBufferBytes = 256; \
        pAPETag->GetFieldString(TAG_FIELD, cBuffer, &nBufferBytes); \
        strBuffer.Replace(TOKEN, cBuffer);

    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_ARTIST, _T("%1"))
    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_TITLE, _T("%2"))
    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_ALBUM, _T("%3"))
    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_YEAR, _T("%4"))
    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_COMMENT, _T("%5"))
    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_GENRE, _T("%6"))
    REPLACE_TOKEN_WITH_TAG_FIELD(APE_TAG_FIELD_TRACK, _T("%7"))

    TCHAR * p = m_cCurrentFilename + _tcslen(m_cCurrentFilename);
    while (*p != '\\' && p >= m_cCurrentFilename)
        p--;
    strBuffer.Replace(_T("%8"), ++p);

    strBuffer.Replace(_T("%9"), m_cCurrentFilename);
}

/**************************************************************************************************
Get the file info
**************************************************************************************************/
void CAPEWinampPlugin::GetFileInformation(char * pFilename, char * pTitle, int * pLengthMS)
{
    CSmartPtr<IAPEDecompress> spAPEDecompress;
    CString strFilename;
    if (!pFilename || !*pFilename)
    {
        // currently playing file
        spAPEDecompress.Assign(m_spAPEDecompress, FALSE, FALSE);
        strFilename = m_cCurrentFilename;
    }
    else
    {
        // different file
        CSmartPtr<wchar_t> spUTF16(CAPECharacterHelper::GetUTF16FromANSI(pFilename), TRUE);
        spAPEDecompress.Assign(CreateIAPEDecompress(spUTF16, NULL, true, true, false));
        strFilename = spUTF16;
    }

    if (spAPEDecompress != NULL)
    {
        if (pLengthMS)
        {
            *pLengthMS = static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_LENGTH_MS));
        }

        if (pTitle)
        {
            CString strDisplay = GetSettings()->m_strFileDisplayMethod;
            BuildDescriptionString(strDisplay, GET_TAG(spAPEDecompress), strFilename);

            CSmartPtr<char> spDisplayANSI(CAPECharacterHelper::GetANSIFromUTF16(strDisplay), TRUE);
            strncpy_s(pTitle, GETFILEINFO_TITLE_LENGTH, spDisplayANSI, _TRUNCATE);
        }
    }
}

/**************************************************************************************************
Displays the configuration dialog
**************************************************************************************************/
void CAPEWinampPlugin::ShowConfigurationDialog(HWND hwndParent)
{
    GetSettings()->Show(hwndParent);
}

/**************************************************************************************************
Show the about dialog
**************************************************************************************************/
void CAPEWinampPlugin::ShowAboutDialog(HWND hwndParent)
{
    MessageBox(hwndParent, PLUGIN_ABOUT, _T("Monkey's Audio Player"), MB_OK);
}

/**************************************************************************************************
Set the volume
**************************************************************************************************/
void CAPEWinampPlugin::SetVolume(int volume)
{
    g_APEWinampPluginModule.outMod->SetVolume(volume);
}

/**************************************************************************************************
Pause
**************************************************************************************************/
void CAPEWinampPlugin::Pause()
{
    m_nPaused = 1;
    g_APEWinampPluginModule.outMod->Pause(1);
}

/**************************************************************************************************
Unpause
**************************************************************************************************/
void CAPEWinampPlugin::Unpause()
{
    m_nPaused = 0;
    g_APEWinampPluginModule.outMod->Pause(0);
}

/**************************************************************************************************
Checks to see if it is currently m_nPaused
**************************************************************************************************/
int CAPEWinampPlugin::IsPaused()
{
    return m_nPaused;
}

/**************************************************************************************************
Set the pan
**************************************************************************************************/
void CAPEWinampPlugin::SetPan(int pan)
{
    g_APEWinampPluginModule.outMod->SetPan(pan);
}

/**************************************************************************************************
Initialize the plugin (called once on the close of Winamp)
**************************************************************************************************/
void CAPEWinampPlugin::InitializePlugin()
{
    Wasabi_Init();
}

/**************************************************************************************************
Uninitialize the plugin (called once on the close of Winamp)
**************************************************************************************************/
void CAPEWinampPlugin::UninitializePlugin()
{
    // delete the decompress object
    CAPEWinampPlugin::m_spAPEDecompress.Delete();
}

/**************************************************************************************************
Is our file (used for detecting URL streams)
**************************************************************************************************/
int CAPEWinampPlugin::IsOurFile(char *) { return 0; }

/**************************************************************************************************
Get the settings
**************************************************************************************************/
CWinampSettingsDlg * CAPEWinampPlugin::GetSettings()
{
    return &CAPEWinampPlugin::m_WinampSettingsDlg;
}

/**************************************************************************************************
Exported functions
**************************************************************************************************/
extern "C"
{
    // use ugly statics since Winamp is built around global variables
    static CString s_strFilename;
    static CSmartPtr<IAPEDecompress> s_spAPEDecompress;
    static HANDLE s_hTimer = NULL;
    static CCriticalSection s_Lock;

    // forward declares to avoid Clang warnings
    __declspec(dllexport) int winampUseUnifiedFileInfoDlg(const wchar_t *);
    __declspec(dllexport) In_Module * winampGetInModule2();
    __declspec(dllexport) int winampGetExtendedFileInfo(extendedFileInfoStruct Info);
    __declspec(dllexport) int winampWriteExtendedFileInfo(void);
    __declspec(dllexport) int winampSetExtendedFileInfo(const char * filename, const char * metadata, char * val);
    int APE_GetAlbumArt(const wchar_t * filename, const wchar_t * type, void ** bits, size_t * len, wchar_t ** mime_type);
    int APE_SetAlbumArt(const wchar_t * filename, const wchar_t * type, void * bits, size_t len, const wchar_t * mime_type);
    int APE_DeleteAlbumArt(const wchar_t * filename, const wchar_t * type);

    // timer for destruction (since Winamp doesn't let us know when it finishes, we just have to use a timer)
    static void KillMediaTimer()
    {
        CSingleLock Lock(&s_Lock);
        if (s_hTimer != NULL)
        {
            BOOL bResult = DeleteTimerQueueTimer(NULL, s_hTimer, NULL);
            if (bResult == 0)
            {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_IO_PENDING)
                {
                    // this means it's working, and will cancel later
                    // this is because we call KillMediaTimer in the TimerProc itself
                    bResult = true;
                }
            }

            // clear the timer
            if (bResult)
                s_hTimer = NULL;
        }
    }

    static void CALLBACK TimerProc(void *, BOOLEAN)
    {
        // check if the timer has already been deleted and don't run in that case
        CSingleLock Lock(&s_Lock);
        if (s_hTimer != NULL)
        {
            s_spAPEDecompress.Delete();
            KillMediaTimer();
        }
    }

    static void UnloadFileAfterDelay()
    {
        CSingleLock Lock(&s_Lock);
        KillMediaTimer();
        CreateTimerQueueTimer(&s_hTimer, NULL, static_cast<WAITORTIMERCALLBACK>(TimerProc), NULL, 3000, 0, WT_EXECUTEINTIMERTHREAD);
    }

    // return 1 if you want winamp to show it's own file info dialogue, 0 if you want to show your own (via In_Module.InfoBox)
    // if returning 1, remember to implement winampGetExtendedFileInfo("formatinformation")!
    __declspec(dllexport) int winampUseUnifiedFileInfoDlg(const wchar_t *)
    {
        return 1;
    }

    __declspec(dllexport) In_Module * winampGetInModule2()
    {
        return &g_APEWinampPluginModule;
    }

    __declspec(dllexport) int winampSetExtendedFileInfo(const char * filename, const char * metadata, char * val)
    {
        CSingleLock Lock(&s_Lock);

        // load the file
        CSmartPtr<wchar_t> spUTF16(CAPECharacterHelper::GetUTF16FromANSI(filename), TRUE);
        if ((s_spAPEDecompress == NULL) || (spUTF16 != s_strFilename))
        {
            s_spAPEDecompress.Assign(CreateIAPEDecompress(spUTF16, NULL, false, true, false));
            s_strFilename = spUTF16;
        }

        IAPETag * pTag = GET_TAG(s_spAPEDecompress);
        if (pTag != NULL)
        {
            CSmartPtr<wchar_t> spValue(CAPECharacterHelper::GetUTF16FromANSI(val), TRUE);

            if (strcmp(metadata, "artist") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_ARTIST, spValue);
            else if (strcmp(metadata, "album") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_ALBUM, spValue);
            else if (strcmp(metadata, "title") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_TITLE, spValue);
            else if (strcmp(metadata, "comment") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_COMMENT, spValue);
            else if (strcmp(metadata, "year") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_YEAR, spValue);
            else if (strcmp(metadata, "genre") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_GENRE, spValue);
            else if (strcmp(metadata, "track") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_TRACK, spValue);
            else if (strcmp(metadata, "disc") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_DISC, spValue);
            else if (strcmp(metadata, "albumartist") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_ALBUM_ARTIST, spValue);
            else if (strcmp(metadata, "composer") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_COMPOSER, spValue);
            else if (strcmp(metadata, "publisher") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_PUBLISHER, spValue);
            else if (strcmp(metadata, "bpm") == 0)
                pTag->SetFieldString(APE_TAG_FIELD_BPM, spValue);
        }

        return 1;
    }

    __declspec(dllexport) int winampWriteExtendedFileInfo(void)
    {
        CSingleLock Lock(&s_Lock);

        int nResult = 0;
        if (s_spAPEDecompress != NULL)
        {
            IAPETag * pTag = GET_TAG(s_spAPEDecompress);
            if (pTag != NULL)
            {
                if (pTag->Save(false) == ERROR_SUCCESS)
                    nResult = 1;
            }
            KillMediaTimer(); // stop the timer since we're already deleting
            s_spAPEDecompress.Delete();
            s_strFilename.Empty();
        }
        return nResult;
    }

    __declspec(dllexport) int winampGetExtendedFileInfo(extendedFileInfoStruct Info)
    {
        CSingleLock Lock(&s_Lock);
        KillMediaTimer(); // stop the cleanup timer (or else it can run while we're working)

        // on startup, type is queried for
        if (((Info.pFilename == NULL) || (Info.pFilename[0] == 0)) &&
            (strcmp(Info.pMetaData, "type") == 0))
        {
            strcpy_s(Info.pReturn, static_cast<size_t>(Info.nReturnBytes), "ape");
            return 1;
        }

        // load the file
        CSmartPtr<wchar_t> spUTF16(CAPECharacterHelper::GetUTF16FromANSI(Info.pFilename), TRUE);
        if ((s_spAPEDecompress == NULL) || (spUTF16 != s_strFilename))
        {
            s_spAPEDecompress.Assign(CreateIAPEDecompress(spUTF16, NULL, false, true, false));
            s_strFilename = spUTF16;
        }

        // get the tag value
        IAPETag * pTag = GET_TAG(s_spAPEDecompress);
        if (pTag != APE_NULL)
        {
            if (strcmp(Info.pMetaData, "artist") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_ARTIST, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "album") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_ALBUM, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "title") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_TITLE, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "type") == 0)
            {
                strcpy_s(Info.pReturn, static_cast<size_t>(Info.nReturnBytes), "ape");
            }
            else if (strcmp(Info.pMetaData, "comment") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_COMMENT, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "year") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_YEAR, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "genre") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_GENRE, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "length") == 0)
            {
                int64 nLength = s_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_LENGTH_MS);
                sprintf_s(Info.pReturn, static_cast<size_t>(Info.nReturnBytes), "%I64d", nLength);
            }
            else if (strcmp(Info.pMetaData, "track") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_TRACK, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "disc") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_DISC, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "albumartist") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_ALBUM_ARTIST, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "composer") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_COMPOSER, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "publisher") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_PUBLISHER, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (strcmp(Info.pMetaData, "bpm") == 0)
            {
                pTag->GetFieldString(APE_TAG_FIELD_BPM, Info.pReturn, &Info.nReturnBytes, FALSE);
            }
            else if (_stricmp(Info.pMetaData, "formatinformation") == 0)
            {
                CString strFormat;
                CString strLine;

                // get the compression level
                str_utfn cCompressionLevel[256]; APE_CLEAR(cCompressionLevel);
                GetAPECompressionLevelName(static_cast<int>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL)), cCompressionLevel, 256, false);

                // overall
                strLine.Format(_T("Monkey's Audio %.2f (%s)"),
                    static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)) / static_cast<double>(1000), cCompressionLevel);
                strFormat += strLine + _T("\r\n");

                // format
                strLine.Format(_T("Format: %.1f khz, %d bit, %d ch"),
                    static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)) / static_cast<double>(1000),
                    static_cast<int>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE)),
                    static_cast<int>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)));
                if (s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_AIFF)
                    strLine += _T(", AIFF");
                strFormat += strLine + _T("\r\n");

                // length
                strLine.Format(_T("Length: %s (%I64d blocks)"),
                    static_cast<LPCTSTR>(FormatDuration(static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_LENGTH_MS)) / 1000.0, FALSE)),
                    s_spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS));
                strFormat += strLine + _T("\r\n");

                // the file size
                strLine.Format(_T("APE: %.2f MB"), static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APE_TOTAL_BYTES)) / static_cast<double>(1048576));
                strFormat += strLine + _T("\r\n");

                strLine.Format(_T("WAV: %.2f MB"), static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TOTAL_BYTES)) / static_cast<double>(1048576));
                strFormat += strLine + _T("\r\n");

                // the compression ratio
                strLine.Format(_T("Compression: %.2f%%"), static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_AVERAGE_BITRATE) * 100) / static_cast<double>(s_spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_DECOMPRESSED_BITRATE)));
                strFormat += strLine + _T("\r\n");

                str_ansi * pFormatANSI = CAPECharacterHelper::GetANSIFromUTF16(strFormat);
                strncpy_s(Info.pReturn, static_cast<size_t>(Info.nReturnBytes), pFormatANSI, static_cast<size_t>(Info.nReturnBytes));
                delete [] pFormatANSI;
            }
            else
            {
                Info.pReturn[0] = 0;
            }

            // unload the file after a delay
            UnloadFileAfterDelay();

            return 1;
        }

        // no tag or file, so fail
        return 0;
    }
}

/**************************************************************************************************
Album art
**************************************************************************************************/
extern "C" int APE_GetAlbumArt(const wchar_t * filename, const wchar_t * type, void ** bits, size_t * len, wchar_t ** mime_type)
{
    // error check
    if (!filename || !*filename || _wcsicmp(type, L"cover"))
        return -1;

    // open file
    int nResult = -1;

    // decompress object
    CSmartPtr<IAPEDecompress> spAPEDecompress;

    // load the file
    spAPEDecompress.Assign(CreateIAPEDecompress(filename, NULL, false, true, false));

    // get the tag
    IAPETag * pTag = GET_TAG(spAPEDecompress);
    if (pTag != NULL)
    {
        CAPETagField * pTagImage = pTag->GetTagField(APE_TAG_FIELD_COVER_ART_FRONT);
        if ((pTagImage != NULL) && (pTagImage->GetFieldSize() > 0))
        {
            *len = static_cast<size_t>(pTagImage->GetFieldSize());
            CSmartPtr<BYTE> spBuffer(new BYTE[*len], true);

            int nBufferBytes = static_cast<int>(*len);
            if (pTag->GetFieldBinary(APE_TAG_FIELD_COVER_ART_FRONT, spBuffer, &nBufferBytes) == ERROR_SUCCESS)
            {
                BYTE * pImage = NULL; int nImageBytes = -1;
                for (int nSearch = 0; nSearch < nBufferBytes; nSearch++)
                {
                    if (spBuffer[nSearch] == 0)
                    {
                        pImage = &spBuffer[nSearch + 1];
                        nImageBytes = nBufferBytes - nSearch - 1;
                        break;
                    }
                }

                if (pImage != NULL)
                {
                    // load the filename
                    CString strFilename;
                    strFilename = CAPECharacterHelper::GetUTF16FromUTF8(static_cast<APE::str_utf8 *>(spBuffer));

                    // get the extension
                    CString strExtension = strFilename.Right(strFilename.GetLength() - strFilename.ReverseFind('.') - 1);

                    // get the mime type
                    *mime_type = static_cast<wchar_t *>(Wasabi_Malloc((static_cast<size_t>(strExtension.GetLength()) + 1) * 2));
                    memcpy(*mime_type, strExtension.GetString(), (static_cast<size_t>(strExtension.GetLength()) + 1) * 2);

                    // copy the data
                    *len = static_cast<size_t>(nImageBytes);
                    *bits = Wasabi_Malloc(static_cast<size_t>(nImageBytes));
                    memcpy(*bits, pImage, static_cast<size_t>(nImageBytes));

                    // success
                    nResult = 0;
                }
            }
        }
    }

    return nResult;
}

extern "C" int APE_SetAlbumArt(const wchar_t * filename, const wchar_t * type, void * /*bits*/, size_t /*len*/, const wchar_t * /*mime_type*/)
{
    // error check
    if (!filename || !*filename || _wcsicmp(type, L"cover"))
        return -1;

    // open file
    int nResult = -1;

    // decompress object
    CSmartPtr<IAPEDecompress> spAPEDecompress;

    // load the file
    spAPEDecompress.Assign(CreateIAPEDecompress(filename, NULL, false, true, false));

    // we don't currently support saving art, maybe someday...
    // if anyone can help me figure out how to make this run in the debugger, please share

    return nResult;
}

extern "C" int APE_DeleteAlbumArt(const wchar_t * filename, const wchar_t * type)
{
    // error check
    if (!filename || !*filename || _wcsicmp(type, L"cover"))
        return -1;

    // open file
    int nResult = -1;

    // decompress object
    CSmartPtr<IAPEDecompress> spAPEDecompress;

    // load the file
    spAPEDecompress.Assign(CreateIAPEDecompress(filename, NULL, false, true, false));

    IAPETag * pTag = GET_TAG(spAPEDecompress);
    if (pTag != NULL)
    {
        // remove cover art
        pTag->RemoveField(APE_TAG_FIELD_COVER_ART_FRONT);

        // save
        pTag->Save();

        // success
        nResult = 0;
    }

    return nResult;
}
