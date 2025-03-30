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
        m_paryFiles->ElementAt(z).m_nKillFlag = KILL_FLAG_STOP;

    // wait until we're done processing
    bool bProcessing = true;
    while (bProcessing != false)
    {
        bProcessing = false;
        for (int z = 0; z < m_paryFiles->GetSize(); z++)
        {
            if (m_paryFiles->ElementAt(z).m_bProcessing)
                bProcessing = true;
        }

        if (bProcessing)
            Sleep(10);
    }
}

void CMACProcessFiles::Destroy()
{
    m_bStopped = false;
    m_bPaused = false;
    m_paryFiles = APE_NULL;
}

bool CMACProcessFiles::Process(MAC_FILE_ARRAY * paryFiles)
{
    Destroy();
    m_paryFiles = paryFiles;
    paryFiles->PrepareForProcessing(this);
    return true;
}

bool CMACProcessFiles::Pause(bool bPause)
{
    // store
    m_bPaused = bPause;

    // update file status
    for (int z = 0; z < m_paryFiles->GetSize(); z++)
    {
        m_paryFiles->ElementAt(z).m_nKillFlag = bPause ? KILL_FLAG_PAUSE : KILL_FLAG_CONTINUE;
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

    return true;
}

int CMACProcessFiles::GetPausedTotalMS() const
{
    int nPausedMS = static_cast<int>(m_nPausedTotalMS);
    if (m_bPaused)
        nPausedMS += static_cast<int>(GetTickCount64() - m_nPausedStartTickCount);
    return nPausedMS;
}

bool CMACProcessFiles::Stop(bool bImmediately)
{
    // immediately stop if specified
    if (bImmediately)
    {
        for (int z = 0; z < m_paryFiles->GetSize(); z++)
            m_paryFiles->ElementAt(z).m_nKillFlag = KILL_FLAG_STOP;
    }

    // flag the stop (it will happen eventually if we didn't do an immediate stop)
    m_bStopped = true;

    return true;
}

bool CMACProcessFiles::ProcessFile(int nIndex)
{
    // setup
    m_paryFiles->ElementAt(nIndex).m_bStarted = true;
    TICK_COUNT_READ(m_paryFiles->ElementAt(nIndex).m_dwStartTickCount);

    // create the thread
    std::thread Thread(ProcessFileThread, &m_paryFiles->ElementAt(nIndex));

    // spin off the thread
    Thread.detach();

    return true;
}

bool CMACProcessFiles::UpdateProgress(double dPercentageDone)
{
    TRACE(_T("%f done\n"), static_cast<double>(dPercentageDone));
    return true;
}

void CMACProcessFiles::ProcessFileThread(MAC_FILE * pInfo)
{
    // we're processing
    pInfo->m_bProcessing = true;

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
    pInfo->m_pFormat = theApp.GetFormatArray()->GetFormat(pInfo);
    pInfo->CalculateFilenames();

    int nRetVal = ERROR_UNDEFINED;
    bool bMakesOutput = (pInfo->m_Mode == MODE_COMPRESS) || (pInfo->m_Mode == MODE_DECOMPRESS) || (pInfo->m_Mode == MODE_CONVERT);
    bool bSkip = false;
    if (bMakesOutput)
    {
        if ((theApp.GetSettings()->m_nOutputExistsMode == OUTPUT_EXISTS_MODE_SKIP) && (FileExists(pInfo->m_strOutputFilename)))
            bSkip = true;
    }

    if (bSkip == false)
    {
        bool bVerifyWhenDone = theApp.GetSettings()->m_bProcessingAutoVerifyOnCreation && (CFilename(pInfo->m_strOutputFilename).GetExtension() == _T(".ape"));
        if (bVerifyWhenDone)
            pInfo->m_nTotalStages++;

        if (bMakesOutput)
            CreateDirectoryEx(CFilename(pInfo->m_strWorkingFilename).GetPath());

        if ((pInfo->m_Mode == MODE_COMPRESS) || (pInfo->m_Mode == MODE_DECOMPRESS) ||
            (pInfo->m_Mode == MODE_VERIFY))
        {
            nRetVal = theApp.GetFormatArray()->Process(pInfo);
        }
        else if (pInfo->m_Mode == MODE_CONVERT)
        {
            // first, see if we can do a "direct" conversion
            bool bDirectConversion = false;

            IFormat * pFormat = theApp.GetFormatArray()->GetFormat(pInfo->m_strFormat);
            if (pFormat)
            {
                CStringArrayEx aryExtensions;
                aryExtensions.InitFromList(pFormat->GetInputExtensions(pInfo->m_Mode), _T(";"));

                if (aryExtensions.Find(CFilename(pInfo->m_strInputFilename).GetExtension()) != -1)
                {
                    bDirectConversion = true;

                    nRetVal = theApp.GetFormatArray()->Process(pInfo);
                }
            }

            if (bDirectConversion == false)
            {
                IFormat * pStartFormat = pInfo->m_pFormat;
                CString strOutputFilenameOriginal = pInfo->m_strOutputFilename;
                bool bEmptyExtension = pInfo->m_bEmptyExtension;

                // two steps -- decompress, then compress
                CString strInputFilename = pInfo->m_strInputFilename;
                CString strOutputFilename = pInfo->m_strOutputFilename + _T(".wav");
                CString strWorkingFilenameStart = pInfo->m_strWorkingFilename;
                pInfo->m_nTotalStages++;

                CString strInputType = pInfo->m_strInputFilename.Right(pInfo->m_strInputFilename.GetLength() - pInfo->m_strInputFilename.ReverseFind('.'));
                pInfo->m_pFormat = theApp.GetFormatArray()->GetFormatFromInputType(strInputType);
                pInfo->m_Mode = MODE_DECOMPRESS;
                pInfo->CalculateFilenames();
                strOutputFilename = pInfo->m_strOutputFilename; // update

                // decompress
                nRetVal = theApp.GetFormatArray()->Process(pInfo);

                // compress if we succeed
                if (nRetVal == ERROR_SUCCESS)
                {
                    pInfo->m_pFormat = pStartFormat;
                    pInfo->m_strInputFilename = pInfo->m_strWorkingFilename; // use the working output from the last stage as the input

                    // update the working filename
                    CString strWorkingFilename = pInfo->m_strWorkingFilename;
                    CString strWorkingFilenameOriginal = pInfo->m_strWorkingFilename;
                    strWorkingFilename = strWorkingFilename.Left(strWorkingFilename.ReverseFind('.') + 1) + _T("dat");
                    pInfo->m_strWorkingFilename = strWorkingFilename;

                    // enter compress mode
                    pInfo->m_Mode = MODE_COMPRESS;
                    pInfo->m_strOutputFilename = strOutputFilenameOriginal;
                    pInfo->m_nCurrentStage++;
                    pInfo->m_nStageProgress = 0;

                    // add the output extension if we didn't add it earlier
                    if (bEmptyExtension)
                    {
                        CString strOutputExtension = pInfo->m_pFormat->GetOutputExtension(MODE_COMPRESS, pInfo->m_strInputFilename, 0);
                        pInfo->m_strOutputFilename += strOutputExtension;
                    }

                    // compress
                    nRetVal = theApp.GetFormatArray()->Process(pInfo);

                    // delete working file
                    DeleteFileEx(strWorkingFilenameOriginal);
                    pInfo->m_strInputFilename = strInputFilename;
                }
            }
        }
        else if (pInfo->m_Mode == MODE_MAKE_APL)
        {
            CAPLHelper APLHelper;
            bool bRetVal = APLHelper.GenerateLinkFiles(pInfo->m_strInputFilename, theApp.GetSettings()->m_strAPLFilenameTemplate);
            nRetVal = bRetVal ? ERROR_SUCCESS : ERROR_UNDEFINED;
        }

        if (bMakesOutput && (nRetVal == ERROR_SUCCESS))
        {
            // analyze if the input and output are the same
            bool bInputOutputSame = (pInfo->m_strInputFilename.CompareNoCase(pInfo->m_strOutputFilename) == 0);

            // we loop a few times and try to name the output file over and over since in race conditions encoding the same file
            // we can get a filename, then it can be gone by the time we go to use it
            // to reproduce, encode two files with similar names with a shared output directory (example: C:\Temporary\AIFF From Porcus\demo-snd.aiff / C:\Temporary\AIFF From Porcus\demo-snd (1).aiff)
            bool bMakeUnique = (bInputOutputSame == false) && (theApp.GetSettings()->m_nOutputExistsMode == OUTPUT_EXISTS_MODE_RENAME);
            for (int nRetry = 0; nRetry < (bMakeUnique ? 3 : 1); nRetry++)
            {
                // rename output if necessary
                if (bMakeUnique)
                    pInfo->m_strOutputFilename = GetUniqueFilename(pInfo->m_strOutputFilename);

                // move the working file to the final location
                if (MoveFile(pInfo->m_strWorkingFilename, pInfo->m_strOutputFilename, true) != false)
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
                CopyFileTime(pInfo->m_strInputFilename, pInfo->m_strOutputFilename);

            if ((nRetVal == ERROR_SUCCESS) && (pInfo->GetOutputExtension() == _T(".ape")) && (pInfo->m_strInputFilename.Right(3).CompareNoCase(_T(".wv")) == 0))
            {
                // copy tags from WavPack to APE files
                CSmartPtr<APE::IAPETag> spAPETag(new CAPETag(pInfo->m_strInputFilename, true));
                if (spAPETag->GetHasAPETag())
                {
                    CSmartPtr<APE::IAPETag> spAPETagNew(new CAPETag(pInfo->m_strOutputFilename, true));
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
            else if ((nRetVal == ERROR_SUCCESS) && (pInfo->GetOutputExtension() == _T(".ape")) && (pInfo->m_strInputFilename.Right(5).CompareNoCase(_T(".flac")) == 0))
            {
                // copy tags from FLAC to APE files
                CSmartPtr<CFLACTag> spFLACTag(new CFLACTag(pInfo->m_strInputFilename));
                if (spFLACTag->m_mapFields.GetCount() > 0)
                {
                    CSmartPtr<APE::IAPETag> spAPETagNew(new CAPETag(pInfo->m_strOutputFilename, true));

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
                        else if (strKey.CompareNoCase(_T("LYRICS")) == 0)
                            spAPETagNew->SetFieldString(APE_TAG_FIELD_LYRICS, strValue);
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
                        int nBufferBytes = static_cast<int>(spFLACTag->m_nPictureBytes) + spFLACTag->m_strImageExtension.GetLength() + 1;
                        CSmartPtr<char> spBuffer(new char [static_cast<size_t>(nBufferBytes)], true);
                        CSmartPtr<char> spExtensionANSI(CAPECharacterHelper::GetANSIFromUTF16(spFLACTag->m_strImageExtension), true);
                        memcpy(spBuffer, spExtensionANSI, static_cast<size_t>(spFLACTag->m_strImageExtension.GetLength())); // extension
                        spBuffer[spFLACTag->m_strImageExtension.GetLength()] = 0; // NULL after the extension
                        memcpy(&spBuffer[spFLACTag->m_strImageExtension.GetLength() + 1], spFLACTag->m_spPicture, spFLACTag->m_nPictureBytes); // image data
                        spAPETagNew->SetFieldBinary(APE_TAG_FIELD_COVER_ART_FRONT, spBuffer, static_cast<intn>(spFLACTag->m_nPictureBytes) + 4, TAG_FIELD_FLAG_DATA_TYPE_BINARY);
                    }

                    spAPETagNew->Save(false);
                }
            }

            // verify (if necessary)
            if (bVerifyWhenDone && (nRetVal == ERROR_SUCCESS))
            {
                CString strInputFilename = pInfo->m_strInputFilename;
                CString strOutputFilename = pInfo->m_strOutputFilename;

                pInfo->m_Mode = MODE_VERIFY;
                pInfo->m_strInputFilename = pInfo->m_strOutputFilename;
                pInfo->m_nCurrentStage++;
                pInfo->m_nStageProgress = 0;
                pInfo->CalculateFilenames();

                nRetVal = theApp.GetFormatArray()->Process(pInfo);

                pInfo->m_strInputFilename = strInputFilename;
                pInfo->m_strOutputFilename = strOutputFilename;
            }

            // delete / recycle input file if specified
            if ((nRetVal == ERROR_SUCCESS) && (bInputOutputSame == false))
            {
                if (theApp.GetSettings()->m_nOutputDeleteAfterSuccessMode == OUTPUT_DELETE_AFTER_SUCCESS_MODE_RECYCLE_SOURCE)
                {
                    RecycleFile(pInfo->m_strInputFilename);
                }
                else if (theApp.GetSettings()->m_nOutputDeleteAfterSuccessMode == OUTPUT_DELETE_AFTER_SUCCESS_MODE_DELETE_SOURCE)
                {
                    DeleteFileEx(pInfo->m_strInputFilename);
                }
            }

            // update the output bytes
            pInfo->m_dOutputFileBytes = GetFileBytes(pInfo->m_strOutputFilename);
        }

        if (nRetVal != ERROR_SUCCESS)
        {
            // cleanup the working file on failure
            DeleteFileEx(pInfo->m_strWorkingFilename);
        }
    }
    else
    {
        nRetVal = ERROR_SKIPPED;
    }

    TICK_COUNT_READ(pInfo->m_dwEndTickCount);
    pInfo->m_nRetVal = nRetVal;
    pInfo->m_bDone = true;
    pInfo->m_bNeedsUpdate = true;

    // update the status to be user cancelled if we stopped
    if ((pInfo->m_nRetVal == ERROR_UNDEFINED) && (pInfo->m_nKillFlag == KILL_FLAG_STOP))
        pInfo->m_nRetVal = ERROR_USER_STOPPED_PROCESSING;

    // reset thread state
    SetThreadExecutionState(ES_CONTINUOUS);

    // we're done processing
    pInfo->m_bProcessing = false;
}
