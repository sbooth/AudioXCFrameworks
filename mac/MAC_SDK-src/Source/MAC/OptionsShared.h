#pragma once

const UINT UM_SAVE_PAGE_OPTIONS = RegisterWindowMessage(_T("Monkey's Audio: Save Page Options"));

class OPTIONS_PAGE
{
public:
    
    OPTIONS_PAGE(CDialog * pDialog, CString strCaption, int nIcon)
    {
        m_pDialog = pDialog;
        m_strCaption = strCaption;
        m_nIcon = nIcon;
        m_nIdealHeight = -1;
    }

    ~OPTIONS_PAGE()
    {
        if ((m_pDialog != NULL) && (IsWindow(m_pDialog->GetSafeHwnd())))
            m_pDialog->DestroyWindow();

        SAFE_DELETE(m_pDialog)
    }
    

    CDialog * m_pDialog;
    CString m_strCaption;
    int m_nIcon;
    int m_nIdealHeight;
};