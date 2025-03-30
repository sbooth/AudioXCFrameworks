#include "stdafx.h"
#include "MAC.h"
#include "MACFileArray.h"

MAC_FILE_ARRAY::MAC_FILE_ARRAY() : CArray<MAC_FILE, MAC_FILE &>()
{
    m_dwStartProcessingTickCount = 0;
}

MAC_FILE_ARRAY::~MAC_FILE_ARRAY()
{
    RemoveAll();
}

bool MAC_FILE_ARRAY::PrepareForProcessing(CMACProcessFiles * pProcessFiles)
{
    // prepare the files
    for (int z = 0; z < GetSize(); z++)
        ElementAt(z).PrepareForProcessing(pProcessFiles);

    // store the start time
    m_dwStartProcessingTickCount = GetTickCount64();

    return true;
}

double MAC_FILE_ARRAY::GetTotalInputBytes()
{
    double dTotalBytes = 0;

    for (int z = 0; z < GetSize(); z++)
    {
        dTotalBytes += ElementAt(z).m_dInputFileBytes;
    }

    return dTotalBytes;
}

double MAC_FILE_ARRAY::GetTotalOutputBytes()
{
    double dTotalBytes = 0;

    for (int z = 0; z < GetSize(); z++)
    {
        dTotalBytes += ElementAt(z).m_dOutputFileBytes;
    }

    return dTotalBytes;
}

bool MAC_FILE_ARRAY::GetProcessingInfo(bool bStopped, int & rnRunning, bool & rbAllDone)
{
    rnRunning = 0;
    rbAllDone = true;

    for (int z = 0; z < GetSize(); z++)
    {
        MAC_FILE * pInfo = &ElementAt(z);

        // running
        if (pInfo->m_bStarted && (pInfo->m_bDone == false))
            rnRunning++;

        // all done (if never started and we've stopped, don't reset rbAllDone to false)
        if (pInfo->m_bStarted || (bStopped == false))
        {
            if (pInfo->m_bDone == false)
                rbAllDone = false;
        }
    }

    return true;
}

bool MAC_FILE_ARRAY::GetProcessingProgress(double & rdProgress, double & rdSecondsLeft, int nPausedTotalMS)
{
    double dTotal = 0;
    double dDone = 0;

    for (int z = 0; z < GetSize(); z++)
    {
        MAC_FILE * pInfo = &ElementAt(z);
        dTotal += pInfo->m_dInputFileBytes;

        if (pInfo->m_bDone == false)
        {
            if (pInfo->m_bStarted)
            {
                double dProgress = pInfo->GetProgress();
                dDone += pInfo->m_dInputFileBytes * dProgress;
            }
        }
        else
        {
            dDone += pInfo->m_dInputFileBytes;
        }
    }

    double dElapsed = static_cast<double>(GetTickCount64() - m_dwStartProcessingTickCount) - static_cast<double>(nPausedTotalMS);

    rdProgress = dDone / dTotal;
    rdSecondsLeft = ((dElapsed / rdProgress) - dElapsed) / 1000;

    return true;
}

bool MAC_FILE_ARRAY::GetContainsFile(const CString & strFilename)
{
    for (int z = 0; z < GetSize(); z++)
    {
        MAC_FILE * pInfo = &ElementAt(z);
        if (pInfo->m_strInputFilename.CompareNoCase(strFilename) == 0)
            return true;
    }

    return false;
}
