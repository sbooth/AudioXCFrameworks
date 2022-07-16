#pragma once
#include "afxcmn.h"
#include "APEInfoFormatDlg.h"
class CMACDlg;

class CAPEInfoDlg : public CDialog
{
public:

    CAPEInfoDlg(CMACDlg * pMACDlg, CStringArray & aryFiles);

    enum { IDD = IDD_APE_INFO };

protected:

    virtual void DoDataExchange(CDataExchange * pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

    CMACDlg * m_pMACDlg;
    CStringArray m_aryFiles;
    CAPEInfoFormatDlg m_dlgFormat;

public:

    CListCtrl m_ctrlFiles;
    afx_msg void OnBnClickedFilesSelectAll();
    afx_msg void OnBnClickedFilesSelectNone();
    CTabCtrl m_ctrlTabs;
    afx_msg void OnLvnItemchangedFileList(NMHDR * pNMHDR, LRESULT * pResult);
};
