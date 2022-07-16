#pragma once

void CALLBACK MACProgressCallback(int nPercentageDone);

#include "MACFileArray.h"

class CMACProcessFiles  
{
public:

    CMACProcessFiles();
    virtual ~CMACProcessFiles();

    BOOL Process(MAC_FILE_ARRAY * paryFiles);
    BOOL ProcessFile(int nIndex);

    BOOL UpdateProgress(double dPercentageDone);
    
    BOOL Pause(BOOL bPause);
    BOOL Stop(BOOL bImmediately);

    inline BOOL GetPaused() { return m_bPaused; }
    inline BOOL GetStopped() { return m_bStopped; }
    int GetPausedTotalMS();

protected:

    // helpers
    void Destroy();

    // data
    MAC_FILE_ARRAY * m_paryFiles;
    BOOL m_bStopped;
    BOOL m_bPaused;
    unsigned long long m_nPausedStartTickCount;
    int64 m_nPausedTotalMS;

    // thread for processing
    static void ProcessFileThread(void * pVoid);
};
