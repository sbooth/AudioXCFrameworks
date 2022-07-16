#pragma once

class CMACDlg;
class OPTIONS_PAGE;

class COptionsProcessingDlg : public CDialog
{
public:
    COptionsProcessingDlg(CMACDlg * pMACDlg, OPTIONS_PAGE * pPage, CWnd * pParent = NULL);

    enum { IDD = IDD_OPTIONS_PROCESSING };
    CStatic m_ctrlOtherPicture;
    CStatic m_ctrlErrorBehaviorPicture;
    CStatic m_ctrlVerifyPicture;
    CStatic m_ctrlGeneralPicture;
    BOOL m_bCompletionSound;
    int m_nPriorityMode;
    int m_nSimultaneousFiles;
    BOOL m_bStopOnError;
    BOOL m_bShowExternalWindows;
    BOOL m_bAutoVerify;
    int m_nVerifyMode;

protected:

    virtual void DoDataExchange(CDataExchange * pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnOK();
    afx_msg void OnCancel();
    afx_msg void OnDestroy();
    afx_msg LRESULT OnSaveOptions(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

    void Layout();

    CMACDlg * m_pMACDlg;
    OPTIONS_PAGE * m_pPage;
    CArray<HICON, HICON> m_aryIcons;
};
