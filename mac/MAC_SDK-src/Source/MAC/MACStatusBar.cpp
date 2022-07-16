#include "stdafx.h"
#include "mac.h"
#include "resource.h"
#include "MACStatusBar.h"
#include "MACFileArray.h"
#include "MACDlg.h"

#define ID_INDICATOR_STATUS              0
#define ID_INDICATOR_FILES               1
#define ID_INDICATOR_PROGRESS            2
#define ID_INDICATOR_PROGRESS_BAR        3

static UINT indicators[] =
{
    ID_INDICATOR_STATUS,
    ID_INDICATOR_FILES,
    ID_INDICATOR_PROGRESS,
    ID_INDICATOR_PROGRESS_BAR,
};

CMACStatusBar::CMACStatusBar(CMACDlg * pMACDlg)
{
    m_pMACDlg = pMACDlg;
    m_hwndParent = NULL;
    m_pTaskBarlist = NULL;
    m_bShowProgress = false;
    m_nProcessTotalMS = 0;
    m_strFreeSpaceDrive = _T("C");
    m_bDisableSleep = false;
    m_bProcessing = false;
}

CMACStatusBar::~CMACStatusBar()
{
}

BEGIN_MESSAGE_MAP(CMACStatusBar, CStatusBar)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

int CMACStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CStatusBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    // store parent
    m_hwndParent = lpCreateStruct->hwndParent;

    // set the indicators
    SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
    
    // set the panes up
    SetPaneInfo(ID_INDICATOR_STATUS, (UINT) -1, SBPS_NORMAL, theApp.GetSize(75, 0).cx);
    SetPaneInfo(ID_INDICATOR_FILES, (UINT) -1, SBPS_NORMAL, theApp.GetSize(300, 0).cx);
    SetPaneInfo(ID_INDICATOR_PROGRESS, (UINT) -1, SBPS_NORMAL, theApp.GetSize(185, 0).cx);
    SetPaneInfo(ID_INDICATOR_PROGRESS_BAR, (UINT) -1, SBPS_STRETCH, theApp.GetSize(0, 0).cx);

    // status
    UpdateProgress(0, 0);
    
    // create the progress control
    m_ctrlProgress.Create(WS_VISIBLE | WS_CHILD, CRect(0, 0, 0, 0), this, 1); 
    m_ctrlProgress.SetRange(0, 100);
    m_ctrlProgress.SetStep(1);
    m_ctrlProgress.SetPos(100);
    
    return 0;
}

void CMACStatusBar::OnSize(UINT nType, int cx, int cy) 
{
    CStatusBar::OnSize(nType, cx, cy);
    
    // position the progress bar
    CRect rectProgressBarPane; GetItemRect(ID_INDICATOR_PROGRESS_BAR, &rectProgressBarPane);
    m_ctrlProgress.SetWindowPos(&wndTop, rectProgressBarPane.left, rectProgressBarPane.top, rectProgressBarPane.Width(), rectProgressBarPane.Height(), 0); 
    m_ctrlProgress.ShowWindow(m_bShowProgress ? SW_SHOW : SW_HIDE);
}

BOOL CMACStatusBar::UpdateFiles(MAC_FILE_ARRAY * paryFiles)
{
    // analyze
    int nFiles = int(paryFiles->GetSize());
    double dInputBytes = paryFiles->GetTotalInputBytes();
    double dOutputBytes = paryFiles->GetTotalOutputBytes();

    // build text
    CString strFiles;
    if (dOutputBytes == 0)
    {
        double dMB = dInputBytes / double(BYTES_IN_MEGABYTE);
        if (dMB > 1000)
        {
            double dGB = dInputBytes / double(BYTES_IN_GIGABYTE);
            strFiles.Format(_T("\t%d file%s (%.2f GB)"),
                nFiles, (nFiles == 1) ? _T("") : _T("s"),
                dGB);
        }
        else
        {
            strFiles.Format(_T("\t%d file%s (%.2f MB)"),
                nFiles, (nFiles == 1) ? _T("") : _T("s"),
                dMB);
        }
    }
    else
    {
        strFiles.Format(_T("\t%d file%s (%.2f MB / %.2f MB [%.2f%%])"),
            nFiles, (nFiles == 1) ? _T("") : _T("s"),
            double(dOutputBytes / (BYTES_IN_MEGABYTE)), double(dInputBytes / (BYTES_IN_MEGABYTE)),
            (dInputBytes > 0) ? (100 * double(dOutputBytes / dInputBytes)) : double(0.0));
    }

    // set text
    SetPaneText(ID_INDICATOR_FILES, strFiles);

    // size
    SizeStatusbar();

    return TRUE;
}

BOOL CMACStatusBar::UpdateProgress(double dProgress, double dSecondsLeft)
{
    bool bProcessing = false;
    if ((dProgress == 0) && (dSecondsLeft == 0))
    {
        SetPaneText(ID_INDICATOR_STATUS, _T("\tReady"));
        
        CString strProgress;
        strProgress.Format(_T("\tTotal Time: %s"), (LPCTSTR) FormatDuration(double(m_nProcessTotalMS) / 1000, TRUE));
        SetPaneText(ID_INDICATOR_PROGRESS, strProgress);
        m_bShowProgress = false;

        CString strDriveFreeSpace;
        double dFreeMB = GetDriveFreeMB(m_strFreeSpaceDrive);
        double dFreeGB = dFreeMB / double(1024);
        if (dFreeGB > 1000)
            strDriveFreeSpace.Format(_T("\t%s:\\ %.2f TB free"), (LPCTSTR) m_strFreeSpaceDrive, dFreeGB / double(1024));
        else
            strDriveFreeSpace.Format(_T("\t%s:\\ %.2f GB free"), (LPCTSTR) m_strFreeSpaceDrive, dFreeGB);
        SetPaneText(ID_INDICATOR_PROGRESS_BAR, strDriveFreeSpace);

        if (IsWindow(m_ctrlProgress.GetSafeHwnd()))
            m_ctrlProgress.ShowWindow(SW_HIDE);

        // set the thread to allow sleep
        if (m_bDisableSleep)
        {
            SetThreadExecutionState(ES_CONTINUOUS);
            m_bDisableSleep = false;
        }
    }
    else
    {
        bProcessing = true;
        SetPaneText(ID_INDICATOR_STATUS, _T("\tProcessing..."));

        CString strStatus; 
        strStatus.Format(_T("\t%.2f%% done (%s left)"), double(dProgress * 100), (LPCTSTR) FormatDuration(dSecondsLeft));
        SetPaneText(ID_INDICATOR_PROGRESS, strStatus);

        SetPaneText(ID_INDICATOR_PROGRESS_BAR, _T("")); // empty the text since we're setting a progress
        m_ctrlProgress.SetPos(int(dProgress * 100));
        m_bShowProgress = true;

        if (IsWindow(m_ctrlProgress.GetSafeHwnd()))
            m_ctrlProgress.ShowWindow(SW_SHOW);

        // set the thread so we don't allow the system to sleep
        if (m_bDisableSleep == false)
        {
            SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
            m_bDisableSleep = true;
        }
    }

    if (m_pTaskBarlist != NULL)
    {
        m_pTaskBarlist->SetProgressValue(m_hwndParent, m_ctrlProgress.GetPos(), 100);
    }

    if (bProcessing != m_bProcessing)
        SizeStatusbar();
    m_bProcessing = bProcessing;

    return TRUE;
}

BOOL CMACStatusBar::SetLastProcessTotalMS(int nMilliseconds)
{
    m_nProcessTotalMS = nMilliseconds;
    UpdateProgress(0, 0);
    return TRUE;
}

void CMACStatusBar::StartProcessing()
{
    HRESULT hr = S_OK;
    if (m_pTaskBarlist == NULL)
    {
        hr = CoCreateInstance(
            CLSID_TaskbarList, NULL, CLSCTX_ALL,
            IID_ITaskbarList3, (void **) &m_pTaskBarlist);
    }
    
    if (SUCCEEDED(hr) && (m_pTaskBarlist != NULL))
    {
        m_pTaskBarlist->SetProgressState(m_hwndParent, TBPF_NORMAL);
    }
}

void CMACStatusBar::EndProcessing()
{
    if (m_pTaskBarlist != NULL)
    {
        // show no progress
        m_pTaskBarlist->SetProgressState(m_hwndParent, TBPF_NOPROGRESS);

        // release so the bar goes away
        m_pTaskBarlist->Release();
        m_pTaskBarlist = NULL;
    }
}

void CMACStatusBar::OnLButtonUp(UINT nFlags, CPoint pt)
{
    CRect rectFreeSpace;
    GetItemRect(ID_INDICATOR_PROGRESS_BAR, &rectFreeSpace);
    if (rectFreeSpace.PtInRect(pt))
    {
        ShowFreeSpaceDrivePopup();
    }

    CStatusBar::OnLButtonUp(nFlags, pt);
}

void CMACStatusBar::OnRButtonUp(UINT nFlags, CPoint pt)
{
    CRect rectFreeSpace;
    GetItemRect(ID_INDICATOR_PROGRESS_BAR, &rectFreeSpace);
    if (rectFreeSpace.PtInRect(pt))
    {
        ShowFreeSpaceDrivePopup();
    }

    CStatusBar::OnRButtonUp(nFlags, pt);
}

void CMACStatusBar::ShowFreeSpaceDrivePopup()
{
    CMenu menuPopup; menuPopup.CreatePopupMenu();
        
    TCHAR aryDrive[4] = _T("C:\\");
    for (int nDrive = 'A'; nDrive < 'Z'; nDrive++)
    {
        aryDrive[0] = (TCHAR) nDrive; CString strDrive(aryDrive);
        if (GetDriveType(strDrive) == DRIVE_FIXED)
        {
            BOOL bChecked = (strDrive.Left(1).CompareNoCase(m_strFreeSpaceDrive) == 0);
            menuPopup.AppendMenu(MF_STRING | (bChecked ? MF_CHECKED : MF_UNCHECKED), 
                UINT_PTR(1000 + nDrive), strDrive);
        }
    }

    CPoint ptMouse; GetCursorPos(&ptMouse);
    int nRetVal = menuPopup.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD, 
        ptMouse.x, ptMouse.y, this);

    if (nRetVal >= 1000 && nRetVal < 1100)
    {
        m_strFreeSpaceDrive = CString(TCHAR(nRetVal - 1000));
        UpdateProgress(0, 0);
    }
}

void CMACStatusBar::SizeStatusbar()
{
    // size
    int aryWidth[3] = { 75, 300, 185 };
    for (int nPane = 0; nPane <= 2; nPane++)
    {
        // get text
        CString strPane = GetPaneText(nPane);

        // measure text
        CSize sizeText = m_pMACDlg->MeasureText(strPane);
        sizeText.cx += theApp.GetSize(32, 0).cx;

        // update size to maximimum of text or current size
        SetPaneInfo(nPane, (UINT)-1, SBPS_NORMAL, ape_max(theApp.GetSize(aryWidth[nPane], 0).cx, sizeText.cx));
    }
}
