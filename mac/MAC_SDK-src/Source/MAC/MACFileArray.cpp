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

BOOL MAC_FILE_ARRAY::PrepareForProcessing(CMACProcessFiles * pProcessFiles)
{
    for (int z = 0; z < GetSize(); z++)
    {
        ElementAt(z).PrepareForProcessing(pProcessFiles);
    }

    m_dwStartProcessingTickCount = GetTickCount();

    return TRUE;
}

double MAC_FILE_ARRAY::GetTotalInputBytes()
{
    double dTotalBytes = 0;

    for (int z = 0; z < GetSize(); z++)
    {
        dTotalBytes += ElementAt(z).dInputFileBytes;
    }

    return dTotalBytes;
}

double MAC_FILE_ARRAY::GetTotalOutputBytes()
{
    double dTotalBytes = 0;

    for (int z = 0; z < GetSize(); z++)
    {
        dTotalBytes += ElementAt(z).dOutputFileBytes;
    }

    return dTotalBytes;
}

BOOL MAC_FILE_ARRAY::GetProcessingInfo(BOOL bStopped, int & rnRunning, BOOL & rbAllDone)
{
    rnRunning = 0;
    rbAllDone = TRUE;
    
    for (int z = 0; z < GetSize(); z++)
    {
        MAC_FILE * pInfo = &ElementAt(z);

        // running
        if (pInfo->bStarted && (pInfo->bDone == FALSE))
            rnRunning++;

        // all done (if never started and we've stopped, don't reset rbAllDone to false)
        if (pInfo->bStarted || (bStopped == FALSE))
        {
            if (pInfo->bDone == FALSE)
                rbAllDone = FALSE;
        }
    }
    
    return TRUE;
}

BOOL MAC_FILE_ARRAY::GetProcessingProgress(double & rdProgress, double & rdSecondsLeft, int nPausedTotalMS)
{
    int nRunning = 0;
    int nDone = 0;

    double dTotal = 0;
    double dDone = 0;

    for (int z = 0; z < GetSize(); z++)
    {
        MAC_FILE * pInfo = &ElementAt(z);
        dTotal += pInfo->dInputFileBytes;

        if (pInfo->bDone == FALSE)
        {
            if (pInfo->bStarted)
            {
                nRunning++;
                
                double dProgress = pInfo->GetProgress();
                dDone += pInfo->dInputFileBytes * dProgress;
            }
        }
        else
        {
            nDone++;
            dDone += pInfo->dInputFileBytes;
        }
    }

    double dElapsed = double(GetTickCount() - m_dwStartProcessingTickCount) - double(nPausedTotalMS);
    
    rdProgress = dDone / dTotal;
    rdSecondsLeft = ((dElapsed / rdProgress) - dElapsed) / 1000;
    
    return TRUE;
}

BOOL MAC_FILE_ARRAY::GetContainsFile(const CString & strFilename)
{
    for (int z = 0; z < GetSize(); z++)
    {
        MAC_FILE * pInfo = &ElementAt(z);
        if (pInfo->strInputFilename.CompareNoCase(strFilename) == 0)
            return TRUE;
    }

    return FALSE;
}