#pragma once

#include "APEButtons.h"
#include "OptionsProcessingDlg.h"
#include "OptionsOutputDlg.h"

const UINT UM_SAVE_PAGE_OPTIONS = RegisterWindowMessage(_T("Monkey's Audio: Save Page Options"));

/**************************************************************************************************
OPTIONS_PAGE - base class for options
**************************************************************************************************/
class OPTIONS_PAGE
{
public:
    OPTIONS_PAGE(CString strCaption, int nIcon, CDialog * pDialog) :
        m_strCaption(strCaption),
        m_nIcon(nIcon),
        m_spDialog(pDialog)
    {
        m_nIdealHeight = -1;
    }

    ~OPTIONS_PAGE()
    {
        if (m_spDialog != APE_NULL)
        {
            if (IsWindow(m_spDialog->GetSafeHwnd()))
                m_spDialog->DestroyWindow();
            m_spDialog.Delete();
        }
    }

    CString m_strCaption;
    int m_nIcon;
    int m_nIdealHeight;
    APE::CSmartPtr<CDialog> m_spDialog;
};

/**************************************************************************************************
OPTIONS_PAGE_PROCESSING - processing page
**************************************************************************************************/
class OPTIONS_PAGE_PROCESSING : public OPTIONS_PAGE
{
public:
    OPTIONS_PAGE_PROCESSING(CMACDlg * pMACDlg, CWnd * pwndParent) :
        OPTIONS_PAGE("Processing", TBB_OPTIONS_PAGE_PROCESSING, new COptionsProcessingDlg(pMACDlg, this))
    {
        m_spDialog->Create(IDD_OPTIONS_PROCESSING, pwndParent);
    }
};

/**************************************************************************************************
OPTIONS_PAGE_PROCESSING - output page
**************************************************************************************************/
class OPTIONS_PAGE_OUTPUT : public OPTIONS_PAGE
{
public:
    OPTIONS_PAGE_OUTPUT(CMACDlg * pMACDlg, CWnd* pwndParent) :
        OPTIONS_PAGE("Output", TBB_OPTIONS_PAGE_OUTPUT, new COptionsOutputDlg(pMACDlg, this))
    {
        m_spDialog->Create(IDD_OPTIONS_OUTPUT, pwndParent);
    }
};
