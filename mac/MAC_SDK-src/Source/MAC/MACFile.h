#pragma once

class CMACProcessFiles;
class IFormat;

class MAC_FILE
{
public:

    // construction / destruction
    MAC_FILE();
    ~MAC_FILE();

    // data
    CString strInputFilename;
    CString strOutputFilename;
    CString strWorkingFilename;
    double dInputFileBytes;
    double dOutputFileBytes;
    
    // processing info
    BOOL bProcessing;
    CMACProcessFiles * pMACProcessFiles;
    MAC_MODES Mode;
    int nStageProgress;
    BOOL bDone;
    BOOL nRetVal;
    BOOL bStarted;
    BOOL bNeedsUpdate;
    TICK_COUNT_TYPE dwStartTickCount;
    TICK_COUNT_TYPE dwEndTickCount;
    CString strFormat;
    int nLevel;
    int nKillFlag;
    int nCurrentStage;
    int nTotalStages;
    IFormat * pFormat;
    APE::str_ansi m_cFileType[64];
    BOOL bEmptyExtension;

    // helpers
    BOOL PrepareForProcessing(CMACProcessFiles * pProcessFiles);
    void CalculateFilenames();
    double GetProgress();
    CString GetOutputExtension();
    inline BOOL GetNeverStarted() { return (bDone == FALSE) && (bStarted == FALSE); }
    inline BOOL GetRunning() { return (bDone == FALSE) && (bStarted != FALSE); }
};
