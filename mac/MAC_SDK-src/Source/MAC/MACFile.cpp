#include "stdafx.h"
#include "MAC.h"
#include "MACFile.h"
#include "MACProgressHelper.h"
#include "FormatArray.h"

using namespace APE;

static int g_nWorkingFilenameIndex = 1;

MAC_FILE::MAC_FILE()
{
    dInputFileBytes = 0;
    dOutputFileBytes = 0;
    bProcessing = FALSE;
    pMACProcessFiles = NULL;
    Mode = MODE_COMPRESS;
    nStageProgress = 0;
    bDone = FALSE;
    nRetVal = FALSE;
    bStarted = FALSE;
    bNeedsUpdate = FALSE;
    dwStartTickCount = 0;
    dwEndTickCount = 0;
    nLevel = 0;
    nKillFlag = 0;
    nCurrentStage = 0;
    nTotalStages = 0;
    pFormat = NULL;
    bEmptyExtension = FALSE;
    bOverwriteInput = FALSE;
}

BOOL MAC_FILE::PrepareForProcessing(CMACProcessFiles * pProcessFiles)
{
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

    // get the size again if we overwrote our input file so doing conversion from APE to APE at different levels updates the size
    if (bOverwriteInput)
    {
        dInputFileBytes = GetFileBytes(strInputFilename);
        bOverwriteInput = FALSE;
    }
    dOutputFileBytes = 0;

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
    CString strName; strName.Format(_T("MAC Temp File (%d - %d)"), GetCurrentThreadId(), g_nWorkingFilenameIndex++);
    strWorkingFilename = fnOutput.BuildFilename(NULL, NULL, strName, strOutputExtension);
    strWorkingFilename = GetUniqueFilename(strWorkingFilename);

    // check if the output filename is the same as the input
    bOverwriteInput = (strOutputFilename.CompareNoCase(fnOriginal.GetFilename()) == 0);
}

double MAC_FILE::GetProgress() const
{
    if (nTotalStages > 0)
        return (static_cast<double>(nStageProgress) / static_cast<double>(100000) / static_cast<double>(nTotalStages)) + ((static_cast<double>(nCurrentStage) - static_cast<double>(1)) / static_cast<double>(nTotalStages));
    else
        return 1;
}

CString MAC_FILE::GetOutputExtension()
{
    CString strExtension = theApp.GetFormatArray()->GetOutputExtension(Mode, strInputFilename, nLevel, pFormat);
    return strExtension;
}
