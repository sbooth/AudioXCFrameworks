#include "stdafx.h"
#include "MAC.h"
#include "MACSettings.h"
#include "FormatArray.h"

CMACSettings::CMACSettings()
{
    m_bValid = FALSE;

#if defined(_M_X64)
    if (m_RegKey.Create(HKEY_CURRENT_USER, _T("Software\\Monkey's Audio x64\\Settings")) == ERROR_SUCCESS)
        m_bValid = TRUE;
#else
    if (m_RegKey.Create(HKEY_CURRENT_USER, _T("Software\\Monkey's Audio\\Settings")) == ERROR_SUCCESS)
        m_bValid = TRUE;
#endif

    Load();
}

CMACSettings::~CMACSettings()
{
    Save();
}


BOOL CMACSettings::Save()
{
    // general
    SaveSetting(_T("Mode"), (int) m_Mode);
    SaveSetting(_T("Compression Main"), m_strFormat);
    SaveSetting(_T("Compression Sub"), m_nLevel);
    SaveSetting(_T("APL Filename Template"), m_strAPLFilenameTemplate);
    SaveSetting(_T("Add Files MRU List"), m_aryAddFilesMRU.GetList(_T("|")));
    SaveSetting(_T("Add Folder MRU List"), m_aryAddFolderMRU.GetList(_T("|")));
    
    // output
    SaveSetting(_T("Output Location Mode"), m_nOutputLocationMode);
    SaveSetting(_T("Output Location Directory"), m_strOutputLocationDirectory);
    SaveSetting(_T("Output Location Recreate Directory Structure"), m_bOutputLocationRecreateDirectoryStructure);
    SaveSetting(_T("Output Location Recreate Directory Structure Levels"), m_nOutputLocationRecreateDirectoryStructureLevels);
    SaveSetting(_T("Output Exists Mode"), m_nOutputExistsMode);
    SaveSetting(_T("Output Delete After Success Mode"), m_nOutputDeleteAfterSuccessMode);
    SaveSetting(_T("Output Mirror Time Stamp"), m_bOutputMirrorTimeStamp);

    // processing
    SaveSetting(_T("Processing Simultaneous Files"), m_nProcessingSimultaneousFiles);
    SaveSetting(_T("Processing Priority Mode"), m_nProcessingPriorityMode);
    SaveSetting(_T("Processing Stop On Errors"), m_bProcessingStopOnErrors);
    SaveSetting(_T("Processing Play Completion Sound"), m_bProcessingPlayCompletionSound);
    SaveSetting(_T("Processing Show External Windows"), m_bProcessingShowExternalWindows);
    SaveSetting(_T("Processing Verify Mode"), m_nProcessingVerifyMode);
    SaveSetting(_T("Processing Automatic Verify On Creation"), m_bProcessingAutoVerifyOnCreation);
    
    return TRUE;
}

BOOL CMACSettings::Load()
{
    // general
    SetMode((MAC_MODES) LoadSetting(_T("Mode"), (int) MODE_COMPRESS));

    m_strFormat = LoadSetting(_T("Compression Main"), COMPRESSION_APE);
    m_nLevel = LoadSetting(_T("Compression Sub"), MAC_COMPRESSION_LEVEL_NORMAL);
    m_strAPLFilenameTemplate = LoadSetting(_T("APL Filename Template"), _T("ARTIST - ALBUM - TRACK# - TITLE"));
    m_aryAddFilesMRU.InitFromList(LoadSetting(_T("Add Files MRU List"), _T("")), _T("|"));
    m_aryAddFolderMRU.InitFromList(LoadSetting(_T("Add Folder MRU List"), _T("")), _T("|"));
    
    // output
    m_nOutputLocationMode = LoadSetting(_T("Output Location Mode"), OUTPUT_LOCATION_MODE_SAME_DIRECTORY);
    m_strOutputLocationDirectory = LoadSetting(_T("Output Location Directory"), _T("C:\\"));
    m_bOutputLocationRecreateDirectoryStructure = LoadSetting(_T("Output Location Recreate Directory Structure"), FALSE);
    m_nOutputLocationRecreateDirectoryStructureLevels = LoadSetting(_T("Output Location Recreate Directory Structure Levels"), 2);
    m_nOutputExistsMode = LoadSetting(_T("Output Exists Mode"), OUTPUT_EXISTS_MODE_RENAME);
    m_nOutputDeleteAfterSuccessMode = LoadSetting(_T("Output Delete After Success Mode"), OUTPUT_DELETE_AFTER_SUCCESS_MODE_NONE);
    m_bOutputMirrorTimeStamp = LoadSetting(_T("Output Mirror Time Stamp"), FALSE);

    // processing
    m_nProcessingSimultaneousFiles = LoadSetting(_T("Processing Simultaneous Files"), 2);
    m_nProcessingPriorityMode = LoadSetting(_T("Processing Priority Mode"), PROCESSING_PRIORITY_MODE_NORMAL);
    m_bProcessingStopOnErrors = LoadSetting(_T("Processing Stop On Errors"), TRUE);
    m_bProcessingPlayCompletionSound = LoadSetting(_T("Processing Play Completion Sound"), FALSE);
    m_bProcessingShowExternalWindows = LoadSetting(_T("Processing Show External Windows"), TRUE);
    m_nProcessingVerifyMode = LoadSetting(_T("Processing Verify Mode"), PROCESSING_VERIFY_MODE_QUICK);
    m_bProcessingAutoVerifyOnCreation = LoadSetting(_T("Processing Automatic Verify On Creation"), FALSE);

    return FALSE;
}

int CMACSettings::LoadSetting(const CString & strName, int nDefault)
{
    if (m_bValid == FALSE) return nDefault;

    int nValue = nDefault;
    m_RegKey.QueryDWORDValue(strName, (DWORD &)nValue);
    return nValue;
}

CString CMACSettings::LoadSetting(const CString & strName, const CString & strDefault, int nMaxLength)
{
    if (m_bValid == FALSE) return strDefault;

    CString strValue;

    DWORD dwLength = nMaxLength;
    LONG nRetVal = m_RegKey.QueryStringValue(strName, strValue.GetBuffer(dwLength + 1), &dwLength);
    strValue.ReleaseBuffer();

    if (nRetVal != ERROR_SUCCESS)
        strValue = strDefault;

    return strValue;
}

BOOL CMACSettings::LoadSetting(const CString & strName, void * pData, int nBytes)
{
    if (m_bValid == FALSE) return FALSE;

    DWORD dwDataType = REG_BINARY; DWORD dwBytes = nBytes;
    if (RegQueryValueEx(m_RegKey.m_hKey, strName, 0, &dwDataType, (LPBYTE) pData, &dwBytes) == ERROR_SUCCESS)
    {
        if (int(dwBytes) == nBytes)
            return TRUE;
    }

    return FALSE;
}


void CMACSettings::SaveSetting(const CString & strName, int nValue)
{
    if (m_bValid == FALSE) return;

    m_RegKey.SetDWORDValue(strName, nValue);
}

void CMACSettings::SaveSetting(const CString & strName, const CString & strValue)
{
    if (m_bValid == FALSE) return;
    
    m_RegKey.SetStringValue(strName, strValue);
}

void CMACSettings::SaveSetting(const CString & strName, void * pData, int nBytes)
{
    if (m_bValid == FALSE) return;

    RegSetValueEx(m_RegKey.m_hKey, strName, 0, REG_BINARY, (LPBYTE) pData, nBytes);
}

void CMACSettings::SetMode(MAC_MODES Mode)
{
    // mode
    m_Mode = Mode;
}

void CMACSettings::SetCompression(const CString & strFormat, int nLevel)
{
    m_strFormat = strFormat;
    m_nLevel = nLevel;
}

CString CMACSettings::GetModeName()
{
    CString strRetVal;
    if ((m_Mode >= 0) && (m_Mode < MODE_COUNT))
        strRetVal = g_aryModeNames[(int) m_Mode];

    return strRetVal;
}

CString CMACSettings::GetCompressionName()
{
    CString strRetVal;

    if ((m_Mode == MODE_COMPRESS) || (m_Mode == MODE_CONVERT))
    {
        if (GetFormat() == COMPRESSION_APE)
            strRetVal = GetAPECompressionName(GetLevel());
        else
            strRetVal = _T("External");
    }
    
    return strRetVal;
}

CString CMACSettings::GetAPECompressionName(int nAPELevel)
{
    CString strRetVal;

    if (nAPELevel == MAC_COMPRESSION_LEVEL_FAST)
        strRetVal = _T("Fast");
    else if (nAPELevel == MAC_COMPRESSION_LEVEL_NORMAL)
        strRetVal = _T("Normal");
    else if (nAPELevel == MAC_COMPRESSION_LEVEL_HIGH)
        strRetVal = _T("High");
    else if (nAPELevel == MAC_COMPRESSION_LEVEL_EXTRA_HIGH)
        strRetVal = _T("Extra High");
    else if (nAPELevel == MAC_COMPRESSION_LEVEL_INSANE)
        strRetVal = _T("Insane");
    
    return strRetVal;
}