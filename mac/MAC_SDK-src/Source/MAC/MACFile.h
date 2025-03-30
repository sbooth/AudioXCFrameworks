#pragma once

class CMACProcessFiles;
class IFormat;

class MAC_FILE
{
public:
    // construction / destruction
    MAC_FILE();

    // data
    CString m_strInputFilename;
    CString m_strOutputFilename;
    CString m_strWorkingFilename;
    double m_dInputFileBytes;
    double m_dOutputFileBytes;

    // processing info
    CMACProcessFiles * m_pMACProcessFiles;
    APE::APE_MODES m_Mode;
    int m_nStageProgress;
    bool m_bProcessing;
    bool m_bDone;
    bool m_bStarted;
    bool m_bNeedsUpdate;
    int m_nRetVal;
    TICK_COUNT_TYPE m_dwStartTickCount;
    TICK_COUNT_TYPE m_dwEndTickCount;
    CString m_strFormat;
    int m_nLevel;
    int m_nKillFlag;
    int m_nCurrentStage;
    int m_nTotalStages;
    IFormat * m_pFormat;
    bool m_bEmptyExtension;
    bool m_bOverwriteInput;
    int m_nThreads;

    // helpers
    bool PrepareForProcessing(CMACProcessFiles * pProcessFiles);
    void CalculateFilenames();
    double GetProgress() const;
    CString GetOutputExtension() const;
    inline bool GetNeverStarted() const { return (m_bDone == false) && (m_bStarted == false); }
    inline bool GetRunning() const { return (m_bDone == false) && (m_bStarted != false); }
};
