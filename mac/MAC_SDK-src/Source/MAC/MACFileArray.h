#pragma once

#include "MACFile.h"

class MAC_FILE_ARRAY : public CArray<MAC_FILE, MAC_FILE &>
{
public:
    // construction / destruction
    MAC_FILE_ARRAY();
    virtual ~MAC_FILE_ARRAY();

    // operations
    bool PrepareForProcessing(CMACProcessFiles * pProcessFiles);

    double GetTotalInputBytes();
    double GetTotalOutputBytes();
    bool GetProcessingProgress(double & rdProgress, double & rdSecondsLeft, int nPausedTotalMS);
    bool GetProcessingInfo(bool bStopped, int & rnRunning, bool & rbAllDone);

    bool GetContainsFile(const CString & strFilename);

    // data
    ULONGLONG m_dwStartProcessingTickCount;
};
