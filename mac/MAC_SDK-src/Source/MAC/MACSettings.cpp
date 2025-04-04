#include "stdafx.h"
#include "MAC.h"
#include "MACSettings.h"
#include "FormatArray.h"
#include "APEInfo.h"
#include <thread>

using namespace APE;

CMACSettings::CMACSettings()
{
    m_bValid = false;

#if defined(_M_X64)
    if (m_RegKey.Create(HKEY_CURRENT_USER, _T("Software\\Monkey's Audio x64\\Settings")) == ERROR_SUCCESS)
        m_bValid = true;
#else
    if (m_RegKey.Create(HKEY_CURRENT_USER, _T("Software\\Monkey's Audio\\Settings")) == ERROR_SUCCESS)
        m_bValid = true;
#endif

    Load();
}

CMACSettings::~CMACSettings()
{
    Save();
}


bool CMACSettings::Save()
{
    // general
    SaveSetting(_T("Mode"), static_cast<int>(m_Mode));
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

    return true;
}

bool CMACSettings::Load()
{
    // general
    SetMode(static_cast<APE_MODES>(LoadSetting(_T("Mode"), static_cast<int>(MODE_COMPRESS))));

    m_strFormat = LoadSetting(_T("Compression Main"), COMPRESSION_APE);
    m_nLevel = LoadSetting(_T("Compression Sub"), APE_COMPRESSION_LEVEL_NORMAL);
    m_strAPLFilenameTemplate = LoadSetting(_T("APL Filename Template"), _T("ARTIST - ALBUM - TRACK# - TITLE"));
    m_aryAddFilesMRU.InitFromList(LoadSetting(_T("Add Files MRU List"), _T("")), _T("|"));
    m_aryAddFolderMRU.InitFromList(LoadSetting(_T("Add Folder MRU List"), _T("")), _T("|"));

    // output
    m_nOutputLocationMode = LoadSetting(_T("Output Location Mode"), OUTPUT_LOCATION_MODE_SAME_DIRECTORY);
    m_strOutputLocationDirectory = LoadSetting(_T("Output Location Directory"), _T("C:\\"));
    m_bOutputLocationRecreateDirectoryStructure = LoadSettingBoolean(_T("Output Location Recreate Directory Structure"), false);
    m_nOutputLocationRecreateDirectoryStructureLevels = LoadSetting(_T("Output Location Recreate Directory Structure Levels"), 2);
    m_nOutputExistsMode = LoadSetting(_T("Output Exists Mode"), OUTPUT_EXISTS_MODE_RENAME);
    m_nOutputDeleteAfterSuccessMode = LoadSetting(_T("Output Delete After Success Mode"), OUTPUT_DELETE_AFTER_SUCCESS_MODE_NONE);
    m_bOutputMirrorTimeStamp = LoadSettingBoolean(_T("Output Mirror Time Stamp"), false);

    // processing
    // std::thread::hardware_concurrency() returns 32 on my i9-13900KF
    int nDefaultSimultaneousFiles = ape_max(4, std::thread::hardware_concurrency() / 4);
    m_nProcessingSimultaneousFiles = LoadSetting(_T("Processing Simultaneous Files"), nDefaultSimultaneousFiles);
    m_nProcessingPriorityMode = LoadSetting(_T("Processing Priority Mode"), PROCESSING_PRIORITY_MODE_NORMAL);
    m_bProcessingStopOnErrors = LoadSettingBoolean(_T("Processing Stop On Errors"), true);
    m_bProcessingPlayCompletionSound = LoadSettingBoolean(_T("Processing Play Completion Sound"), false);
    m_bProcessingShowExternalWindows = LoadSettingBoolean(_T("Processing Show External Windows"), true);
    m_nProcessingVerifyMode = LoadSetting(_T("Processing Verify Mode"), PROCESSING_VERIFY_MODE_QUICK);
    m_bProcessingAutoVerifyOnCreation = LoadSettingBoolean(_T("Processing Automatic Verify On Creation"), false);

    return false;
}

int CMACSettings::LoadSetting(const CString & strName, int nDefault)
{
    if (m_bValid == false) return nDefault;

    int nValue = nDefault;
    m_RegKey.QueryDWORDValue(strName, reinterpret_cast<DWORD &>(nValue));
    return nValue;
}

bool CMACSettings::LoadSettingBoolean(const CString & strName, bool bDefault)
{
    if (m_bValid == false) return bDefault;

    int nValue = bDefault;
    m_RegKey.QueryDWORDValue(strName, reinterpret_cast<DWORD &>(nValue));

    return static_cast<bool>(nValue);
}

CString CMACSettings::LoadSetting(const CString & strName, const CString & strDefault, int nMaxLength)
{
    if (m_bValid == false) return strDefault;

    CString strValue;

    DWORD dwLength = static_cast<DWORD>(nMaxLength);
    LONG nRetVal = m_RegKey.QueryStringValue(strName, strValue.GetBuffer(nMaxLength + 1), &dwLength);
    strValue.ReleaseBuffer();

    if (nRetVal != ERROR_SUCCESS)
        strValue = strDefault;

    return strValue;
}

bool CMACSettings::LoadSetting(const CString & strName, void * pData, int nBytes)
{
    if (m_bValid == false) return false;

    DWORD dwDataType = REG_BINARY; DWORD dwBytes = static_cast<DWORD>(nBytes);
    if (RegQueryValueEx(m_RegKey.m_hKey, strName, APE_NULL, &dwDataType, static_cast<LPBYTE>(pData), &dwBytes) == ERROR_SUCCESS)
    {
        if (static_cast<int>(dwBytes) == nBytes)
            return true;
    }

    return false;
}


void CMACSettings::SaveSetting(const CString & strName, int nValue)
{
    if (m_bValid == false) return;

    m_RegKey.SetDWORDValue(strName, static_cast<DWORD>(nValue));
}

void CMACSettings::SaveSetting(const CString & strName, const CString & strValue)
{
    if (m_bValid == false) return;

    m_RegKey.SetStringValue(strName, strValue);
}

void CMACSettings::SaveSetting(const CString & strName, void * pData, int nBytes)
{
    if (m_bValid == false) return;

    RegSetValueEx(m_RegKey.m_hKey, strName, 0, REG_BINARY, static_cast<LPBYTE>(pData), nBytes);
}

void CMACSettings::SetMode(APE_MODES Mode)
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

    str_utfn cModeName[256]; APE_CLEAR(cModeName);
    GetAPEModeName(m_Mode, cModeName, 256, false);
    strRetVal = cModeName;

    return strRetVal;
}

CString CMACSettings::GetCompressionName()
{
    CString strRetVal;

    if ((m_Mode == MODE_COMPRESS) || (m_Mode == MODE_CONVERT))
    {
        if (GetFormat() == COMPRESSION_APE)
        {
            str_utfn cCompressionLevel[16]; APE_CLEAR(cCompressionLevel);
            GetAPECompressionLevelName(GetLevel(), cCompressionLevel, 16, true);
            strRetVal = cCompressionLevel;
        }
        else
        {
            strRetVal = _T("External");
        }
    }

    return strRetVal;
}
