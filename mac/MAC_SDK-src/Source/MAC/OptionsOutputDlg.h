#pragma once

class CMACDlg;
class OPTIONS_PAGE;

class COptionsOutputDlg : public CDialog
{
public:

    COptionsOutputDlg(CMACDlg * pMACDlg, OPTIONS_PAGE * pPage, CWnd * pParent = NULL);

    enum { IDD = IDD_OPTIONS_OUTPUT };
    CStatic    m_ctrlOtherPicture;
    CComboBox    m_ctrlDeleteAfterSuccessCombo;
    CStatic    m_ctrlBehaviorPicture;
    CComboBox    m_ctrlOutputExistsCombo;
    CComboBox    m_ctrlOutputLocationDirectoryCombo;
    CButton    m_ctrlOutputLocationDirectoryBrowse;
    CComboBox  m_ctrlOutputLocationDirectoryRecreate;
    CString    m_strAPLFilenameTemplate;
    CStatic    m_ctrlOutputLocationPicture;
    BOOL    m_bMirrorTimeStamp;

protected:

    virtual void DoDataExchange(CDataExchange * pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg void OnAplFilenameTemplateHelp();
    afx_msg void OnOutputLocationDirectoryBrowse();
    afx_msg void OnOutputLocationSpecifiedDirectory();
    afx_msg void OnOutputLocationSameDirectory();
    afx_msg void OnOutputLocationRecreateDirectoryStructureCheck();
    afx_msg void OnOK();
    afx_msg void OnCancel();
    afx_msg LRESULT OnSaveOptions(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

    void Layout();

    CArray<HICON, HICON> m_aryIcons;
    CMACDlg * m_pMACDlg;
    OPTIONS_PAGE * m_pPage;
    void UpdateDialogState();
};
