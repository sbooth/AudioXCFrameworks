#pragma once

#include "resource.h"
#include "afxcmn.h"

class CWinampSettingsDlg : public CDialog
{
    DECLARE_DYNAMIC(CWinampSettingsDlg)

public:
    CWinampSettingsDlg(CWnd * pParent = APE_NULL);
    virtual ~CWinampSettingsDlg();

    bool Show(HWND hwndParent);

    virtual BOOL OnInitDialog();
    enum { IDD = IDD_WINAMP_SETTINGS };

    BOOL m_bIgnoreBitstreamErrors;
    BOOL m_bSuppressSilence;
    BOOL m_bScaleOutput;
    CString m_strFileDisplayMethod;
    int m_nThreadPriority;

protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    virtual void OnOK();
    DECLARE_MESSAGE_MAP()

    CSliderCtrl m_ctrlThreadPrioritySlider;
    HWND m_hwndParent;

    CString m_strSettingsFilename;
    bool LoadSettings();
    bool SaveSettings();
    int GetSliderFromThreadPriority();
    int GetThreadPriorityFromSlider();
};
