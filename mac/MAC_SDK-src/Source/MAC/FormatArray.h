#pragma once

#include "Format.h"

class MAC_FILE;
class CMACDlg;

class CFormatArray
{
public:
    CFormatArray(CMACDlg * pMACDlg);
    virtual ~CFormatArray();

    bool Load();
    bool Unload();

    bool FillCompressionMenu(CMenu * pMenu);
    bool ProcessCompressionMenu(int nID);

    IFormat * GetFormat(const CString & strName);

    int Process(MAC_FILE * pInfo);

    CString GetOutputExtension(APE::APE_MODES Mode, const CString & strInputFilename, int nLevel, IFormat * pFormat);
    bool GetInputExtensions(CStringArrayEx & aryExtensions);
    CString GetOpenFilesFilter(bool bAddAllFiles = true);
    IFormat * GetFormat(MAC_FILE * pInfo);
    IFormat * GetFormatFromInputType(const CString & strInputExtension);

protected:
    CMACDlg * m_pMACDlg;
    CArray<IFormat *, IFormat *> m_aryFormats;
    CArray<CMenu *, CMenu *> m_aryMenus;

    void ClearMenuCache();
};
