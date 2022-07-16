#pragma once
#include "afxwin.h"
#include "afxcmn.h"
class CMACDlg;

class CAPEInfoFormatDlg : public CDialog
{
    DECLARE_DYNAMIC(CAPEInfoFormatDlg)

public:

    CAPEInfoFormatDlg(CMACDlg * pMACDlg, CWnd * pParent = NULL);
    virtual ~CAPEInfoFormatDlg();

    void Layout();
    BOOL SetFiles(CStringArray & aryFiles);

    enum { IDD = IDD_APE_INFO_FORMAT };

protected:

    virtual void DoDataExchange(CDataExchange * pDX);
    virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()

    CMACDlg * m_pMACDlg;
    CString GetSummary(const CString & strFilename);
    CStringArray m_aryFiles;

public:

    CRichEditCtrl m_ctrlFormat;
};
