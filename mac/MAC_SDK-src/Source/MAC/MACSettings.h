#pragma once

#define COMPRESSION_APE                                      _T("Monkey's Audio")

#define OUTPUT_LOCATION_MODE_SAME_DIRECTORY                  0
#define OUTPUT_LOCATION_MODE_SPECIFIED_DIRECTORY             1

#define OUTPUT_EXISTS_MODE_SKIP                              0
#define OUTPUT_EXISTS_MODE_RENAME                            1
#define OUTPUT_EXISTS_MODE_OVERWRITE                         2

#define OUTPUT_DELETE_AFTER_SUCCESS_MODE_NONE                0
#define OUTPUT_DELETE_AFTER_SUCCESS_MODE_RECYCLE_SOURCE      1
#define OUTPUT_DELETE_AFTER_SUCCESS_MODE_DELETE_SOURCE       2

#define PROCESSING_PRIORITY_MODE_IDLE                        0
#define PROCESSING_PRIORITY_MODE_LOW                         1
#define PROCESSING_PRIORITY_MODE_NORMAL                      2
#define PROCESSING_PRIORITY_MODE_HIGH                        4

#define PROCESSING_VERIFY_MODE_FULL                          0
#define PROCESSING_VERIFY_MODE_QUICK                         1

class CMACSettings
{
public:
    // construction / destruction
    CMACSettings();
    virtual ~CMACSettings();

    // load / save
    bool Save();
    bool Load();

    // load / save individual
    int LoadSetting(const CString & strName, int nDefault);
    bool LoadSettingBoolean(const CString & strName, bool bDefault);
    CString LoadSetting(const CString & strName, const CString & strDefault, int nMaxLength = 8192);
    bool LoadSetting(const CString & strName, void * pData, int nBytes);

    void SaveSetting(const CString & strName, int nValue);
    void SaveSetting(const CString & strName, const CString & strValue);
    void SaveSetting(const CString & strName, void * pData, int nBytes);

    // general settings
    CString m_strAPLFilenameTemplate;
    CStringArrayEx m_aryAddFilesMRU;
    CStringArrayEx m_aryAddFolderMRU;

    // output settings
    int m_nOutputLocationMode;
    CString m_strOutputLocationDirectory;
    bool m_bOutputLocationRecreateDirectoryStructure;
    int m_nOutputLocationRecreateDirectoryStructureLevels;
    int m_nOutputExistsMode;
    int m_nOutputDeleteAfterSuccessMode;
    bool m_bOutputMirrorTimeStamp;

    // processing settings
    int m_nProcessingSimultaneousFiles;
    int m_nProcessingPriorityMode;
    bool m_bProcessingStopOnErrors;
    bool m_bProcessingPlayCompletionSound;
    bool m_bProcessingShowExternalWindows;
    int m_nProcessingVerifyMode;
    bool m_bProcessingAutoVerifyOnCreation;

    // helpers
    inline APE::APE_MODES GetMode() const { return m_Mode; }
    inline CString GetFormat() const { return m_strFormat; }
    inline int GetLevel() const { return m_nLevel;}
    void SetMode(APE::APE_MODES Mode);
    void SetCompression(const CString & strFormat, int nLevel);
    CString GetModeName();
    CString GetCompressionName();

protected:
    APE::APE_MODES m_Mode;
    CString m_strFormat;
    int m_nLevel;

    CRegKey m_RegKey;
    bool m_bValid;
};
