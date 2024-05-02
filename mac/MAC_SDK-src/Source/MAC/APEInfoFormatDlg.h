#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "APETag.h"

class CMACDlg;

class CAPEInfoFormatDlg : public CDialog
{
    DECLARE_DYNAMIC(CAPEInfoFormatDlg)

public:
    CAPEInfoFormatDlg(CMACDlg * pMACDlg, CWnd * pParent = APE_NULL);
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
    void OutputAPETag(APE::IAPETag * pTag, CString & strSummary);
    CStringArray m_aryFiles;

public:
    CRichEditCtrl m_ctrlFormat;
};
