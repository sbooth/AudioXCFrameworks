#pragma once

#include "OptionsShared.h"
class CMACDlg;

class COptionsDlg : public CDialog
{
public:

    COptionsDlg(CMACDlg * pMACDlg, CWnd * pParent = NULL);

    enum { IDD = IDD_OPTIONS };
    CListCtrl m_ctrlPageList;
    CButton m_ctrlPageFrame;

protected:

    virtual void DoDataExchange(CDataExchange * pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    afx_msg void OnItemchangedPageList(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

    CArray<OPTIONS_PAGE *, OPTIONS_PAGE *> m_aryPages;

    BOOL UpdatePage();
    int GetSelectedPage();

    CMACDlg * m_pMACDlg;
};
