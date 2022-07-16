#include "stdafx.h"
#include "MAC.h"
#include "FormatPluginInfoDlg.h"
#include "MACDlg.h"

CFormatPluginInfoDlg::CFormatPluginInfoDlg(CMACDlg * pMACDlg, CString strName, CString strVersion, CString strAuthor, CString strDescription, CString strURL, CWnd * pParent)
    : CDialog(CFormatPluginInfoDlg::IDD, pParent),
    m_ctrlURL(&pMACDlg->GetFont())
{
    m_pMACDlg = pMACDlg;
    m_strURL = strURL;
    m_strDescription1 = strName;
    if (strVersion.GetLength() > 0)
        m_strDescription1 += _T(" (") + strVersion + _T(")");
    if (strAuthor.GetLength() > 0)
        m_strDescription1 += _T(" by ") + strAuthor;
    m_strDescription2 = strDescription;
}

void CFormatPluginInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URL, m_ctrlURL);
    DDX_Text(pDX, IDC_DESCRIPTION_1, m_strDescription1);
    DDX_Text(pDX, IDC_DESCRIPTION_2, m_strDescription2);
}

BEGIN_MESSAGE_MAP(CFormatPluginInfoDlg, CDialog)
END_MESSAGE_MAP()

BOOL CFormatPluginInfoDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    // set the font to all the controls
    SetFont(&m_pMACDlg->GetFont());
    m_ctrlURL.SetFont(&m_pMACDlg->GetFont());
    SendMessageToDescendants(WM_SETFONT, (WPARAM) m_pMACDlg->GetFont().GetSafeHandle(), MAKELPARAM(FALSE, 0), TRUE);
  
    if (m_strURL.IsEmpty())
    {
        m_ctrlURL.SetWindowText(_T("no webpage available"));
        m_ctrlURL.SetURL(_T(""));
        m_ctrlURL.EnableWindow(FALSE);
    }
    else
    {
        m_ctrlURL.SetWindowText(m_strURL);
        m_ctrlURL.SetURL(m_strURL);
        m_ctrlURL.SetLinkCursor(LoadCursor(NULL, IDC_HAND));
    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
