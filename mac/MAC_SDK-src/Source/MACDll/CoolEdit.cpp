#include "stdafx.h"
#include <math.h>
#include <stdio.h>
#include "Filters.h"
#include "resource.h"
#include "All.h"
#include "APEInfo.h"
#include "APECompress.h"
#include "CharacterHelper.h"

using namespace APE;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Forward declares to avoid Clang warnings
///////////////////////////////////
__declspec(dllexport) short FAR PASCAL QueryCoolFilter(COOLQUERY far * cq);
__declspec(dllexport) BOOL FAR PASCAL FilterUnderstandsFormat(LPSTR filename);
__declspec(dllexport) INT_PTR FAR PASCAL DIALOGMsgProc(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam);
__declspec(dllexport) DWORD FAR PASCAL FilterGetOptions(HWND hWnd, HINSTANCE hInst, long, WORD, WORD, DWORD dwOptions);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//QueryCoolFilter: Setup the filter
///////////////////////////////////
__declspec(dllexport) short FAR PASCAL QueryCoolFilter(COOLQUERY far * cq)
{
    strcpy_s(cq->szName, 24, "Monkey's Audio");
    strcpy_s(cq->szCopyright, 80, "Monkey's Audio file");
    strcpy_s(cq->szExt, 4, "APE");
    strcpy_s(cq->szExt2, 4, "MAC");

    cq->lChunkSize=1;
    cq->dwFlags=QF_READSPECIALLAST|QF_WRITESPECIALFIRST|QF_RATEADJUSTABLE|
        QF_CANSAVE|QF_CANLOAD|QF_HASOPTIONSBOX|QF_CANDO32BITFLOATS;
     cq->Stereo8=0xFF;
     cq->Stereo16=0xFF;
     cq->Stereo24=0xFF;
     cq->Stereo32=0x00;
     cq->Mono8=0xFF;
     cq->Mono16=0xFF;
     cq->Mono24=0xFF;
     cq->Mono32=0x00;

     return C_VALIDLIBRARY;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterUnderstandsFormat: Check if the file is a real .ape
///////////////////////////////////////////////////////////
__declspec(dllexport) BOOL FAR PASCAL FilterUnderstandsFormat(LPSTR filename)
{
    BOOL bValid = FALSE;

    APE_CATCH_ERRORS
    (
        CSmartPtr<wchar_t> spFilenameUTF16(CAPECharacterHelper::GetUTF16FromANSI(filename), TRUE);
        int nErrorCode = ERROR_SUCCESS;
        IAPEDecompress * pAPEDecompress = CreateIAPEDecompress(spFilenameUTF16, &nErrorCode, true, true, false);
        if (pAPEDecompress != APE_NULL)
        {
            if (nErrorCode == ERROR_SUCCESS)
            {
                bValid = TRUE;
            }
            delete pAPEDecompress;
        }
    )

    return bValid;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DIALOGMsgProc: All Dialog stuff here
//////////////////////////////////////
__declspec(dllexport) INT_PTR FAR PASCAL DIALOGMsgProc(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        ///////////////////////////////////////////////////////////////////////////////
        //Initialize Dialog
        ///////////////////
    case WM_INITDIALOG:
    {
        long nDialogReturn = 0;

        nDialogReturn = static_cast<long>(lParam);
        if (nDialogReturn == 1) CheckDlgButton(hWndDlg, IDC_R1, TRUE);
        else if (nDialogReturn == 2) CheckDlgButton(hWndDlg, IDC_R2, TRUE);
        else if (nDialogReturn == 3) CheckDlgButton(hWndDlg, IDC_R3, TRUE);
        else if (nDialogReturn == 4) CheckDlgButton(hWndDlg, IDC_R4, TRUE);
        else if (nDialogReturn == 5) CheckDlgButton(hWndDlg, IDC_R5, TRUE);
    }
    break;

    case WM_CLOSE:
        PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

        ///////////////////////////////////////////////////////////////////////////////
        //All WM_COMMAND here
        /////////////////////
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            long nDialogReturn = 0;

            if (IsDlgButtonChecked(hWndDlg, IDC_R1)) nDialogReturn = 1;
            else if (IsDlgButtonChecked(hWndDlg, IDC_R2)) nDialogReturn = 2;
            else if (IsDlgButtonChecked(hWndDlg, IDC_R3)) nDialogReturn = 3;
            else if (IsDlgButtonChecked(hWndDlg, IDC_R4)) nDialogReturn = 4;
            else if (IsDlgButtonChecked(hWndDlg, IDC_R5)) nDialogReturn = 5;

            EndDialog(hWndDlg, static_cast<short>(nDialogReturn));
        }
        break;

        case IDCANCEL:
            EndDialog(hWndDlg, FALSE);
            break;
        }
        break;
    }

    default:
        return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FilterGetOptions: Show Dialog to select Compression Level
///////////////////////////////////////////////////////////
__declspec(dllexport) DWORD FAR PASCAL FilterGetOptions(HWND hWnd, HINSTANCE hInst, long, WORD, WORD, DWORD dwOptions)
{
    DWORD dwDialogReturn = 0L;

    if (dwOptions == 0)
        dwDialogReturn = 1;
    else
        dwDialogReturn = dwOptions;

    dwDialogReturn = static_cast<DWORD>(DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_COMPRESSION), hWnd, DIALOGMsgProc, dwDialogReturn));

    return dwDialogReturn;
}
