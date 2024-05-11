#include "stdafx.h"
#include "MAC.h"
#include "APEInfoFormatDlg.h"
#include "APETag.h"
#include "WAVInputSource.h"
#include "MACDlg.h"
#include "APEInfo.h"

using namespace APE;

IMPLEMENT_DYNAMIC(CAPEInfoFormatDlg, CDialog)
CAPEInfoFormatDlg::CAPEInfoFormatDlg(CMACDlg * pMACDlg, CWnd * pParent)
    : CDialog(CAPEInfoFormatDlg::IDD, pParent)
{
    m_pMACDlg = pMACDlg;
}

CAPEInfoFormatDlg::~CAPEInfoFormatDlg()
{
}

void CAPEInfoFormatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FORMAT, m_ctrlFormat);
}

BOOL CAPEInfoFormatDlg::OnInitDialog()
{
    // set the font to all the controls
    SetFont(&m_pMACDlg->GetFont());
    SendMessageToDescendants(WM_SETFONT, reinterpret_cast<WPARAM>(m_pMACDlg->GetFont().GetSafeHandle()), MAKELPARAM(FALSE, 0), TRUE);

    // parent
    return CDialog::OnInitDialog();
}

BEGIN_MESSAGE_MAP(CAPEInfoFormatDlg, CDialog)
END_MESSAGE_MAP()

void CAPEInfoFormatDlg::Layout()
{
    CRect rectWindow;
    GetClientRect(&rectWindow);
    const int nBorder = 0;
    m_ctrlFormat.SetWindowPos(APE_NULL, theApp.GetSize(nBorder, 0).cx, theApp.GetSize(nBorder, 0).cx, rectWindow.Width() - theApp.GetSize(nBorder * 2, 0).cx, rectWindow.Height() - theApp.GetSize(nBorder * 2, 0).cx, SWP_NOZORDER);
}

BOOL CAPEInfoFormatDlg::SetFiles(CStringArray & aryFiles)
{
    m_aryFiles.Copy(aryFiles);

    CString strSummary;

    if (m_aryFiles.GetSize() <= 100)
    {
        for (int z = 0; z < m_aryFiles.GetSize(); z++)
        {
            strSummary += m_aryFiles[z] + _T("\r\n");
            strSummary += GetSummary(m_aryFiles[z]) + _T("\r\n\r\n");
        }
    }
    else
    {
        strSummary = _T("Too many files selected.  Please select less tracks.");
    }
    strSummary.TrimRight();

    m_ctrlFormat.SetWindowText(strSummary);

    return TRUE;
}

CString CAPEInfoFormatDlg::GetSummary(const CString & strFilename)
{
    CString strSummary;

    CSmartPtr<IAPEDecompress> spAPEDecompress; int nFunctionRetVal = ERROR_UNDEFINED;
    spAPEDecompress.Assign(CreateIAPEDecompress(strFilename, &nFunctionRetVal, true, true, false));
    if ((spAPEDecompress != APE_NULL) && (nFunctionRetVal == ERROR_SUCCESS))
    {
        CString strLine;

        str_utfn cCompressionName[256]; APE_CLEAR(cCompressionName);
        GetAPECompressionLevelName(static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL)), cCompressionName, 256, false);

        strLine.Format(_T("Monkey's Audio %.2f (%s)"),
            static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)) / static_cast<double>(1000),
            static_cast<LPCTSTR>(cCompressionName));
        strSummary += strLine + _T("\r\n");

        // format
        strLine.Format(_T("Format: %.1f khz, %d bit, %d ch"),
            static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)) / static_cast<double>(1000),
            static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE)),
            static_cast<int>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS)));
        if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & APE_FORMAT_FLAG_AIFF)
            strLine += _T(", AIFF");
        strSummary += strLine + _T("\r\n");

        // length
        strLine.Format(_T("Length: %s (%I64d blocks)"),
            FormatDuration(static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_LENGTH_MS)) / 1000.0, FALSE).GetString(),
            static_cast<int64>(spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS)));
        strSummary += strLine + _T("\r\n");

        // the file size
        strLine.Format(_T("APE: %.2f MB"), static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_APE_TOTAL_BYTES)) / static_cast<double>(1048576));
        strSummary += strLine + _T("\r\n");

        strLine.Format(_T("WAV: %.2f MB"), static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_WAV_TOTAL_BYTES)) / static_cast<double>(1048576));
        strSummary += strLine + _T("\r\n");

        // the compression ratio
        strLine.Format(_T("Compression: %.2f%%"), static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_AVERAGE_BITRATE) * 100) / static_cast<double>(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_DECOMPRESSED_BITRATE)));
        strSummary += strLine + _T("\r\n");

        IAPETag * pTag = GET_TAG(spAPEDecompress);
        OutputAPETag(pTag, strSummary);
    }
    else
    {
        APE::WAVEFORMATEX wfeInput; APE_CLEAR(wfeInput); int64 nTotalBlocks = 0; int64 nHeaderBytes = 0; int64 nTerminatingBytes = 0; int32 nFlags = 0; int nErrorCode = 0;
        CSmartPtr<CInputSource> spInputSource(CreateInputSource(strFilename, &wfeInput, &nTotalBlocks, &nHeaderBytes, &nTerminatingBytes, &nFlags, &nErrorCode));

        CString strFormat;
        if (nErrorCode == ERROR_SUCCESS)
        {
            strFormat.Format(_T("%d samples per second\r\n%d channels\r\n%d bits per sample\r\n%I64d header bytes\r\n%I64d terminating bytes\r\n%I64d total blocks"), static_cast<int>(wfeInput.nSamplesPerSec), static_cast<int>(wfeInput.nChannels), static_cast<int>(wfeInput.wBitsPerSample), nHeaderBytes, nTerminatingBytes, nTotalBlocks);
            if (nFlags & APE_FORMAT_FLAG_BIG_ENDIAN)
                strFormat += _T("\r\nBig endian");
            if (nFlags & APE_FORMAT_FLAG_SIGNED_8_BIT)
                strFormat += _T("\r\nSigned 8-bit");
            if (spInputSource->GetFloat())
                strFormat += _T("\r\nFloating point");
        }
        else
        {
            // see if the file has an APE tag (WavPack files do)
            CSmartPtr<CAPETag> spAPETag(new CAPETag(strFilename, true));
            if (spAPETag->GetHasAPETag())
            {
                OutputAPETag(spAPETag, strSummary);
            }
            else
            {
                strFormat = _T("Format information not available for this file (unsupported by Monkey's Audio)");
            }
        }

        strSummary += strFormat;
    }

    strSummary.TrimRight();
    return strSummary;
}

void CAPEInfoFormatDlg::OutputAPETag(APE::IAPETag * pTag, CString & strSummary)
{
    if (pTag != APE_NULL)
    {
        CString strTag = _T("None");
        if (pTag->GetHasAPETag())
        {
            strTag.Format(_T("APE Tag v%.2f"), static_cast<double>(pTag->GetAPETagVersion()) / static_cast<double>(1000));
            if (pTag->GetHasID3Tag())
                strTag += _T(", ID3v1.1");
        }
        else if (pTag->GetHasID3Tag())
        {
            strTag = _T("ID3v1.1");
        }
        CString strLine;
        strLine.Format(_T("Tag: %s (%d bytes)"),
            strTag.GetString(), pTag->GetTagBytes());
        strSummary += strLine + _T("\r\n");

        int nTagIndex = 0; CAPETagField* pTagField = APE_NULL;
        while ((pTagField = pTag->GetTagField(nTagIndex++)) != APE_NULL)
        {
            WCHAR cValue[1024];
            APE_CLEAR(cValue);
            int nValueBytes = 1023;
            pTag->GetFieldString(pTagField->GetFieldName(), &cValue[0], &nValueBytes);

            strLine.Format(_T("    %s: %s"),
                pTagField->GetFieldName(),
                (nValueBytes >= 1024) ? _T("<too large to display>") : cValue);
            strSummary += strLine + _T("\r\n");
        }
    }
}
