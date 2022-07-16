#include "stdafx.h"
#include "MAC.h"
#include "FormatArray.h"
#include "FormatPlugin.h"
#include "FormatAPE.h"
#include "MACFile.h"
#include "MACDlg.h"

CFormatArray::CFormatArray(CMACDlg * pMACDlg)
{
    m_pMACDlg = pMACDlg;
}

CFormatArray::~CFormatArray()
{
    Unload();
}

BOOL CFormatArray::Load()
{
    // remove all
    Unload();

    // APE
    m_aryFormats.Add(new CFormatAPE(0));

    // plugins
    CStringArray aryFiles;
    ListFiles(&aryFiles, GetProgramPath() + _T("External\\"));
    for (int z = 0; z < aryFiles.GetSize(); z++)
    {
        if (CFilename(aryFiles[z]).GetExtension() == _T(".apx"))
        {
            IFormat * pPlugin = new CFormatPlugin(m_pMACDlg, int(m_aryFormats.GetSize()), aryFiles[z]);
            m_aryFormats.Add(pPlugin);
        }
    }

    return TRUE;
}

BOOL CFormatArray::Unload()
{
    for (int z = 0; z < m_aryFormats.GetSize(); z++)
    {
        SAFE_DELETE(m_aryFormats[z])
    }
    m_aryFormats.RemoveAll();

    ClearMenuCache();

    return TRUE;
}

void CFormatArray::ClearMenuCache()
{
    for (int z = 0; z < m_aryMenus.GetSize(); z++)
    {
        m_aryMenus[z]->DestroyMenu();
        SAFE_DELETE(m_aryMenus[z]);
    }
    m_aryMenus.RemoveAll();
}

BOOL CFormatArray::FillCompressionMenu(CMenu * pMenu)
{
    // clear the menu cache
    ClearMenuCache();

    // error check
    if (m_aryFormats.GetSize() <= 0)
        return FALSE;

    // primary (not nested)
    m_aryFormats[0]->BuildMenu(pMenu, ID_SET_COMPRESSION_FIRST + (100 * (0 + 1)));

    // non-primary
    pMenu->AppendMenu(MF_SEPARATOR);
    
    CMenu * pExternalMenu = new CMenu; pExternalMenu->CreatePopupMenu(); m_aryMenus.Add(pExternalMenu);
    pMenu->AppendMenu(MF_POPUP, (UINT_PTR) pExternalMenu->GetSafeHmenu(), _T("E&xternal"));
    
    if (m_aryFormats.GetSize() <= 1)
    {
        pExternalMenu->AppendMenu(MF_STRING | MF_GRAYED, 0, _T("n/a"));
    }
    else
    {
        for (int z = 1; z < m_aryFormats.GetSize(); z++)
        {
            CMenu * pNewMenu = new CMenu; pNewMenu->CreatePopupMenu();
            if (m_aryFormats[z]->BuildMenu(pNewMenu, ID_SET_COMPRESSION_FIRST + (100 * (z + 1))))
            {
                m_aryMenus.Add(pNewMenu);
                pExternalMenu->AppendMenu(MF_POPUP, (UINT_PTR)pNewMenu->GetSafeHmenu(), m_aryFormats[z]->GetName());
            }
            else
            {
                delete pNewMenu;
            }
        }
    }

    return TRUE;
}

BOOL CFormatArray::ProcessCompressionMenu(int nID)
{
    nID -= ID_SET_COMPRESSION_FIRST;

    int nIndex = (nID / 100);
    int nCommand = nID - (nIndex * 100);
    nIndex -= 1;

    if (nIndex == -1)
    {
        // local commands (i.e. "Refresh Plugins")
    }
    else if ((nIndex >= 0) && (nIndex < m_aryFormats.GetSize()))
    {
        // plugin commands
        m_aryFormats[nIndex]->ProcessMenuCommand(nCommand);
    }

    return TRUE;
}

int CFormatArray::Process(MAC_FILE * pInfo)
{
    int nRetVal = ERROR_INVALID_INPUT_FILE;

    // use the format
    IFormat * pFormat = pInfo->pFormat;

    // if we found a format that can do the job, use it to do the work
    if (pFormat != NULL)
    {
        nRetVal = pFormat->Process(pInfo);
    }

    return nRetVal;
}

IFormat * CFormatArray::GetFormat(const CString & strName)
{
    for (int z = 0; z < m_aryFormats.GetSize(); z++)
    {
        if (m_aryFormats[z]->GetName() == strName)
            return m_aryFormats[z];
    }

    return NULL;
}
    
CString CFormatArray::GetOutputExtension(MAC_MODES Mode, const CString & strInputFilename, int nLevel, IFormat * pFormat)
{
    CString strExtension;

    if (Mode == MODE_MAKE_APL)
    {
        strExtension = _T(".apl");
    }
    else if (pFormat != NULL)
    {
        strExtension = pFormat->GetOutputExtension(Mode, strInputFilename, nLevel);
    }
        
    return strExtension;
}

BOOL CFormatArray::GetInputExtensions(CStringArrayEx & aryExtensions)
{
    // clear the list
    aryExtensions.RemoveAll();
    
    // build a map of extensions (so we remove duplicates)
    CMap<CString, LPCTSTR, BOOL, BOOL> mapExtensions;
    for (int z = 0; z < m_aryFormats.GetSize(); z++)
    {
        CString strExtensions = m_aryFormats[z]->GetInputExtensions(theApp.GetSettings()->GetMode());
        CStringArrayEx aryFormatExtensions; aryFormatExtensions.InitFromList(strExtensions, _T(";"));
        for (int nFormatExtension = 0; nFormatExtension < aryFormatExtensions.GetSize(); nFormatExtension++)
        {
            // "clean" the extension
            CString strExtension = aryFormatExtensions[nFormatExtension];
            if (strExtension.Left(1) != _T(".")) strExtension = _T(".") + strExtension;
            strExtension.MakeLower();

            // add it to the map
            mapExtensions.SetAt(strExtension, TRUE);
        }
    }

    // add all the extensions to the array
    POSITION Pos = mapExtensions.GetStartPosition();
    while (Pos)
    {
        CString strExtension; BOOL bTemp = FALSE;
        mapExtensions.GetNextAssoc(Pos, strExtension, bTemp);
        aryExtensions.Add(strExtension);
    }

    // sort
    aryExtensions.SortAscending();

    return TRUE;
}

CString CFormatArray::GetOpenFilesFilter(BOOL bAddAllFiles)
{
    // filter
    CString strFilter;

    // get the input extensions
    CStringArrayEx aryExtensions;
    GetInputExtensions(aryExtensions);

    // build the supported extensions expression
    CString strSupportedExtensions;
    for (int z = 0; z < aryExtensions.GetSize(); z++)
        strSupportedExtensions += _T("*") + aryExtensions[z] + _T(";");
    strSupportedExtensions.TrimRight(_T(";"));
    strFilter.Format(_T("All Supported Files (%s)|%s|"), (LPCTSTR) strSupportedExtensions, (LPCTSTR) strSupportedExtensions);

    // build the list on a per-format basis
    for (int z = 0; z < m_aryFormats.GetSize(); z++)
    {
        // get the format's extensions
        aryExtensions.InitFromList(m_aryFormats[z]->GetInputExtensions(theApp.GetSettings()->GetMode()), _T(";"));
        
        if (aryExtensions.GetSize() > 0)
        {
            // build the extension list
            CString strFormatExtensions;
            for (int nExtension = 0; nExtension < aryExtensions.GetSize(); nExtension++)
                strFormatExtensions += _T("*") + aryExtensions[nExtension] + _T(";");
            strFormatExtensions.TrimRight(_T(";"));

            // build the filter string
            CString strFormatFilter;
            strFormatFilter.Format(_T("%s Files (%s)|%s|"), (LPCTSTR) m_aryFormats[z]->GetName(), (LPCTSTR) strFormatExtensions, (LPCTSTR) strFormatExtensions);
            strFilter += strFormatFilter;
        }
    }
    
    // all files
    if (bAddAllFiles)
        strFilter += _T("All Files (*.*)|*.*|");

    // terminate
    strFilter.TrimRight(_T("|"));
    strFilter += _T("||");

    // return
    return strFilter;
}


IFormat * CFormatArray::GetFormat(MAC_FILE * pInfo)
{
    IFormat * pFormat = NULL;

    if ((pInfo->Mode == MODE_DECOMPRESS) || (pInfo->Mode == MODE_VERIFY))
    {
        // find the first plugin that supports this file type
        CString strExtension = CFilename(pInfo->strInputFilename).GetExtension();
        for (int z = 0; z < m_aryFormats.GetSize(); z++)
        {
            CStringArrayEx aryExtensions;
            aryExtensions.InitFromList(m_aryFormats[z]->GetInputExtensions(pInfo->Mode), _T(";"));
            if (aryExtensions.Find(strExtension) != -1)
            {
                pFormat = m_aryFormats[z];
                break;
            }
        }
    }
    else
    {
        pFormat = GetFormat(theApp.GetSettings()->GetFormat());
    }

    return pFormat;
}

IFormat * CFormatArray::GetFormatFromInputType(const CString & strInputExtension)
{
    for (int z = 0; z < m_aryFormats.GetSize(); z++)
    {
        CStringArrayEx aryExtensions;
        aryExtensions.InitFromList(m_aryFormats[z]->GetInputExtensions(MODE_DECOMPRESS), _T(";"));
        if (aryExtensions.Find(strInputExtension) != -1)
        {
            return m_aryFormats[z];
        }
    }

    return NULL;
}