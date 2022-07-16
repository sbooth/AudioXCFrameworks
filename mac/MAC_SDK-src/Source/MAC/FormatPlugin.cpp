#include "stdafx.h"
#include "MAC.h"
#include "FormatPlugin.h"
#include "Markup.h"
#include "MACFile.h"
#include "FormatPluginConfigureDlg.h"
#include "FormatPluginInfoDlg.h"
#include "MACDlg.h"

CFormatPlugin::CFormatPlugin(CMACDlg * pMACDlg, int nIndex, const CString & strAPXFilename)
{
    // initialize
    m_pMACDlg = pMACDlg;
    m_nIndex = nIndex;
    m_bIsValid = FALSE;
    
    // load 
    if (strAPXFilename.IsEmpty() == FALSE)
        Load(strAPXFilename);
}

CFormatPlugin::~CFormatPlugin()
{

}

BOOL CFormatPlugin::Load(const CString & strAPXFilename)
{
    BOOL bRetVal = FALSE;

    m_strAPXFilename = strAPXFilename;

    CString strAPX;
    if (ReadWholeFile(m_strAPXFilename, strAPX))
    {
        // parse the XML
        CMarkup XML(strAPX);
        if (XML.FindElem(_T("APX")) && (XML.GetAttrib(_T("version")) == _T("1.0")))
        {
            // general info
            if (XML.FindChildElem(_T("General")))
            {
                XML.IntoElem();
                m_strName = XML.GetChildData(_T("Name"));
                m_strURL = XML.GetChildData(_T("URL"));
                m_strAuthor = XML.GetChildData(_T("Author"));
                m_strVersion = XML.GetChildData(_T("Version"));
                m_strDescription = XML.GetChildData(_T("Description"));
                XML.OutOfElem();
            }

            // mode info
            ParseModeInfo(XML, MODE_COMPRESS, _T("Compress"));
            ParseModeInfo(XML, MODE_DECOMPRESS, _T("Decompress"));
            ParseModeInfo(XML, MODE_VERIFY, _T("Verify"));
            ParseModeInfo(XML, MODE_CHECK, _T("Check"));

            // configure
            m_bHasConfiguration = FALSE;
            if (XML.FindChildElem(_T("Configure")))
            {
                m_bHasConfiguration = TRUE;
                
                XML.IntoElem();

                for (int z = 0; z < 3; z++)
                {
                    CString strName; strName.Format(_T("Configure%d"), z + 1);

                    if (XML.FindChildElem(strName))
                    {
                        XML.IntoElem();
                        m_strConfigureDescription[z] = XML.GetChildData(_T("Description"));
                        
                        CString strSetting;
                        strSetting.Format(_T("Format Plugin - %s - Configure %d"), (LPCTSTR) GetName(), z + 1);
                        m_strConfigureValue[z] = theApp.GetSettings()->LoadSetting(strSetting, XML.GetChildData(_T("DefaultValue")));

                        XML.OutOfElem();
                    }
                }

                XML.OutOfElem();
            }

            bRetVal = TRUE;
        }        
    }

    // check
    {
        for (int z = 0; z < m_aryModeInfo[MODE_CHECK].GetSize(); z++)
        {
            CFormatPluginLevelInfo & Level = m_aryModeInfo[MODE_CHECK].GetAt(z);
            if (Level.m_strApplication.GetLength() > 0)
            {
                MAC_FILE File;
                File.Mode = MODE_CHECK;
                if (Process(&File) != ERROR_SUCCESS)
                {
                    bRetVal = false;
                }
            }
        }
    }
    
    if (bRetVal)
        m_bIsValid = TRUE;

    return bRetVal;
}

void CFormatPlugin::ParseModeInfo(CMarkup & XML, MAC_MODES Mode, const CString & strKeyword)
{
    m_aryModeInfo[Mode].RemoveAll();

    if (XML.FindChildElem(strKeyword))
    {
        XML.IntoElem();

        while (XML.FindChildElem())
        {
            XML.IntoElem();
            CFormatPluginLevelInfo Level(XML);
            m_aryModeInfo[Mode].Add(Level);
            XML.OutOfElem();
        }

        XML.OutOfElem();
    }
}

BOOL CFormatPlugin::BuildMenu(CMenu * pMenu, int nBaseID)
{
    if (m_bIsValid)
    {
        for (int z = 0; z < m_aryModeInfo[MODE_COMPRESS].GetSize(); z++)
        {
            BOOL bChecked = (theApp.GetSettings()->GetFormat() == m_strName) &&
                (theApp.GetSettings()->GetLevel() == z);

            pMenu->AppendMenu(MF_STRING | (bChecked ? MF_CHECKED : MF_UNCHECKED), nBaseID + 10 + z, m_aryModeInfo[MODE_COMPRESS][z].m_strName);
        }

        pMenu->AppendMenu(MF_SEPARATOR);

        if (m_bHasConfiguration)
            pMenu->AppendMenu(MF_STRING, nBaseID + 0, _T("Configure..."));

        pMenu->AppendMenu(MF_STRING, nBaseID + 1, _T("Info..."));
    }

    return ((pMenu->GetSafeHmenu() != NULL) && m_bIsValid) ? TRUE : FALSE;
}

BOOL CFormatPlugin::ProcessMenuCommand(int nCommand)
{
    if (nCommand == 0)
    {
        CFormatPluginConfigureDlg dlgConfigure(m_pMACDlg, m_strConfigureDescription[0], m_strConfigureValue[0],
            m_strConfigureDescription[1], m_strConfigureValue[1], m_strConfigureDescription[2], m_strConfigureValue[2]);

        if (dlgConfigure.DoModal() == IDOK)
        {
            CString strSetting; 
            
            m_strConfigureValue[0] = dlgConfigure.m_strConfigureEdit1;
            strSetting.Format(_T("Format Plugin - %s - Configure 1"), (LPCTSTR) GetName());
            theApp.GetSettings()->SaveSetting(strSetting, m_strConfigureValue[0]);

            m_strConfigureValue[1] = dlgConfigure.m_strConfigureEdit2;
            strSetting.Format(_T("Format Plugin - %s - Configure 2"), (LPCTSTR) GetName());
            theApp.GetSettings()->SaveSetting(strSetting, m_strConfigureValue[1]);

            m_strConfigureValue[2] = dlgConfigure.m_strConfigureEdit3;
            strSetting.Format(_T("Format Plugin - %s - Configure 3"), (LPCTSTR) GetName());
            theApp.GetSettings()->SaveSetting(strSetting, m_strConfigureValue[2]);
        }
    }
    else if (nCommand == 1)
    {
        CFormatPluginInfoDlg dlgPluginInfo(m_pMACDlg, m_strName, m_strVersion, m_strAuthor, m_strDescription, m_strURL);
        dlgPluginInfo.DoModal();
    }
    else if ((nCommand >= 10) && (nCommand < 100))
    {
        int nMode = nCommand - 10;
        theApp.GetSettings()->SetCompression(GetName(), nMode);
    }
    
    return TRUE;
}

CString CFormatPlugin::GetOutputExtension(MAC_MODES Mode, const CString & strInputFilename, int nLevel)
{
    CString strExtension;

    CFormatPluginLevelInfo * pLevelInfo = GetLevelInfo(Mode, strInputFilename, nLevel);
    if (pLevelInfo != NULL)
    {
        strExtension = pLevelInfo->m_strOutputExtension;
    }

    return strExtension;
}

int CFormatPlugin::Process(MAC_FILE * pInfo)
{
    int nRetVal = ERROR_UNDEFINED;
    
    CFormatPluginLevelInfo * pLevelInfo = GetLevelInfo(pInfo->Mode, pInfo->strInputFilename, pInfo->nLevel);
    if (pLevelInfo != NULL)
    {
        CString strCommandLine = pLevelInfo->m_strCommandLine;
        strCommandLine.Replace(_T("[INPUT]"), _T("\"") + pInfo->strInputFilename + _T("\""));
        strCommandLine.Replace(_T("[OUTPUT]"), _T("\"") + pInfo->strWorkingFilename + _T("\""));
        strCommandLine.Replace(_T("[CONFIGURE 1]"), m_strConfigureValue[0]);
        strCommandLine.Replace(_T("[CONFIGURE 2]"), m_strConfigureValue[1]);
        strCommandLine.Replace(_T("[CONFIGURE 3]"), m_strConfigureValue[2]);

        pInfo->nStageProgress = 0;

        CString strPath = GetProgramPath() + _T("External\\");
        CString strApplication = strPath + pLevelInfo->m_strApplication;

        int nExitCode = -1;
        bool bShow = theApp.GetSettings()->m_bProcessingShowExternalWindows;
        if (pInfo->Mode == MODE_CHECK)
            bShow = false;
        BOOL bSuccess = ExecuteProgramBlocking(strApplication, strCommandLine, &nExitCode, bShow);
        
        if (pLevelInfo->m_strSuccessReturn.IsEmpty() == FALSE)
        {
            if (bSuccess && (nExitCode == _ttoi(pLevelInfo->m_strSuccessReturn)))
                nRetVal = ERROR_SUCCESS;
            else
                nRetVal = ERROR_UNDEFINED;
        }
        else
        {
            if (pLevelInfo->m_strCommandLine.Find(_T("[OUTPUT]")) != -1)
                nRetVal = (bSuccess && FileExists(pInfo->strWorkingFilename)) ? ERROR_SUCCESS : ERROR_UNDEFINED;
            else
                nRetVal = ERROR_SUCCESS;
        }
    }

    return nRetVal;
}

CString CFormatPlugin::GetInputExtensions(MAC_MODES Mode)
{
    CStringArrayEx aryExtensions;
    for (int z = 0; z < m_aryModeInfo[Mode].GetSize(); z++)
        aryExtensions.Append(m_aryModeInfo[Mode][z].m_aryInputExtensions);
        
    if (Mode == MODE_CONVERT)
    {
        for (int z = 0; z < m_aryModeInfo[MODE_DECOMPRESS].GetSize(); z++)
            aryExtensions.Append(m_aryModeInfo[MODE_DECOMPRESS][z].m_aryInputExtensions);
    }

    aryExtensions.RemoveDuplicates(FALSE);
    return aryExtensions.GetList(_T(";"));
}

CFormatPluginLevelInfo * CFormatPlugin::GetLevelInfo(MAC_MODES Mode, const CString & strInputFilename, int nLevel)
{
    // start with NULL
    CFormatPluginLevelInfo * pLevelInfo = NULL;

    // for decompress and verify, we pick the level info based on the input filename
    if (Mode == MODE_DECOMPRESS || Mode == MODE_VERIFY || Mode == MODE_CONVERT)
    {
        nLevel = -1;
        CString strExtension = CFilename(strInputFilename).GetExtension();
        for (int z = 0; z < m_aryModeInfo[Mode].GetSize(); z++)
        {
            if (m_aryModeInfo[Mode][z].m_aryInputExtensions.Find(strExtension) != -1)
            {
                nLevel = z;
                break;
            }
        }
    }

    // for check, we just pick the first
    if (Mode == MODE_CHECK)
    {
        nLevel = 0;
    }

    // if we found a suitable level, get it
    if ((nLevel >= 0) && (nLevel < m_aryModeInfo[Mode].GetSize()))
    {
        pLevelInfo = &m_aryModeInfo[Mode][nLevel];
    }

    // return
    return pLevelInfo;
}