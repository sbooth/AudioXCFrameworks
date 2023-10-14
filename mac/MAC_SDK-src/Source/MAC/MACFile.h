#pragma once

class CMACProcessFiles;
class IFormat;

class MAC_FILE
{
public:
    // construction / destruction
    MAC_FILE();

    // data
    CString strInputFilename;
    CString strOutputFilename;
    CString strWorkingFilename;
    double dInputFileBytes;
    double dOutputFileBytes;

    // processing info
    BOOL bProcessing;
    CMACProcessFiles * pMACProcessFiles;
    APE::APE_MODES Mode;
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
    BOOL bEmptyExtension;
    BOOL bOverwriteInput;

    // helpers
    BOOL PrepareForProcessing(CMACProcessFiles * pProcessFiles);
    void CalculateFilenames();
    double GetProgress() const;
    CString GetOutputExtension();
    inline BOOL GetNeverStarted() { return (bDone == FALSE) && (bStarted == FALSE); }
    inline BOOL GetRunning() { return (bDone == FALSE) && (bStarted != FALSE); }
};
