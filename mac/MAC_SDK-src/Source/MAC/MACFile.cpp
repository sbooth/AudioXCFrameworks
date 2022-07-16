#include "stdafx.h"
#include "MAC.h"
#include "MACFile.h"
#include "MACProgressHelper.h"
#include "FormatArray.h"

int g_nWorkingFilenameIndex = 1;

MAC_FILE::MAC_FILE()
{
    dInputFileBytes = 0;
    dOutputFileBytes = 0;
    pFormat = NULL;
    m_cFileType[0] = 0;

    bProcessing = FALSE;
}

MAC_FILE::~MAC_FILE()
{

}

BOOL MAC_FILE::PrepareForProcessing(CMACProcessFiles * pProcessFiles)
{
    bProcessing = TRUE;
    pMACProcessFiles = pProcessFiles;
    Mode = theApp.GetSettings()->GetMode();
    nStageProgress = 0;
    bDone = FALSE;
    nRetVal = ERROR_UNDEFINED;
    bStarted = FALSE;
    bNeedsUpdate = FALSE;
    dwStartTickCount = 0;
    dwEndTickCount = 0;
    strFormat = theApp.GetSettings()->GetFormat();
    nLevel = theApp.GetSettings()->GetLevel();
    nKillFlag = KILL_FLAG_CONTINUE;
    strOutputFilename.Empty();
    strWorkingFilename.Empty();
    nCurrentStage = 1;
    nTotalStages = 1;
    bEmptyExtension = FALSE;

    return TRUE;
}

void MAC_FILE::CalculateFilenames()
{
    // output filename
    CFilename fnOriginal(strInputFilename);
    CString strOutputExtension = GetOutputExtension();
    bEmptyExtension = (strOutputExtension.GetLength() <= 0);
    if (theApp.GetSettings()->m_nOutputLocationMode == OUTPUT_LOCATION_MODE_SAME_DIRECTORY)
    {
        // output to same directory
        strOutputFilename = fnOriginal.BuildFilename(NULL, NULL, NULL, strOutputExtension);
    }
    else if (theApp.GetSettings()->m_nOutputLocationMode == OUTPUT_LOCATION_MODE_SPECIFIED_DIRECTORY)
    {
        // specified directory
        CString strPath = theApp.GetSettings()->m_strOutputLocationDirectory;
        if (strPath.Right(1) != _T("\\")) strPath += _T("\\");

        // recreate directory structure
        if (theApp.GetSettings()->m_bOutputLocationRecreateDirectoryStructure)
        {
            CString strExtraPath;
            CString strInputPath = strInputFilename.Left(strInputFilename.ReverseFind('\\'));

            int nLevels = theApp.GetSettings()->m_nOutputLocationRecreateDirectoryStructureLevels;
            while (nLevels > 0)
            {
                int nSlash = strInputPath.ReverseFind('\\');
                if (nSlash <= 1)
                    break;

                if (strExtraPath.IsEmpty())
                    strExtraPath = strInputPath.Right(strInputPath.GetLength() - nSlash - 1);
                else
                    strExtraPath = strInputPath.Right(strInputPath.GetLength() - nSlash - 1) + _T("\\") + strExtraPath;
                
                strInputPath = strInputPath.Left(nSlash);

                nLevels = nLevels - 1;
            }

            strPath += strExtraPath;
            FixDirectory(strPath);
        }

        strOutputFilename = strPath + fnOriginal.GetName() + strOutputExtension;
    }
    
    // working filename (use g_nWorkingFilenameIndex so they'll never collide in multi-thread mode) (use parenthesis so it matches our GetUniqueFilename function)
    CFilename fnOutput(strOutputFilename);
    CString strName; strName.Format(_T("MAC Temp File (%d)"), g_nWorkingFilenameIndex++);
    strWorkingFilename = fnOutput.BuildFilename(NULL, NULL, strName, strOutputExtension);
    strWorkingFilename = GetUniqueFilename(strWorkingFilename);
}

double MAC_FILE::GetProgress()
{
    if (nTotalStages > 0)
        return (double(nStageProgress) / double(100000) / double(nTotalStages)) + (double(nCurrentStage - 1) / double(nTotalStages));
    else
        return 1;
}

CString MAC_FILE::GetOutputExtension()
{
    CString strExtension = theApp.GetFormatArray()->GetOutputExtension(Mode, strInputFilename, nLevel, pFormat);
    return strExtension;
}