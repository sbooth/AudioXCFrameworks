#pragma once

class CMACDlg;
class OPTIONS_PAGE;

class COptionsDlg : public CDialog
{
public:
    COptionsDlg(CMACDlg * pMACDlg, CWnd * pParent = APE_NULL);

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
    afx_msg void OnMoving(UINT, LPRECT pRect);
    DECLARE_MESSAGE_MAP()

    CArray<OPTIONS_PAGE *, OPTIONS_PAGE *> m_aryPages;

    bool UpdatePage();
    int GetSelectedPage() const;

    CMACDlg * m_pMACDlg;
};
