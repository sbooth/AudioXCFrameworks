#pragma once

namespace APE
{

#define KILL_FLAG_CONTINUE          0
#define KILL_FLAG_PAUSE             -1
#define KILL_FLAG_STOP              1

class IAPEProgressCallback;

class CMACProgressHelper  
{
public:
    
    CMACProgressHelper(int64 nTotalSteps, IAPEProgressCallback * pProgressCallback);
    virtual ~CMACProgressHelper();

    void UpdateProgress(int64 nCurrentStep = -1, bool bForceUpdate = false);
    void UpdateProgressComplete() { UpdateProgress(m_nTotalSteps, true); }

    int ProcessKillFlag(bool bSleep = true);
    
private:
    IAPEProgressCallback * m_pProgressCallback;
    int64 m_nTotalSteps;
    int64 m_nCurrentStep;
    int m_nLastCallbackFiredPercentageDone;
};

}
