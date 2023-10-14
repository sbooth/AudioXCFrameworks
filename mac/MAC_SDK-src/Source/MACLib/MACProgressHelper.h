#pragma once

namespace APE
{

#define KILL_FLAG_CONTINUE          0
#define KILL_FLAG_PAUSE             -1
#define KILL_FLAG_STOP              1

class IAPEProgressCallback;

#pragma pack(push, 1)

class CMACProgressHelper
{
public:
    CMACProgressHelper(int64 nTotalSteps, IAPEProgressCallback * pProgressCallback);

    void UpdateProgress(int64 nCurrentStep = -1, bool bForceUpdate = false);
    void UpdateProgressComplete();

    int ProcessKillFlag();

private:
    int64 m_nTotalSteps;
    int64 m_nCurrentStep;
    int m_nLastCallbackFiredPercentageDone;
    IAPEProgressCallback * m_pProgressCallback;
};

#pragma pack(pop)

}
