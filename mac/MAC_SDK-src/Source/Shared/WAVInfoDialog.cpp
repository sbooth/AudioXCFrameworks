#include <windows.h>
#include "All.h"
using namespace APE;

#include "WAVInfoDialog.h"
#include "WAVInputSource.h"
#include "CharacterHelper.h"

/**************************************************************************************************
The dialog component ID's
**************************************************************************************************/
#define FILE_NAME_STATIC                1000

#define FILE_SIZE_STATIC                2000
#define TRACK_LENGTH_STATIC                2001
#define AUDIO_BYTES_STATIC                2002
#define HEADER_BYTES_STATIC                2003
#define TERMINATING_BYTES_STATIC        2004

#define SAMPLE_RATE_STATIC                3000
#define CHANNELS_STATIC                    3001
#define BITS_PER_SAMPLE_STATIC            3002

#define OK_BUTTON                        4000

/**************************************************************************************************
Global pointer to this instance
**************************************************************************************************/
CWAVInfoDialog * g_pWAVInfoDialog;


/**************************************************************************************************
Construction / destruction
**************************************************************************************************/
CWAVInfoDialog::CWAVInfoDialog()
{
    g_pWAVInfoDialog = NULL;
}

CWAVInfoDialog::~CWAVInfoDialog()
{

}

/**************************************************************************************************
Display the file info dialog
**************************************************************************************************/
long CWAVInfoDialog::ShowWAVInfoDialog(const str_utfn * pFilename, HINSTANCE hInstance, const str_utfn * lpTemplateName, HWND hWndParent)
{
    // only allow one instance at a time
    if (g_pWAVInfoDialog != NULL)
    {
        return ERROR_UNDEFINED;
    }

    _tcscpy_s(m_cFileName, MAX_PATH - 1, pFilename);
    g_pWAVInfoDialog = this;

    DialogBoxParam(hInstance, lpTemplateName, hWndParent, (DLGPROC) DialogProc, 0);
    
    g_pWAVInfoDialog = NULL;
    return ERROR_SUCCESS;
}

/**************************************************************************************************
Initialize the dialog
**************************************************************************************************/
long CWAVInfoDialog::InitDialog(HWND hDlg)
{
    // analyze the WAV
    APE::WAVEFORMATEX wfeWAV;
    int64 nTotalBlocks = 0;
    int64 nHeaderBytes = 0;
    int64 nTerminatingBytes = 0;
    int nErrorCode = 0;

    CWAVInputSource WAVInfo(m_cFileName, &wfeWAV, &nTotalBlocks, &nHeaderBytes, &nTerminatingBytes, &nErrorCode);
    if (nErrorCode != 0)
        return nErrorCode;

    int64 nAudioBytes = nTotalBlocks * wfeWAV.nBlockAlign;

    // set info
    TCHAR cTemp[1024] = { 0 };

    SetDlgItemText(hDlg, FILE_NAME_STATIC, m_cFileName);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Sample Rate: %d"), int(wfeWAV.nSamplesPerSec));
    SetDlgItemText(hDlg, SAMPLE_RATE_STATIC, cTemp);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Channels: %d"), int(wfeWAV.nChannels));
    SetDlgItemText(hDlg, CHANNELS_STATIC, cTemp);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Bits Per Sample: %d"), int(wfeWAV.wBitsPerSample));
    SetDlgItemText(hDlg, BITS_PER_SAMPLE_STATIC, cTemp);

    int64 nSeconds = nAudioBytes / wfeWAV.nAvgBytesPerSec; int64 nMinutes = nSeconds / 60; nSeconds = nSeconds % 60; int64 nHours = nMinutes / 60; nMinutes = nMinutes % 60;
    if (nHours > 0) _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Length: %I64d:%02I64d:%02I64d"), nHours, nMinutes, nSeconds);
    else if (nMinutes > 0) _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Length: %I64d:%02I64d"), nMinutes, nSeconds);
    else _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Length: 0:%02I64d"), nSeconds);
    SetDlgItemText(hDlg, TRACK_LENGTH_STATIC, cTemp);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Audio Bytes: %I64d"), nAudioBytes);
    SetDlgItemText(hDlg, AUDIO_BYTES_STATIC, cTemp);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Header Bytes: %I64d"), nHeaderBytes);
    SetDlgItemText(hDlg, HEADER_BYTES_STATIC, cTemp);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("Terminating Bytes: %I64d"), nTerminatingBytes);
    SetDlgItemText(hDlg, TERMINATING_BYTES_STATIC, cTemp);

    _stprintf_s(cTemp, _countof(cTemp) - 1, _T("File Size: %.2f MB"), double(nAudioBytes + nHeaderBytes + nTerminatingBytes) / double(1024 * 1024));
    SetDlgItemText(hDlg, FILE_SIZE_STATIC, cTemp);
    
    return 0;
}

/**************************************************************************************************
The dialog procedure
**************************************************************************************************/
LRESULT CALLBACK CWAVInfoDialog::DialogProc(HWND hDlg, UINT message, intn wParam, intn)
{
    int wmID, wmEvent;
    long RetVal;

    switch (message)
    {
        case WM_INITDIALOG:
            //fill in the info on initialization
            RetVal = g_pWAVInfoDialog->InitDialog(hDlg);
            return TRUE;
            break;
        case WM_COMMAND:
            wmID = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            switch (wmID)
            {
                case IDCANCEL: //traps the [esc] key
                    EndDialog(hDlg, 0);
                    return TRUE;
                    break;
                case OK_BUTTON:
                    EndDialog(hDlg, 0);
                    return TRUE;
                    break;
            }
            break;
        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;
            break;
    }
    
    return FALSE;
}


