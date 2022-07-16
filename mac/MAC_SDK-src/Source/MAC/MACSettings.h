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

const LPCTSTR g_aryModeNames[APE::MODE_COUNT] = { _T("Compress"), _T("Decompress"), _T("Verify"), _T("Convert"), _T("Make APL's") };
const LPCTSTR g_aryModeActionNames[APE::MODE_COUNT] = { _T("Compressing"), _T("Decompressing"), _T("Verifying"), _T("Converting"), _T("Making APL's") };

class CMACSettings  
{
public:

    // construction / destruction
    CMACSettings();
    virtual ~CMACSettings();

    // load / save
    BOOL Save();
    BOOL Load();

    // load / save individual
    int LoadSetting(const CString & strName, int nDefault);
    CString LoadSetting(const CString & strName, const CString & strDefault, int nMaxLength = 8192);
    BOOL LoadSetting(const CString & strName, void * pData, int nBytes);

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
    BOOL m_bOutputLocationRecreateDirectoryStructure;
    int m_nOutputLocationRecreateDirectoryStructureLevels;
    int m_nOutputExistsMode;
    int m_nOutputDeleteAfterSuccessMode;
    BOOL m_bOutputMirrorTimeStamp;

    // processing settings
    int m_nProcessingSimultaneousFiles;
    int m_nProcessingPriorityMode;
    BOOL m_bProcessingStopOnErrors;
    BOOL m_bProcessingPlayCompletionSound;
    BOOL m_bProcessingShowExternalWindows;
    int m_nProcessingVerifyMode;
    BOOL m_bProcessingAutoVerifyOnCreation;

    // helpers
    inline APE::MAC_MODES GetMode() const { return m_Mode; }
    inline CString GetFormat() const { return m_strFormat; }
    inline int GetLevel() const { return m_nLevel;}
    void SetMode(APE::MAC_MODES Mode);
    void SetCompression(const CString & strFormat, int nLevel);
    CString GetModeName();
    CString GetCompressionName();
    CString GetAPECompressionName(int nAPELevel);
    
protected:
    
    APE::MAC_MODES m_Mode;
    CString m_strFormat;
    int m_nLevel;

    CRegKey m_RegKey;
    BOOL m_bValid;
};
