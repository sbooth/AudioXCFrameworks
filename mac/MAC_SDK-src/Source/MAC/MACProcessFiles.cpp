#include "stdafx.h"
#include "MAC.h"
#include "MACProcessFiles.h"
#include "MACProgressHelper.h"
#include "APLHelper.h"
#include "FormatArray.h"
#include "APETag.h"
#include "FLACTag.h"
#include "IO.h"

using namespace APE;

CMACProcessFiles::CMACProcessFiles()
{
    m_paryFiles = APE_NULL;
    m_nPausedStartTickCount = 0;
    m_nPausedTotalMS = 0;
    Destroy();
}

CMACProcessFiles::~CMACProcessFiles()
{
    // stop
    for (int z = 0; z < m_paryFiles->GetSize(); z++)
        m_paryFiles->ElementAt(z).nKillFlag = KILL_FLAG_STOP;

    // wait until we're done processing
    bool bProcessing = true;
    while (bProcessing != false)
    {
        bProcessing = false;
        for (int z = 0; z < m_paryFiles->GetSize(); z++)
        {
            if (m_paryFiles->ElementAt(z).bProcessing)
                bProcessing = true;
        }

        if (bProcessing)
            Sleep(10);
    }
}

void CMACProcessFiles::Destroy()
{
    m_bStopped = FALSE;
    m_bPaused = FALSE;
    m_paryFiles = APE_NULL;
}

BOOL CMACProcessFiles::Process(MAC_FILE_ARRAY * paryFiles)
{
    Destroy();
    m_paryFiles = paryFiles;
    paryFiles->PrepareForProcessing(this);
    return TRUE;
}

BOOL CMACProcessFiles::Pause(BOOL bPause)
{
    // store
    m_bPaused = bPause;

    // update file status
    for (int z = 0; z < m_paryFiles->GetSize(); z++)
    {
        m_paryFiles->ElementAt(z).nKillFlag = bPause ? KILL_FLAG_PAUSE : KILL_FLAG_CONTINUE;
    }

    // update the pause timer
    if (m_bPaused)
    {
        m_nPausedStartTickCount = GetTickCount64();
    }
    else
    {
        m_nPausedTotalMS += GetTickCount64() - m_nPausedStartTickCount;
        m_nPausedStartTickCount = 0;
    }

    return TRUE;
}

int CMACProcessFiles::GetPausedTotalMS()
{
    int nPausedMS = static_cast<int>(m_nPausedTotalMS);
    if (m_bPaused)
        nPausedMS += static_cast<int>(GetTickCount64() - m_nPausedStartTickCount);
    return nPausedMS;
}

BOOL CMACProcessFiles::Stop(BOOL bImmediately)
{
    // immediately stop if specified
    if (bImmediately)
    {
        for (int z = 0; z < m_paryFiles->GetSize(); z++)
            m_paryFiles->ElementAt(z).nKillFlag = KILL_FLAG_STOP;
    }

    // flag the stop (it will happen eventually if we didn't do an immediate stop)
    m_bStopped = TRUE;

    return TRUE;
}


BOOL CMACProcessFiles::ProcessFile(int nIndex)
{
    // setup
    m_paryFiles->ElementAt(nIndex).bStarted = TRUE;
    TICK_COUNT_READ(m_paryFiles->ElementAt(nIndex).dwStartTickCount);

    // create the thread
    std::thread Thread(ProcessFileThread, &m_paryFiles->ElementAt(nIndex));

    // spin off the thread
    Thread.detach();

    return TRUE;
}

BOOL CMACProcessFiles::UpdateProgress(double dPercentageDone)
{
    TRACE(_T("%f done\n"), static_cast<double>(dPercentageDone));
    return TRUE;
}

void CMACProcessFiles::ProcessFileThread(MAC_FILE * pInfo)
{
    // we're processing
    pInfo->bProcessing = TRUE;

    // thread priority
    if (theApp.GetSettings()->m_nProcessingPriorityMode == PROCESSING_PRIORITY_MODE_IDLE)
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
    }
    else if (theApp.GetSettings()->m_nProcessingPriorityMode == PROCESSING_PRIORITY_MODE_LOW)
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    }
    else if (theApp.GetSettings()->m_nProcessingPriorityMode == PROCESSING_PRIORITY_MODE_HIGH)
    {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    }
    else if (theApp.GetSettings()->m_nProcessingPriorityMode == PROCESSING_PRIORITY_MODE_NORMAL)
    {
        // we should already be THREAD_PRIORITY_NORMAL since we were started and never set
        ASSERT(GetThreadPriority(GetCurrentThread()) == THREAD_PRIORITY_NORMAL);
    }

    // process
    pInfo->pFormat = theApp.GetFormatArray()->GetFormat(pInfo);
    pInfo->CalculateFilenames();

    int nRetVal = ERROR_UNDEFINED;
    BOOL bMakesOutput = (pInfo->Mode == MODE_COMPRESS) || (pInfo->Mode == MODE_DECOMPRESS) || (pInfo->Mode == MODE_CONVERT);
    BOOL bSkip = FALSE;
    if (bMakesOutput)
    {
        if ((theApp.GetSettings()->m_nOutputExistsMode == OUTPUT_EXISTS_MODE_SKIP) && (FileExists(pInfo->strOutputFilename)))
            bSkip = TRUE;
    }

    if (bSkip == FALSE)
    {
        BOOL bVerifyWhenDone = theApp.GetSettings()->m_bProcessingAutoVerifyOnCreation && (CFilename(pInfo->strOutputFilename).GetExtension() == _T(".ape"));
        if (bVerifyWhenDone)
            pInfo->nTotalStages++;

        if (bMakesOutput)
            CreateDirectoryEx(CFilename(pInfo->strWorkingFilename).GetPath());

        if ((pInfo->Mode == MODE_COMPRESS) || (pInfo->Mode == MODE_DECOMPRESS) ||
            (pInfo->Mode == MODE_VERIFY))
        {
            nRetVal = theApp.GetFormatArray()->Process(pInfo);
        }
        else if (pInfo->Mode == MODE_CONVERT)
        {
            // first, see if we can do a "direct" conversion
            BOOL bDirectConversion = FALSE;

            IFormat * pFormat = theApp.GetFormatArray()->GetFormat(pInfo->strFormat);
            if (pFormat)
            {
                CStringArrayEx aryExtensions;
                aryExtensions.InitFromList(pFormat->GetInputExtensions(pInfo->Mode), _T(";"));

                if (aryExtensions.Find(CFilename(pInfo->strInputFilename).GetExtension()) != -1)
                {
                    bDirectConversion = TRUE;

                    nRetVal = theApp.GetFormatArray()->Process(pInfo);
                }
            }

            if (bDirectConversion == FALSE)
            {
                IFormat * pStartFormat = pInfo->pFormat;
                CString strOutputFilenameOriginal = pInfo->strOutputFilename;
                BOOL bEmptyExtension = pInfo->bEmptyExtension;

                // two steps -- decompress, then compress
                CString strInputFilename = pInfo->strInputFilename;
                CString strOutputFilename = pInfo->strOutputFilename + _T(".wav");
                CString strWorkingFilenameStart = pInfo->strWorkingFilename;
                pInfo->nTotalStages++;

                CString strInputType = pInfo->strInputFilename.Right(pInfo->strInputFilename.GetLength() - pInfo->strInputFilename.ReverseFind('.'));
                pInfo->pFormat = theApp.GetFormatArray()->GetFormatFromInputType(strInputType);
                pInfo->Mode = MODE_DECOMPRESS;
                pInfo->CalculateFilenames();
                strOutputFilename = pInfo->strOutputFilename; // update

                // decompress
                nRetVal = theApp.GetFormatArray()->Process(pInfo);

                // compress if we succeed
                if (nRetVal == ERROR_SUCCESS)
                {
                    pInfo->pFormat = pStartFormat;
                    pInfo->strInputFilename = pInfo->strWorkingFilename; // use the working output from the last stage as the input

                    // update the working filename
                    CString strWorkingFilename = pInfo->strWorkingFilename;
                    CString strWorkingFilenameOriginal = pInfo->strWorkingFilename;
                    strWorkingFilename = strWorkingFilename.Left(strWorkingFilename.ReverseFind('.') + 1) + _T("dat");
                    pInfo->strWorkingFilename = strWorkingFilename;

                    // enter compress mode
                    pInfo->Mode = MODE_COMPRESS;
                    pInfo->strOutputFilename = strOutputFilenameOriginal;
                    pInfo->nCurrentStage++;
                    pInfo->nStageProgress = 0;

                    // add the output extension if we didn't add it earlier
                    if (bEmptyExtension)
                    {
                        CString strOutputExtension = pInfo->pFormat->GetOutputExtension(MODE_COMPRESS, pInfo->strInputFilename, 0);
                        pInfo->strOutputFilename += strOutputExtension;
                    }

                    // compress
                    nRetVal = theApp.GetFormatArray()->Process(pInfo);

                    // delete working file
                    DeleteFileEx(strWorkingFilenameOriginal);
                    pInfo->strInputFilename = strInputFilename;
                }
            }
        }
        else if (pInfo->Mode == MODE_MAKE_APL)
        {
            CAPLHelper APLHelper;
            BOOL bRetVal = APLHelper.GenerateLinkFiles(pInfo->strInputFilename, theApp.GetSettings()->m_strAPLFilenameTemplate);
            nRetVal = bRetVal ? ERROR_SUCCESS : ERROR_UNDEFINED;
        }

        if (bMakesOutput && (nRetVal == ERROR_SUCCESS))
        {
            // analyze if the input and output are the same
            BOOL bInputOutputSame = (pInfo->strInputFilename.CompareNoCase(pInfo->strOutputFilename) == 0);

            // we loop a few times and try to name the output file over and over since in race conditions encoding the same file
            // we can get a filename, then it can be gone by the time we go to use it
            // to reproduce, encode two files with similar names with a shared output directory (example: C:\Temporary\AIFF From Porcus\demo-snd.aiff / C:\Temporary\AIFF From Porcus\demo-snd (1).aiff)
            bool bMakeUnique = (bInputOutputSame == FALSE) && (theApp.GetSettings()->m_nOutputExistsMode == OUTPUT_EXISTS_MODE_RENAME);
            for (int nRetry = 0; nRetry < (bMakeUnique ? 3 : 1); nRetry++)
            {
                // rename output if necessary
                if (bMakeUnique)
                    pInfo->strOutputFilename = GetUniqueFilename(pInfo->strOutputFilename);

                // move the working file to the final location
                if (MoveFile(pInfo->strWorkingFilename, pInfo->strOutputFilename, TRUE) != FALSE)
                {
                    nRetVal = ERROR_SUCCESS;
                    break;
                }
                else
                {
                    nRetVal = ERROR_INVALID_OUTPUT_FILE;
                }
            }

            // mirror time stamp if specified
            if ((nRetVal == ERROR_SUCCESS) && theApp.GetSettings()->m_bOutputMirrorTimeStamp)
                CopyFileTime(pInfo->strInputFilename, pInfo->strOutputFilename);

            if ((nRetVal == ERROR_SUCCESS) && (pInfo->GetOutputExtension() == _T(".ape")) && (pInfo->strInputFilename.Right(3).CompareNoCase(_T(".wv")) == 0))
            {
                // copy tags from WavPack to APE files
                CSmartPtr<APE::IAPETag> spAPETag(new CAPETag(pInfo->strInputFilename, true));
                if (spAPETag->GetHasAPETag())
                {
                    CSmartPtr<APE::IAPETag> spAPETagNew(new CAPETag(pInfo->strOutputFilename, true));
                    for (int z = 0; true; z++)
                    {
                        CAPETagField * pField = spAPETag->GetTagField(z);
                        if (pField == APE_NULL)
                            break;
                        spAPETagNew->SetFieldBinary(pField->GetFieldName(), pField->GetFieldValue(), pField->GetFieldValueSize(), pField->GetFieldFlags());
                    }
                    spAPETagNew->Save(false);
                }
            }
            else if ((nRetVal == ERROR_SUCCESS) && (pInfo->GetOutputExtension() == _T(".ape")) && (pInfo->strInputFilename.Right(5).CompareNoCase(_T(".flac")) == 0))
            {
                // copy tags from FLAC to APE files
                CSmartPtr<CFLACTag> spFLACTag(new CFLACTag(pInfo->strInputFilename));
                if (spFLACTag->m_mapFields.GetCount() > 0)
                {
                    CSmartPtr<APE::IAPETag> spAPETagNew(new CAPETag(pInfo->strOutputFilename, true));

                    POSITION Pos = spFLACTag->m_mapFields.GetStartPosition();
                    while (Pos)
                    {
                        CString strKey, strValue;
                        spFLACTag->m_mapFields.GetNextAssoc(Pos, strKey, strValue);

                        if (strKey.CompareNoCase(_T("TITLE")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_TITLE, strValue);
                        else if (strKey.CompareNoCase(_T("ARTIST")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_ARTIST, strValue);
                        else if (strKey.CompareNoCase(_T("ALBUM")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_ALBUM, strValue);
                        else if (strKey.CompareNoCase(_T("ALBUM ARTIST")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_ALBUM_ARTIST, strValue);
                        else if (strKey.CompareNoCase(_T("TRACKNUMBER")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_TRACK, strValue);
                        else if (strKey.CompareNoCase(_T("DISCNUMBER")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_DISC, strValue);
                        else if (strKey.CompareNoCase(_T("COMMENT")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_COMMENT, strValue);
                        else if (strKey.CompareNoCase(_T("GENRE")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_GENRE, strValue);
                        else if (strKey.CompareNoCase(_T("COMPOSER")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_COMPOSER, strValue);
                        else if (strKey.CompareNoCase(_T("CONDUCTOR")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_CONDUCTOR, strValue);
                        else if (strKey.CompareNoCase(_T("ORCHESTRA")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_ORCHESTRA, strValue);
                        else if (strKey.CompareNoCase(_T("RATING")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_RATING, strValue);
                        else if (strKey.CompareNoCase(_T("DATE")) == 0)
                        {
                            // year only
                            if ((strValue.GetLength() == 4) && (_ttoi(strValue) >= 1500) && (_ttoi(strValue) <= 2500))
                            {
                                // date is simply a year
                                spAPETagNew->SetFieldString(APE_TAG_FIELD_YEAR, strValue);
                            }
                            else
                            {
                                // date is formatted more advanced than just a year so try to parse it
                                COleDateTime dtDate;
                                if (dtDate.ParseDateTime(strValue) && (dtDate.GetYear() > 0))
                                {
                                    strValue.Format(_T("%d"), dtDate.GetYear());
                                    spAPETagNew->SetFieldString(APE_TAG_FIELD_YEAR, strValue);
                                }
                            }
                        }
                        else
                        {
                            // look at strKey here
                            CString strMissing = strKey;
                            OutputDebugString(strMissing);
                        }
                    }

                    if (spFLACTag->m_spPicture != NULL)
                    {
                        CSmartPtr<char> spBuffer(new char [spFLACTag->m_nPictureBytes + 10], true);
                        memcpy(spBuffer, "Cover.jpg\0", 10);
                        memcpy(&spBuffer[10], spFLACTag->m_spPicture, spFLACTag->m_nPictureBytes);
                        spAPETagNew->SetFieldBinary(APE_TAG_FIELD_COVER_ART_FRONT, spBuffer, spFLACTag->m_nPictureBytes + 10, TAG_FIELD_FLAG_DATA_TYPE_BINARY);
                    }

                    spAPETagNew->Save(false);
                }
            }

            // verify (if necessary)
            if (bVerifyWhenDone && (nRetVal == ERROR_SUCCESS))
            {
                CString strInputFilename = pInfo->strInputFilename;
                CString strOutputFilename = pInfo->strOutputFilename;

                pInfo->Mode = MODE_VERIFY;
                pInfo->strInputFilename = pInfo->strOutputFilename;
                pInfo->nCurrentStage++;
                pInfo->nStageProgress = 0;
                pInfo->CalculateFilenames();

                nRetVal = theApp.GetFormatArray()->Process(pInfo);

                pInfo->strInputFilename = strInputFilename;
                pInfo->strOutputFilename = strOutputFilename;
            }

            // delete / recycle input file if specified
            if ((nRetVal == ERROR_SUCCESS) && (bInputOutputSame == FALSE))
            {
                if (theApp.GetSettings()->m_nOutputDeleteAfterSuccessMode == OUTPUT_DELETE_AFTER_SUCCESS_MODE_RECYCLE_SOURCE)
                {
                    RecycleFile(pInfo->strInputFilename);
                }
                else if (theApp.GetSettings()->m_nOutputDeleteAfterSuccessMode == OUTPUT_DELETE_AFTER_SUCCESS_MODE_DELETE_SOURCE)
                {
                    DeleteFileEx(pInfo->strInputFilename);
                }
            }

            // update the output bytes
            pInfo->dOutputFileBytes = GetFileBytes(pInfo->strOutputFilename);
        }

        if (nRetVal != ERROR_SUCCESS)
        {
            // cleanup the working file on failure
            DeleteFileEx(pInfo->strWorkingFilename);
        }
    }
    else
    {
        nRetVal = ERROR_SKIPPED;
    }

    TICK_COUNT_READ(pInfo->dwEndTickCount);
    pInfo->nRetVal = nRetVal;
    pInfo->bDone = TRUE;
    pInfo->bNeedsUpdate = TRUE;

    // update the status to be user cancelled if we stopped
    if ((pInfo->nRetVal == ERROR_UNDEFINED) && (pInfo->nKillFlag == KILL_FLAG_STOP))
        pInfo->nRetVal = ERROR_USER_STOPPED_PROCESSING;

    // reset thread state
    SetThreadExecutionState(ES_CONTINUOUS);

    // we're done processing
    pInfo->bProcessing = FALSE;
}
