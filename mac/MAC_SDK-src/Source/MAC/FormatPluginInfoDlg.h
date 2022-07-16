#pragma once

#include "Hyperlink.h"
class CMACDlg;

class CFormatPluginInfoDlg : public CDialog
{
public:
    
    CFormatPluginInfoDlg(CMACDlg * pMACDlg, CString strName, CString strVersion, CString strAuthor, CString strDescription, CString strURL, CWnd * pParent = NULL);

    enum { IDD = IDD_FORMAT_PLUGIN_INFO };
    CHyperLink m_ctrlURL;
    CString    m_strDescription1;
    CString    m_strDescription2;

protected:

    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

    CMACDlg * m_pMACDlg;
    CString m_strURL;
};
