#pragma once

const UINT UM_SAVE_PAGE_OPTIONS = RegisterWindowMessage(_T("Monkey's Audio: Save Page Options"));

class OPTIONS_PAGE
{
public:
    OPTIONS_PAGE(CString strCaption, int nIcon) :
        m_strCaption(strCaption),
        m_nIcon(nIcon)
    {
        m_pDialog = APE_NULL;
        m_nIdealHeight = -1;
    }

    ~OPTIONS_PAGE()
    {
        if (m_pDialog != APE_NULL)
        {
            if (IsWindow(m_pDialog->GetSafeHwnd()))
                m_pDialog->DestroyWindow();
            delete m_pDialog;
        }
    }

    CDialog * m_pDialog;
    CString m_strCaption;
    int m_nIcon;
    int m_nIdealHeight;
};
