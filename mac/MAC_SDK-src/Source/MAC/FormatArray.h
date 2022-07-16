#pragma once

#include "Format.h"

class MAC_FILE;
class CMACDlg;

class CFormatArray  
{
public:

    CFormatArray(CMACDlg * pMACDlg);
    virtual ~CFormatArray();

    BOOL Load();
    BOOL Unload();
    
    BOOL FillCompressionMenu(CMenu * pMenu);
    BOOL ProcessCompressionMenu(int nID);

    IFormat * GetFormat(const CString & strName);
    
    int Process(MAC_FILE * pInfo);

    CString GetOutputExtension(MAC_MODES Mode, const CString & strInputFilename, int nLevel, IFormat * pFormat);
    BOOL GetInputExtensions(CStringArrayEx & aryExtensions);
    CString GetOpenFilesFilter(BOOL bAddAllFiles = TRUE);
    IFormat * GetFormat(MAC_FILE * pInfo);
    IFormat * GetFormatFromInputType(const CString & strInputExtension);

protected:
    
    CMACDlg * m_pMACDlg;
    CArray<IFormat *, IFormat *> m_aryFormats;
    CArray<CMenu *, CMenu *> m_aryMenus;

    void ClearMenuCache();
};

