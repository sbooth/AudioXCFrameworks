#include "stdafx.h"
#include "MAC.h"
#include "MACFile.h"
#include "MACProgressHelper.h"
#include "FormatArray.h"
#include "MACProcessFiles.h"

using namespace APE;

static int g_nWorkingFilenameIndex = 1;

MAC_FILE::MAC_FILE()
{
    m_dInputFileBytes = 0;
    m_dOutputFileBytes = 0;
    m_bProcessing = false;
    m_pMACProcessFiles = APE_NULL;
    m_Mode = MODE_COMPRESS;
    m_nStageProgress = 0;
    m_bDone = false;
    m_nRetVal = ERROR_UNDEFINED;
    m_bStarted = false;
    m_bNeedsUpdate = false;
    m_dwStartTickCount = 0;
    m_dwEndTickCount = 0;
    m_nLevel = 0;
    m_nKillFlag = 0;
    m_nCurrentStage = 0;
    m_nTotalStages = 0;
    m_pFormat = APE_NULL;
    m_bEmptyExtension = false;
    m_bOverwriteInput = false;
    m_nThreads = 1;
}

bool MAC_FILE::PrepareForProcessing(CMACProcessFiles * pProcessFiles)
{
    m_pMACProcessFiles = pProcessFiles;
    m_Mode = theApp.GetSettings()->GetMode();
    m_nStageProgress = 0;
    m_bDone = false;
    m_nRetVal = ERROR_UNDEFINED;
    m_bStarted = false;
    m_bNeedsUpdate = false;
    m_dwStartTickCount = 0;
    m_dwEndTickCount = 0;
    m_strFormat = theApp.GetSettings()->GetFormat();
    m_nLevel = theApp.GetSettings()->GetLevel();
    m_nKillFlag = KILL_FLAG_CONTINUE;
    m_strOutputFilename.Empty();
    m_strWorkingFilename.Empty();
    m_nCurrentStage = 1;
    m_nTotalStages = 1;
    m_bEmptyExtension = false;

    // calculate the number of processing threads
    m_nThreads = 1;
    if (pProcessFiles->GetSize() < theApp.GetSettings()->m_nProcessingSimultaneousFiles)
    {
        int nSimultaneous = theApp.GetSettings()->m_nProcessingSimultaneousFiles;
        int nSize = pProcessFiles->GetSize();
        m_nThreads = (nSimultaneous + (nSize - 1)) / nSize; // round up
    }

    // get the size again if we overwrote our input file so doing conversion from APE to APE at different levels updates the size
    if (m_bOverwriteInput)
    {
        m_dInputFileBytes = GetFileBytes(m_strInputFilename);
        m_bOverwriteInput = false;
    }
    m_dOutputFileBytes = 0;

    return true;
}

void MAC_FILE::CalculateFilenames()
{
    // output filename
    CFilename fnOriginal(m_strInputFilename);
    CString strOutputExtension = GetOutputExtension();
    m_bEmptyExtension = (strOutputExtension.GetLength() <= 0);
    if (theApp.GetSettings()->m_nOutputLocationMode == OUTPUT_LOCATION_MODE_SAME_DIRECTORY)
    {
        // output to same directory
        m_strOutputFilename = fnOriginal.BuildFilename(APE_NULL, APE_NULL, APE_NULL, strOutputExtension);
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
            CString strInputPath = m_strInputFilename.Left(m_strInputFilename.ReverseFind('\\'));

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

        m_strOutputFilename = strPath + fnOriginal.GetName() + strOutputExtension;
    }

    // working filename (use g_nWorkingFilenameIndex so they'll never collide in multi-thread mode) (use parenthesis so it matches our GetUniqueFilename function)
    CFilename fnOutput(m_strOutputFilename);
    CString strName; strName.Format(_T("MAC Temp File (%d - %d)"), GetCurrentThreadId(), g_nWorkingFilenameIndex++);
    m_strWorkingFilename = fnOutput.BuildFilename(APE_NULL, APE_NULL, strName, strOutputExtension);
    m_strWorkingFilename = GetUniqueFilename(m_strWorkingFilename);

    // check if the output filename is the same as the input
    m_bOverwriteInput = (m_strOutputFilename.CompareNoCase(fnOriginal.GetFilename()) == 0);
}

double MAC_FILE::GetProgress() const
{
    if (m_nTotalStages > 0)
        return (static_cast<double>(m_nStageProgress) / static_cast<double>(100000) / static_cast<double>(m_nTotalStages)) + ((static_cast<double>(m_nCurrentStage) - static_cast<double>(1)) / static_cast<double>(m_nTotalStages));
    else
        return 1;
}

CString MAC_FILE::GetOutputExtension() const
{
    CString strExtension = theApp.GetFormatArray()->GetOutputExtension(m_Mode, m_strInputFilename, m_nLevel, m_pFormat);
    return strExtension;
}
