#include "stdafx.h"
#include "MAC.h"
#include "APLHelper.h"

#include <mmsystem.h>
#include "All.h"
#include "APETag.h"
#include "CharacterHelper.h"
#include "IO.h"

using namespace APE;

CAPLHelper::CAPLHelper()
{
}

CAPLHelper::~CAPLHelper()
{
}

BOOL CAPLHelper::GenerateLinkFiles(const CString & strImage, const CString & strNamingTemplate)
{
    // get the directory
    CString strDirectory = strImage;
    strDirectory = strDirectory.Left(strDirectory.ReverseFind('\\') + 1);

    // get the CUE data
    HANDLE hCUE = CreateFile(strImage, GENERIC_READ, 0, APE_NULL, OPEN_EXISTING, 0, APE_NULL);
    DWORD dwCUEBytes = GetFileSize(hCUE, APE_NULL);

    CSmartPtr<char> spCUE(new char [static_cast<size_t>(dwCUEBytes) + 1], TRUE);
    memset(spCUE, 0, static_cast<size_t>(dwCUEBytes) + 1);

    unsigned long nBytesRead = 0;
    BOOL bReadResult = ReadFile(hCUE, spCUE.GetPtr(), dwCUEBytes, &nBytesRead, APE_NULL);
    CloseHandle(hCUE);
    if (bReadResult == FALSE)
    {
        return FALSE;
    }

    CString strCUE = spCUE.GetPtr();

    // parse the CUE header
    int nStart = strCUE.Find(_T("PERFORMER \""), 0);
    CString strArtist;
    if (nStart >= 0)
    {
        strArtist = strCUE.Right(strCUE.GetLength() - nStart - 11);
        strArtist = strArtist.SpanExcluding(_T("\""));
    }

    nStart = strCUE.Find(_T("TITLE \""), 0);
    CString strAlbum;
    if (nStart >= 0)
    {
        strAlbum = strCUE.Right(strCUE.GetLength() - nStart - 7);
        strAlbum = strAlbum.SpanExcluding(_T("\""));
    }

    nStart = strCUE.Find(_T("FILE \""), 0);
    CString strImageFile;
    if (nStart >= 0)
    {
        strImageFile = strCUE.Right(strCUE.GetLength() - nStart - 6);
        strImageFile = strImageFile.SpanExcluding(_T("\""));
        if (strImageFile.FindOneOf(_T("\\")) == -1)
        {
            strImageFile = strDirectory + strImageFile;
        }
    }

    // get the sample rate
    int64 nSampleRate = GetAPESampleRate(strImageFile);
    if (nSampleRate <= 0)
    {
        return FALSE;
    }

    // convert to a relative path
    strImageFile = strImageFile.Right(strImageFile.GetLength() - strImageFile.ReverseFind('\\') - 1);

    // go through each track in the CUE file and build an APL file from it
    int nTrack = 1;
    TCHAR cSearch[256];
    wsprintf(cSearch, _T("TRACK %2d AUDIO"), nTrack);
    if (cSearch[6] == ' ') cSearch[6] = '0';
    nStart = strCUE.Find(cSearch, 0);
    while (nStart >= 0)
    {
        int nStart2 = strCUE.Find(_T("TITLE \""), nStart);
        CString strTitle;
        if (nStart2 >= 0)
        {
            strTitle = strCUE.Right(strCUE.GetLength() - nStart2 - 7);
            strTitle = strTitle.SpanExcluding(_T("\""));
        }

        nStart2 = strCUE.Find(_T("PERFORMER \""), nStart);
        if (nStart2 >= 0)
        {
            strArtist = strCUE.Right(strCUE.GetLength() - nStart2 - 11);
            strArtist = strArtist.SpanExcluding(_T("\""));
        }

        nStart2 = strCUE.Find(_T("INDEX 01 "), nStart);
        CString strStartTime;
        if (nStart2 >= 0)
        {
            strStartTime = strCUE.Right(strCUE.GetLength() - nStart2 - 9);
            strStartTime = strStartTime.SpanExcluding(_T("\""));
        }

        nStart2 = strCUE.Find(_T("INDEX 01 "), nStart2 + 1);
        CString strFinishTime;
        if (nStart2 >= 0)
        {
            strFinishTime = strCUE.Right(strCUE.GetLength() - nStart2 - 9);
            strFinishTime = strFinishTime.SpanExcluding(_T("\""));
        }

        // figure the times out
        int nMinutes = _ttoi(strStartTime.SpanExcluding(_T(":")));
        strStartTime = strStartTime.Right(strStartTime.GetLength() - 3);
        int nSeconds = _ttoi(strStartTime.SpanExcluding(_T(":")));
        strStartTime = strStartTime.Right(strStartTime.GetLength() - 3);
        int nDecimal = _ttoi(strStartTime);

        double dStartSecond = (static_cast<double>(nMinutes) * static_cast<double>(60)) + static_cast<double>(nSeconds) + (static_cast<double>(nDecimal) / static_cast<double>(75));
        double dStartBlock = static_cast<double>(nSampleRate) * dStartSecond;

        CString strStartBlock; strStartBlock.Format(_T("%d"), static_cast<int>(dStartBlock + 0.5));

        CString strFinishBlock;
        if (strFinishTime.IsEmpty())
        {
            strFinishBlock = _T("-1");
        }
        else
        {
            nMinutes = _ttoi(strFinishTime.SpanExcluding(_T(":")));
            strFinishTime = strFinishTime.Right(strFinishTime.GetLength() - 3);
            nSeconds = _ttoi(strFinishTime.SpanExcluding(_T(":")));
            strFinishTime = strFinishTime.Right(strFinishTime.GetLength() - 3);
            nDecimal = _ttoi(strFinishTime);

            double dFinishSecond = (static_cast<double>(nMinutes) * static_cast<double>(60)) + static_cast<double>(nSeconds) + (static_cast<double>(nDecimal) / static_cast<double>(75));
            double dFinishBlock = static_cast<double>(nSampleRate) * dFinishSecond;

            strFinishBlock.Format(_T("%d"), static_cast<int>(dFinishBlock + 0.5));
        }

        CString strTrack; strTrack.Format(_T("%d"), nTrack);
        if (strTrack.GetLength() == 1) strTrack = _T("0") + strTrack;

        // build the filename
        CString strAPL = strNamingTemplate;
        strAPL.Replace(_T("ARTIST"), strArtist);
        strAPL.Replace(_T("TRACK#"), strTrack);
        strAPL.Replace(_T("TITLE"), strTitle);
        strAPL.Replace(_T("ALBUM"), strAlbum);

        if (strAPL.GetLength() >= 4)
        {
            if (strAPL.Right(4) == _T(".apl"))
                strAPL = strAPL.Left(strAPL.GetLength() - 4);
        }

        // fix the filename
        strAPL.Replace('?', '-');
        strAPL.Replace('\"', '-');
        strAPL.Replace('\\', '-');
        strAPL.Replace('/', '-');
        strAPL.Replace('<', '-');
        strAPL.Replace('>', '-');
        strAPL.Replace('*', '-');
        strAPL.Replace('|', '-');
        strAPL.Replace(':', '-');

        // add the directory
        strAPL = strDirectory + strAPL + _T(".apl");

        DeleteFileEx(strAPL);

        CSmartPtr<CIO> spFileIO(CreateCIO());
        if (spFileIO->Create(strAPL) == 0)
        {
            spFileIO->Seek(0, SeekFileEnd);

            CString strHeader;
            strHeader.Format(_T("[Monkey's Audio Image Link File]\r\n")
                _T("Image File=%s\r\n")
                _T("Start Block=%s\r\n")
                _T("Finish Block=%s\r\n"),
                strImageFile.GetString(), strStartBlock.GetString(), strFinishBlock.GetString());

            CSmartPtr<char> spHeader(CAPECharacterHelper::GetANSIFromUTF16(strHeader), TRUE);
            unsigned int nBytesWritten = 0;
            spFileIO->Write(spHeader.GetPtr(), static_cast<unsigned int>(strlen(spHeader)), &nBytesWritten);

            const char * pcTagHeader = "\r\n----- APE TAG (DO NOT TOUCH!!!) -----\r\n";
            nBytesWritten = 0;
            spFileIO->Write(pcTagHeader, static_cast<unsigned int>(strlen(pcTagHeader)), &nBytesWritten);

            CAPETag APETag(spFileIO, TRUE);
            APETag.SetFieldString(APE_TAG_FIELD_ARTIST, strArtist);
            APETag.SetFieldString(APE_TAG_FIELD_ALBUM, strAlbum);
            APETag.SetFieldString(APE_TAG_FIELD_TITLE, strTitle);
            APETag.SetFieldString(APE_TAG_FIELD_TRACK, strTrack);

            APETag.Save();
        }

        nTrack++;
        wsprintf(cSearch, _T("TRACK %2d AUDIO"), nTrack);
        if (cSearch[6] == ' ') cSearch[6] = '0';
        nStart = strCUE.Find(cSearch, 0);
    }

    return TRUE;
}

int64 CAPLHelper::GetAPESampleRate(const CString & strFilename)
{
    int64 nSampleRate = -1;

    CSmartPtr<IAPEDecompress> spAPEDecompress;
    try
    {
        int nFunctionRetVal = ERROR_SUCCESS;

        spAPEDecompress.Assign(CreateIAPEDecompress(strFilename, &nFunctionRetVal, true, false, false));
        if (spAPEDecompress == APE_NULL || nFunctionRetVal != ERROR_SUCCESS) throw(nFunctionRetVal);

        nSampleRate = spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE);
    }
    catch(...)
    {
        nSampleRate = -1;
    }

    return nSampleRate;
}
