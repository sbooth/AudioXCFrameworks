#pragma once

#include "MACFile.h"

class MAC_FILE_ARRAY : public CArray<MAC_FILE, MAC_FILE &>
{
public:

    // construction / destruction
    MAC_FILE_ARRAY();
    virtual ~MAC_FILE_ARRAY();

    // operations
    BOOL PrepareForProcessing(CMACProcessFiles * pProcessFiles);

    double GetTotalInputBytes();
    double GetTotalOutputBytes();
    BOOL GetProcessingProgress(double & rdProgress, double & rdSecondsLeft, int nPausedTotalMS);
    BOOL GetProcessingInfo(BOOL bStopped, int & rnRunning, BOOL & rbAllDone);

    BOOL GetContainsFile(const CString & strFilename);

    // data
    DWORD m_dwStartProcessingTickCount;
};
