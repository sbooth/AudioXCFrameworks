#pragma once

#include "MACListCtrl.h"
#include "MACStatusBar.h"
#include "MACProcessFiles.h"
#include "MACFileArray.h"

class CMACDlg : public CDialog
{
public:
    
    CMACDlg(CWnd * pParent = NULL);

    enum { IDD = IDD_MAC_DIALOG };
    CMACListCtrl m_ctrlList;
    CMACStatusBar m_ctrlStatusBar;
    CToolBar m_ctrlToolbar;

    BOOL GetProcessing() { return (m_spProcessFiles != NULL); }
    BOOL GetInitialized() { return m_bInitialized; }
    CFont & GetFont() { return m_Font; }
    CSize MeasureText(const CString & strText);

    virtual BOOL PreTranslateMessage(MSG * pMsg);
    virtual void WinHelp(DWORD_PTR dwData, UINT nCmd = HELP_CONTEXT);

    void LayoutControlTop(CWnd * pwndLayout, CRect & rectLayout, bool bOnlyControlWidth = false, bool bCombobox = false, CWnd * pwndRight = NULL);
    void LayoutControlTopWithDivider(CWnd * pwndLayout, CWnd * pwndDivider, CWnd * pwndImage, CRect & rectLayout);

protected:

    virtual void DoDataExchange(CDataExchange * pDX);

    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnFileProcessFiles();
    afx_msg void OnFileAddFiles();
    afx_msg void OnFileAddFolder();
    afx_msg void OnFileSelectAll();
    afx_msg void OnFileClearFiles();
    afx_msg void OnFileRemoveFiles();
    afx_msg void OnFileFileInfo();
    afx_msg void OnFileExit();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnToolbarDropDown(NMHDR * pnmh, LRESULT * plRes);
    afx_msg void OnModeCompress();
    afx_msg void OnModeDecompress();
    afx_msg void OnModeVerify();
    afx_msg void OnModeConvert();
    afx_msg void OnModeMakeAPL();
    afx_msg void OnStop();
    afx_msg void OnCompression();
    afx_msg void OnCompressionAPEExtraHigh();
    afx_msg void OnCompressionAPEFast();
    afx_msg void OnCompressionAPEHigh();
    afx_msg void OnCompressionAPENormal();
    afx_msg void OnCompressionAPEInsane();
    afx_msg void OnSetCompressionMenu(UINT nID);
    afx_msg void OnDestroy();
    afx_msg void OnPause();
    afx_msg void OnStopAfterCurrentFile();
    afx_msg void OnStopImmediately();
    afx_msg void OnHelpHelp();
    afx_msg void OnHelpAbout();
    afx_msg void OnHelpWebsiteMonkeysAudio();
    afx_msg void OnHelpWebsiteJRiver();
    afx_msg void OnHelpWebsiteWinamp();
    afx_msg void OnHelpWebsiteEac();
    afx_msg void OnToolsOptions();
    afx_msg void OnInitMenu(CMenu * pMenu);
    afx_msg void OnInitMenuPopup(CMenu * pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR * lpMMI);
    afx_msg BOOL OnQueryEndSession();
    afx_msg void OnEndSession(BOOL bEnding);
    afx_msg LRESULT OnDPIChange(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

    BOOL m_bInitialized;
    CSmartPtr<CMACProcessFiles> m_spProcessFiles;
    MAC_FILE_ARRAY m_aryFiles;
    HACCEL m_hAcceleratorTable;
    CString m_strAddFilesBasePath;
    CMenu m_menuMain;
    HICON m_hIcon;
    CFont m_Font;
    CFont m_fontStart;
    BOOL m_bLastLoadMenuAndToolbarProcessing;

    void LayoutWindow();
    void UpdateWindow();
    BOOL AddToolbarButton(int nID, int nBitmap, const CString & strText = "", int nStyle = TBSTYLE_BUTTON);
    BOOL SetMode(MAC_MODES Mode);
    BOOL SetAPECompressionLevel(int nAPECompressionLevel);
    BOOL LoadMenuAndToolbar(BOOL bProcessing);
    void SetToolbarButtonBitmap(int nID, int nBitmap);
    void PlayDefaultSound();
    void LoadScale();
};

