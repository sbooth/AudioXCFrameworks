#pragma once

class CMACDlg;

class CFormatPluginConfigureDlg : public CDialog
{
public:
    CFormatPluginConfigureDlg(CMACDlg * pMACDlg, CString strConfigureDescription1,
        CString strConfigureValue1, CString strConfigureDescription2, CString strConfigureValue2,
        CString strConfigureDescription3, CString strConfigureValue3, CWnd * pParent = NULL);

    enum { IDD = IDD_FORMAT_PLUGIN_CONFIGURE };
    CString m_strConfigureEdit1;
    CString m_strConfigureEdit2;
    CString m_strConfigureEdit3;
    CString m_strConfigureStatic1;
    CString m_strConfigureStatic2;
    CString m_strConfigureStatic3;

protected:

    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

    CMACDlg * m_pMACDlg;
};
