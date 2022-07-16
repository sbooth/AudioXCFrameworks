#include "All.h"
#include "MACProgressHelper.h"
#include "MACLib.h"

namespace APE
{

CMACProgressHelper::CMACProgressHelper(int64 nTotalSteps, IAPEProgressCallback * pProgressCallback)
{
    m_pProgressCallback = pProgressCallback;

    m_nTotalSteps = nTotalSteps;
    m_nCurrentStep = 0;
    m_nLastCallbackFiredPercentageDone = 0;

    UpdateProgress(0);
}

CMACProgressHelper::~CMACProgressHelper()
{

}

void CMACProgressHelper::UpdateProgress(int64 nCurrentStep, bool bForceUpdate)
{
    // update the step
     if (nCurrentStep == -1)
         m_nCurrentStep++;
     else
        m_nCurrentStep = nCurrentStep;

    // figure the percentage done
     double dPercentageDone = double(m_nCurrentStep) / double(ape_max(m_nTotalSteps, 1));
    int nPercentageDone = (int) (dPercentageDone * 1000 * 100);
    if (nPercentageDone > 100000) nPercentageDone = 100000;

    // fire the callback
    if (m_pProgressCallback != NULL)
    {
        if (bForceUpdate || (nPercentageDone - m_nLastCallbackFiredPercentageDone) >= 1000)
        {
            m_pProgressCallback->Progress(nPercentageDone);
            m_nLastCallbackFiredPercentageDone = nPercentageDone;
        }
    }
}

int CMACProgressHelper::ProcessKillFlag(bool bSleep)
{
    // process any messages (allows repaint, etc.)
    if (bSleep)
    {
        PUMP_MESSAGE_LOOP
    }

    if (m_pProgressCallback)
    {
        while (m_pProgressCallback->GetKillFlag() == KILL_FLAG_PAUSE)
        {
            SLEEP(50);
            PUMP_MESSAGE_LOOP
        }

        if ((m_pProgressCallback->GetKillFlag() != KILL_FLAG_CONTINUE) && (m_pProgressCallback->GetKillFlag() != KILL_FLAG_PAUSE))
        {
            return ERROR_UNDEFINED;
        }
    }

    return ERROR_SUCCESS;
}

}