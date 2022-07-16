#pragma once

class MAC_FILE_ARRAY;
class CMACDlg;

class CMACStatusBar : public CStatusBar
{
public:
    
    CMACStatusBar(CMACDlg * pMACDlg);
    virtual ~CMACStatusBar();

    BOOL UpdateProgress(double dProgress, double dSecondsLeft);
    BOOL UpdateFiles(MAC_FILE_ARRAY * paryFiles);
    BOOL SetLastProcessTotalMS(int nMilliseconds);
    void StartProcessing();
    void EndProcessing();

protected:

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint pt);
    DECLARE_MESSAGE_MAP()

    void ShowFreeSpaceDrivePopup();
    void SizeStatusbar();

    CMACDlg * m_pMACDlg;
    HWND m_hwndParent;
    CProgressCtrl m_ctrlProgress;
    bool m_bShowProgress;
    int m_nProcessTotalMS;
    CString m_strFreeSpaceDrive;
    ITaskbarList3 * m_pTaskBarlist;
    bool m_bDisableSleep;
    bool m_bProcessing;
};