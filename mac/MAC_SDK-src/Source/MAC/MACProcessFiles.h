#pragma once

#include "MACFileArray.h"
#include <thread>

class CMACProcessFiles
{
public:
    CMACProcessFiles();
    virtual ~CMACProcessFiles();

    bool Process(MAC_FILE_ARRAY * paryFiles);
    bool ProcessFile(int nIndex);

    bool UpdateProgress(double dPercentageDone);

    bool Pause(bool bPause);
    bool Stop(bool bImmediately);

    inline bool GetPaused() const { return m_bPaused; }
    inline bool GetStopped() const { return m_bStopped; }
    int GetPausedTotalMS() const;
    int GetSize() const { return static_cast<int>(m_paryFiles->GetSize()); }

protected:
    // helpers
    void Destroy();

    // data
    MAC_FILE_ARRAY * m_paryFiles;
    bool m_bStopped;
    bool m_bPaused;
    unsigned long long m_nPausedStartTickCount;
    APE::int64 m_nPausedTotalMS;

    // thread for processing
    static void ProcessFileThread(MAC_FILE * pInfo);
};
