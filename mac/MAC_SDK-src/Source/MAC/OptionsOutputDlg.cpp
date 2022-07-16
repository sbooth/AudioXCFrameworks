#include "stdafx.h"
#include "MAC.h"
#include "OptionsOutputDlg.h"
#include "OptionsShared.h"
#include "FolderDialog.h"
#include "APEButtons.h"
#include "MACDlg.h"

COptionsOutputDlg::COptionsOutputDlg(CMACDlg * pMACDlg, OPTIONS_PAGE * pPage, CWnd * pParent)
    : CDialog(COptionsOutputDlg::IDD, pParent)
{
    m_pPage = pPage;
    m_pMACDlg = pMACDlg;
    m_bMirrorTimeStamp = FALSE;
}

void COptionsOutputDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_OTHER_PICTURE, m_ctrlOtherPicture);
    DDX_Control(pDX, IDC_DELETE_AFTER_SUCCESS_COMBO, m_ctrlDeleteAfterSuccessCombo);
    DDX_Control(pDX, IDC_BEHAVIOR_PICTURE, m_ctrlBehaviorPicture);
    DDX_Control(pDX, IDC_OUTPUT_EXISTS_COMBO, m_ctrlOutputExistsCombo);
    DDX_Control(pDX, IDC_OUTPUT_LOCATION_DIRECTORY_COMBO, m_ctrlOutputLocationDirectoryCombo);
    DDX_Control(pDX, IDC_OUTPUT_LOCATION_DIRECTORY_BROWSE, m_ctrlOutputLocationDirectoryBrowse);
    DDX_Control(pDX, IDC_OUTPUT_LOCATION_PICTURE, m_ctrlOutputLocationPicture);
    DDX_Control(pDX, IDC_OUTPUT_LOCATION_DIRECTORY_RECREATE, m_ctrlOutputLocationDirectoryRecreate);
    DDX_CBString(pDX, IDC_APL_FILENAME_TEMPLATE_COMBO, m_strAPLFilenameTemplate);
    DDX_Check(pDX, IDC_MIRROR_TIME_STAMP_CHECK, m_bMirrorTimeStamp);
}


BEGIN_MESSAGE_MAP(COptionsOutputDlg, CDialog)
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_APL_FILENAME_TEMPLATE_HELP, OnAplFilenameTemplateHelp)
    ON_BN_CLICKED(IDC_OUTPUT_LOCATION_DIRECTORY_BROWSE, OnOutputLocationDirectoryBrowse)
    ON_BN_CLICKED(IDC_OUTPUT_LOCATION_SPECIFIED_DIRECTORY, OnOutputLocationSpecifiedDirectory)
    ON_BN_CLICKED(IDC_OUTPUT_LOCATION_SAME_DIRECTORY, OnOutputLocationSameDirectory)
    ON_BN_CLICKED(IDC_OUTPUT_LOCATION_RECREATE_DIRECTORY_STRUCTURE_CHECK, OnOutputLocationRecreateDirectoryStructureCheck)
    ON_BN_CLICKED(IDOK, OnOK)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_REGISTERED_MESSAGE(UM_SAVE_PAGE_OPTIONS, OnSaveOptions)
END_MESSAGE_MAP()

BOOL COptionsOutputDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    // set the font to all the controls
    SetFont(&m_pMACDlg->GetFont());
    SendMessageToDescendants(WM_SETFONT, (WPARAM) m_pMACDlg->GetFont().GetSafeHandle(), MAKELPARAM(FALSE, 0), TRUE);

    // images
    HICON hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_OUTPUT_LOCATION);
    m_ctrlOutputLocationPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_OUTPUT_BEHAVIOR);
    m_ctrlBehaviorPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    hIcon = theApp.GetImageList(CMACApp::Image_OptionsPages)->ExtractIcon(TBB_OPTIONS_OUTPUT_OTHER);
    m_ctrlOtherPicture.SetIcon(hIcon);
    m_aryIcons.Add(hIcon);

    // output location settings
    ((CButton *) GetDlgItem(IDC_OUTPUT_LOCATION_SAME_DIRECTORY + theApp.GetSettings()->m_nOutputLocationMode))->SetCheck(TRUE);
    m_ctrlOutputLocationDirectoryCombo.SetWindowText(theApp.GetSettings()->m_strOutputLocationDirectory);
    if (theApp.GetSettings()->m_bOutputLocationRecreateDirectoryStructure == false)
        m_ctrlOutputLocationDirectoryRecreate.SetCurSel(0);
    else
        m_ctrlOutputLocationDirectoryRecreate.SetCurSel(theApp.GetSettings()->m_nOutputLocationRecreateDirectoryStructureLevels);

    // behavior
    m_ctrlOutputExistsCombo.SetCurSel(theApp.GetSettings()->m_nOutputExistsMode);
    m_ctrlDeleteAfterSuccessCombo.SetCurSel(theApp.GetSettings()->m_nOutputDeleteAfterSuccessMode);

    // other
    m_bMirrorTimeStamp = theApp.GetSettings()->m_bOutputMirrorTimeStamp;
    m_strAPLFilenameTemplate = theApp.GetSettings()->m_strAPLFilenameTemplate;

    // layout
    Layout();

    // update our size
    //CRect rectWindow; GetWindowRect(&rectWindow);
    //SetWindowPos(NULL, 0, 0, rectWindow.Width(), m_pPage->m_nIdealHeight + theApp.GetSize(128, 0).cx, SWP_NOMOVE);

    // update
    UpdateData(FALSE);
    UpdateDialogState();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void COptionsOutputDlg::UpdateDialogState()
{
    int nOutputLocation = GetCheckedRadioButton(IDC_OUTPUT_LOCATION_SAME_DIRECTORY, IDC_OUTPUT_LOCATION_SPECIFIED_DIRECTORY) - IDC_OUTPUT_LOCATION_SAME_DIRECTORY;
    
    m_ctrlOutputLocationDirectoryRecreate.EnableWindow((nOutputLocation == 1));
    m_ctrlOutputLocationDirectoryCombo.EnableWindow((nOutputLocation == 1));
    m_ctrlOutputLocationDirectoryBrowse.EnableWindow((nOutputLocation == 1));
}

void COptionsOutputDlg::OnSize(UINT nType, int cx, int cy)
{
    Layout();
    CDialog::OnSize(nType, cx, cy);
}

void COptionsOutputDlg::OnDestroy() 
{
    CDialog::OnDestroy();
    
    for (int z = 0; z < m_aryIcons.GetSize(); z++)
        DestroyIcon(m_aryIcons[z]);
    m_aryIcons.RemoveAll();
}

LRESULT COptionsOutputDlg::OnSaveOptions(WPARAM, LPARAM)
{
    UpdateData(TRUE);

    // output location
    theApp.GetSettings()->m_nOutputLocationMode = GetCheckedRadioButton(IDC_OUTPUT_LOCATION_SAME_DIRECTORY, IDC_OUTPUT_LOCATION_SPECIFIED_DIRECTORY) - IDC_OUTPUT_LOCATION_SAME_DIRECTORY;
    m_ctrlOutputLocationDirectoryCombo.GetWindowText(theApp.GetSettings()->m_strOutputLocationDirectory);
    if (m_ctrlOutputLocationDirectoryRecreate.GetCurSel() == 0)
    {
        theApp.GetSettings()->m_bOutputLocationRecreateDirectoryStructure = false;
    }
    else
    {
        theApp.GetSettings()->m_bOutputLocationRecreateDirectoryStructure = true;
        theApp.GetSettings()->m_nOutputLocationRecreateDirectoryStructureLevels = m_ctrlOutputLocationDirectoryRecreate.GetCurSel();
    }

    // behavior
    theApp.GetSettings()->m_nOutputExistsMode = m_ctrlOutputExistsCombo.GetCurSel();
    theApp.GetSettings()->m_nOutputDeleteAfterSuccessMode = m_ctrlDeleteAfterSuccessCombo.GetCurSel();

    // other
    theApp.GetSettings()->m_bOutputMirrorTimeStamp = m_bMirrorTimeStamp;
    theApp.GetSettings()->m_strAPLFilenameTemplate = m_strAPLFilenameTemplate;

    return TRUE;
}

void COptionsOutputDlg::OnAplFilenameTemplateHelp() 
{
    CString strMessage;
    
    strMessage = _T("You can create filenames using any combination of letters and keywords.\r\n\r\n")
        _T("These keywords will be replaced by the corresponding values for each track:\r\n")
        _T("     ARTIST\r\n")
        _T("     ALBUM\r\n")
        _T("     TITLE\r\n")
        _T("     TRACK#\r\n")
        _T("\r\nExample:\r\n") 
        _T("     Naming template: \"ARTIST - ALBUM - TRACK# - TITLE\"\r\n")
        _T("     Resulting filename: \"Bush - Sixteen Stone - 09 - Monkey.apl\"");
    
    ::MessageBox(GetSafeHwnd(), strMessage, _T("Filename Help"), MB_OK | MB_ICONINFORMATION);
}

void COptionsOutputDlg::OnOutputLocationDirectoryBrowse() 
{
    CString strPath;
    m_ctrlOutputLocationDirectoryCombo.GetWindowText(strPath);

    CFolderDialog FolderDialog(strPath);
    if (FolderDialog.DoModal() == IDOK)
    {
        m_ctrlOutputLocationDirectoryCombo.SetWindowText(FolderDialog.GetPathName());
    }    
}

void COptionsOutputDlg::OnOutputLocationSpecifiedDirectory() 
{
    UpdateDialogState();
}

void COptionsOutputDlg::OnOutputLocationSameDirectory() 
{
    UpdateDialogState();
}

void COptionsOutputDlg::OnOutputLocationRecreateDirectoryStructureCheck() 
{
    UpdateDialogState();    
}

void COptionsOutputDlg::OnOK()
{

}

void COptionsOutputDlg::OnCancel()
{
    
}

void COptionsOutputDlg::Layout()
{
    CRect rectLayout; GetClientRect(&rectLayout);
    int nBorder = theApp.GetSize(8, 0).cx;
    rectLayout.DeflateRect(nBorder, nBorder, nBorder, nBorder);
    int nLeft = rectLayout.left;

    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE1), GetDlgItem(IDC_DIVIDER1), GetDlgItem(IDC_OUTPUT_LOCATION_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_OUTPUT_LOCATION_SAME_DIRECTORY), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_OUTPUT_LOCATION_SPECIFIED_DIRECTORY), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_OUTPUT_LOCATION_DIRECTORY_COMBO), rectLayout, false, true, GetDlgItem(IDC_OUTPUT_LOCATION_DIRECTORY_BROWSE));
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_OUTPUT_LOCATION_DIRECTORY_RECREATE), rectLayout, false, true);

    rectLayout.left = nLeft;
    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE2), GetDlgItem(IDC_DIVIDER2), GetDlgItem(IDC_BEHAVIOR_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STATICB1), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_OUTPUT_EXISTS_COMBO), rectLayout, false, true);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STATICB2), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_DELETE_AFTER_SUCCESS_COMBO), rectLayout, false, true);

    rectLayout.left = nLeft;
    m_pMACDlg->LayoutControlTopWithDivider(GetDlgItem(IDC_TITLE3), GetDlgItem(IDC_DIVIDER3), GetDlgItem(IDC_OTHER_PICTURE), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_MIRROR_TIME_STAMP_CHECK), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_STATICO1), rectLayout);
    m_pMACDlg->LayoutControlTop(GetDlgItem(IDC_APL_FILENAME_TEMPLATE_COMBO), rectLayout, false, true, GetDlgItem(IDC_APL_FILENAME_TEMPLATE_HELP));

    // update ideal height
    m_pPage->m_nIdealHeight = rectLayout.top + nBorder;
}