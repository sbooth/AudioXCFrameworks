#pragma once

#include "Format.h"
#include "Markup.h"
class CMACDlg;

class CFormatPluginLevelInfo
{
public:
    
    CFormatPluginLevelInfo()
    {
    }
    
    CFormatPluginLevelInfo(CMarkup & XML)
    {
        m_strName = XML.GetTagName();
        m_aryInputExtensions.InitFromList(XML.GetChildData(_T("InputExtensions")), _T(";"));
        m_strOutputExtension = XML.GetChildData(_T("OutputExtension"));
        m_strApplication = XML.GetChildData(_T("Application"));
        m_strCommandLine = XML.GetChildData(_T("CommandLine"));
        m_strSuccessReturn = XML.GetChildData(_T("SuccessReturn"));
    }

    CString m_strName;
    CStringArrayEx m_aryInputExtensions;
    CString m_strOutputExtension;
    CString m_strApplication;
    CString m_strCommandLine;
    CString m_strSuccessReturn;
};

class CFormatPlugin : public IFormat
{
public:

    CFormatPlugin(CMACDlg * pMACDlg, int nIndex, const CString & strAPXFilename = _T(""));
    virtual ~CFormatPlugin();

    BOOL Load(const CString & strAPXFilename);

    virtual CString GetName() { return m_strName; }
    
    virtual int Process(MAC_FILE * pInfo);

    virtual BOOL BuildMenu(CMenu * pMenu, int nBaseID);
    virtual BOOL ProcessMenuCommand(int nCommand);

    virtual CString GetInputExtensions(MAC_MODES Mode);
    virtual CString GetOutputExtension(MAC_MODES Mode, const CString & strInputFilename, int nLevel);

protected:
    
    // helpers
    void ParseModeInfo(CMarkup & XML, MAC_MODES Mode, const CString & strKeyword);
    CFormatPluginLevelInfo * GetLevelInfo(MAC_MODES Mode, const CString & strInputFilename, int nLevel);
    
    // parent
    CMACDlg * m_pMACDlg;

    // properties
    BOOL m_bIsValid;
    int m_nIndex;
    
    // filename
    CString m_strAPXFilename;

    // general
    CString m_strName;
    CString m_strURL;
    CString m_strAuthor;
    CString m_strVersion;
    CString m_strDescription;

    // mode info
    CArray<CFormatPluginLevelInfo, CFormatPluginLevelInfo &> m_aryModeInfo[MODE_COUNT];

    // configuration
    BOOL m_bHasConfiguration;
    CString m_strConfigureDescription[3];
    CString m_strConfigureValue[3];
};
