#include "stdafx.h"
#include "MAC.h"
#include "OptionsProcessingDlg.h"
#include "OptionsShared.h"
#include "APEButtons.h"
#include "MACDlg.h"

COptionsProcessingDlg::COptionsProcessingDlg(CMACDlg * pMACDlg, OPTIONS_PAGE * pPage, CWnd * pParent)
    : CDialog(COptionsProcessingDlg::IDD, pParent)
{
    m_pMACDlg = pMACDlg;
    m_pPage = pPage;
    m_bCompletionSound = FALSE;
    m_nPriorityMode = -1;
    m_nSimultaneousFiles = -1;
    m_bStopOnError = FALSE;
    m_bShowExternalWindows = FALSE;
    m_bAutoVerify = FALSE;
    m_nVerifyMode = -1;
}

void COptionsProcessingDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OTHER_PICTURE, m_ctrlOtherPicture);
    DDX_Control(pDX, IDC_ERROR_BEHAVIOR_PICTURE, m_ctrlErrorBehaviorPicture);
    DDX_Control(pDX, IDC_VERIFY_PICTURE, m_ctrlVerifyPicture);
    DDX_Control(pDX, IDC_GENERAL_PICTURE, m_ctrlGeneralPicture);
    DDX_Check(pDX, IDC_COMPLETION_SOUND_CHECK, m_bCompletionSound);
    DDX_CBIndex(pDX, IDC_PRIORITY_COMBO, m_nPriorityMode);
    DDX_CBIndex(pDX, IDC_SIMULTANEOUS_FILES_COMBO, m_nSimultaneousFiles);
    DDX_Check(pDX, IDC_STOP_ON_ERROR_CHECK, m_bStopOnError);
    DDX_Check(pDX, IDC_SHOW_EXTERNAL_WINDOWS, m_bShowExternalWindows);
    DDX_Check(pDX, IDC_AUTO_VERIFY_CHECK, m_bAutoVerify);
    DDX_CBIndex(pDX, IDC_VERIFY_MODE_COMBO, m_nVerifyMode);
}

BEGIN_MESSAGE_MAP(COptionsProcessingDlg, CDialog)
    ON_WM_SIZE()
    ON_BN_CLICKED(IDOK, OnOK)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_WM_DESTROY()
    ON_REGISTERED_MESSAGE(UM_SAVE_PAGE_OPTIONS, OnSaveOptions)
END_MESSAGE_MAP()

BOOL COptionsProcessingDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    // set the font to all the controls
    SetFont(&m_pMACDlg->GetFont());
    SendMessageToDescendants(WM_SETFONT, (WPARAM) m_pMACDlg->GetFont().GetSafeHandle(), MAKELPARAM(FALSE, 0), TRUE);

    // images
    HICON hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_PROCESSING_GENERAL);
    m_ctrlGeneralPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_PROCESSING_VERIFY);
    m_ctrlVerifyPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_PROCESSING_ERROR_BEHAVIOR);
    m_ctrlErrorBehaviorPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_PROCESSING_OTHER);
    m_ctrlOtherPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    // settings
    m_nSimultaneousFiles = theApp.GetSettings()->m_nProcessingSimultaneousFiles - 1;
    m_nPriorityMode = theApp.GetSettings()->m_nProcessingPriorityMode;
    m_bStopOnError = theApp.GetSettings()->m_bProcessingStopOnErrors;
    m_bCompletionSound = theApp.GetSettings()->m_bProcessingPlayCompletionSound;
    m_bShowExternalWindows = theApp.GetSettings()->m_bProcessingShowExternalWindows;
    m_bAutoVerify = theApp.GetSettings()->m_bProcessingAutoVerifyOnCreation;
    m_nVerifyMode = theApp.GetSettings()->m_nProcessingVerifyMode;

    // layout (to get the ideal height)
    Layout();

    // settings
    m_nSimultaneousFiles = theApp.GetSettings()->m_nProcessingSimultaneousFiles - 1;
    m_nPriorityMode = theApp.GetSettings()->m_nProcessingPriorityMode;
    m_bStopOnError = theApp.GetSettings()->m_bProcessingStopOnErrors;
    m_bCompletionSound = theApp.GetSettings()->m_bProcessingPlayCompletionSound;
    m_bShowExternalWindows = theApp.GetSettings()->m_bProcessingShowExternalWindows;
    m_bAutoVerify = theApp.GetSettings()->m_bProcessingAutoVerifyOnCreation;
    m_nVerifyMode = theApp.GetSettings()->m_nProcessingVerifyMode;

    // update our size
    //CRect rectWindow; GetWindowRect(&rectWindow);
    //SetWindowPos(NULL, 0, 0, rectWindow.Width(), m_pPage->m_nIdealHeight + theApp.GetSize(128, 0).cx, SWP_NOMOVE);

    // update
    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void COptionsProcessingDlg::OnDestroy() 
{
    CDialog::OnDestroy();
    
    for (int z = 0; z < m_aryIcons.GetSize(); z++)
        DestroyIcon(m_aryIcons[z]);
    m_aryIcons.RemoveAll();
}

LRESULT COptionsProcessingDlg::OnSaveOptions(WPARAM, LPARAM)
{
    UpdateData(TRUE);

    // settings
    theApp.GetSettings()->m_nProcessingSimultaneousFiles = m_nSimultaneousFiles + 1;
    theApp.GetSettings()->m_nProcessingPriorityMode = m_nPriorityMode;
    theApp.GetSettings()->m_bProcessingStopOnErrors = m_bStopOnError;
    theApp.GetSettings()->m_bProcessingPlayCompletionSound = m_bCompletionSound;
    theApp.GetSettings()->m_bProcessingShowExternalWindows = m_bShowExternalWindows;
    theApp.GetSettings()->m_bProcessingAutoVerifyOnCreation = m_bAutoVerify;
    theApp.GetSettings()->m_nProcessingVerifyMode = m_nVerifyMode;

    return TRUE;
}

void COptionsProcessingDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    Layout();
}

void COptionsProcessingDlg::OnOK()
{

}

void COptionsProcessingDlg::OnCancel()
{
    
}

void COptionsProcessingDlg::Layout()
{
    // layout
    CRect rectLayout; GetClientRect(&rectLayout);
    int nBorder = theApp.GetSize(8, 0).cx;
    rectLayout.DeflateRect(nBorder, nBorder, nBorder, nBorder);
    int nLeft = rectLayout.left;

    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE1), GetDlgItem(IDC_DIVIDER1), GetDlgItem(IDC_GENERAL_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STATIC1), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_SIMULTANEOUS_FILES_COMBO), rectLayout, false, true);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STATIC2), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_PRIORITY_COMBO), rectLayout, false, true);

    rectLayout.left = nLeft;
    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE2), GetDlgItem(IDC_DIVIDER2), GetDlgItem(IDC_VERIFY_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STATIC3), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_VERIFY_MODE_COMBO), rectLayout, false, true);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_AUTO_VERIFY_CHECK), rectLayout);

    //m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_OUTPUT_EXISTS_COMBO), rectLayout);
    //m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_AUTO_VERIFY_CHECK), rectLayout);

    rectLayout.left = nLeft;
    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE3), GetDlgItem(IDC_DIVIDER3), GetDlgItem(IDC_ERROR_BEHAVIOR_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STOP_ON_ERROR_CHECK), rectLayout);
    rectLayout.top += nBorder; // extra space

    rectLayout.left = nLeft;
    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE4), GetDlgItem(IDC_DIVIDER4), GetDlgItem(IDC_OTHER_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_COMPLETION_SOUND_CHECK), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_SHOW_EXTERNAL_WINDOWS), rectLayout);

    // update ideal height
    m_pPage->m_nIdealHeight = rectLayout.top + nBorder;
}
