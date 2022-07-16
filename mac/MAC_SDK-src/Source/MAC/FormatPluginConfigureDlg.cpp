#include "stdafx.h"
#include "MAC.h"
#include "FormatPluginConfigureDlg.h"
#include "MACDlg.h"

CFormatPluginConfigureDlg::CFormatPluginConfigureDlg(CMACDlg * pMACDlg, CString strConfigureDescription1,
    CString strConfigureValue1, CString strConfigureDescription2, CString strConfigureValue2,
    CString strConfigureDescription3, CString strConfigureValue3, CWnd * pParent)
    : CDialog(CFormatPluginConfigureDlg::IDD, pParent)
{
    m_pMACDlg = pMACDlg;
    m_strConfigureEdit1 = strConfigureValue1;
    m_strConfigureEdit2 = strConfigureValue2;
    m_strConfigureEdit3 = strConfigureValue3;
    m_strConfigureStatic1 = strConfigureDescription1;
    m_strConfigureStatic2 = strConfigureDescription2;
    m_strConfigureStatic3 = strConfigureDescription3;
}


void CFormatPluginConfigureDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_CONFIGURE_EDIT_1, m_strConfigureEdit1);
    DDX_Text(pDX, IDC_CONFIGURE_EDIT_2, m_strConfigureEdit2);
    DDX_Text(pDX, IDC_CONFIGURE_EDIT_3, m_strConfigureEdit3);
    DDX_Text(pDX, IDC_CONFIGURE_STATIC_1, m_strConfigureStatic1);
    DDX_Text(pDX, IDC_CONFIGURE_STATIC_2, m_strConfigureStatic2);
    DDX_Text(pDX, IDC_CONFIGURE_STATIC_3, m_strConfigureStatic3);
}


BEGIN_MESSAGE_MAP(CFormatPluginConfigureDlg, CDialog)
END_MESSAGE_MAP()


BOOL CFormatPluginConfigureDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    // set the font to all the controls
    SetFont(&m_pMACDlg->GetFont());
    SendMessageToDescendants(WM_SETFONT, (WPARAM)m_pMACDlg->GetFont().GetSafeHandle(), MAKELPARAM(FALSE, 0), TRUE);
   
    if (m_strConfigureStatic1.IsEmpty())
    {
        GetDlgItem(IDC_CONFIGURE_EDIT_1)->EnableWindow(FALSE);
        m_strConfigureEdit1.Empty();
    }
    if (m_strConfigureStatic2.IsEmpty())
    {
        GetDlgItem(IDC_CONFIGURE_EDIT_2)->EnableWindow(FALSE);
        m_strConfigureEdit2.Empty();
    }
    if (m_strConfigureStatic3.IsEmpty())
    {
        GetDlgItem(IDC_CONFIGURE_EDIT_3)->EnableWindow(FALSE);
        m_strConfigureEdit3.Empty();
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
