#include "stdafx.h"
#include "MAC.h"
#include "APEInfoFormatDlg.h"
#include "APETag.h"
#include "WAVInputSource.h"
#include "MACDlg.h"

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
    SendMessageToDescendants(WM_SETFONT, (WPARAM) m_pMACDlg->GetFont().GetSafeHandle(), MAKELPARAM(FALSE, 0), TRUE);

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
    m_ctrlFormat.SetWindowPos(NULL, theApp.GetSize(nBorder, 0).cx, theApp.GetSize(nBorder, 0).cx, rectWindow.Width() - theApp.GetSize(nBorder * 2, 0).cx, rectWindow.Height() - theApp.GetSize(nBorder * 2, 0).cx, SWP_NOZORDER);
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
    if ((spAPEDecompress != NULL) && (nFunctionRetVal == ERROR_SUCCESS))
    {
        CString strLine;

        strLine.Format(_T("Monkey's Audio %.2f (%s)"),
            double(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FILE_VERSION)) / double(1000),
            (LPCTSTR) theApp.GetSettings()->GetAPECompressionName(int(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_COMPRESSION_LEVEL))));
        strSummary += strLine + _T("\r\n");
        
        strLine.Format(_T("Format: %.1f khz, %d bit, %d ch"),
            double(spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_SAMPLE_RATE)) / double(1000),
            spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_BITS_PER_SAMPLE),
            spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_CHANNELS));
        if (spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_FORMAT_FLAGS) & MAC_FORMAT_FLAG_AIFF)
            strLine += _T(", AIFF");
        strSummary += strLine + _T("\r\n");

        strLine.Format(_T("Length: %s (%d blocks)"),
            (LPCTSTR) FormatDuration(double(spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_LENGTH_MS)) / 1000.0, FALSE),
            spAPEDecompress->GetInfo(IAPEDecompress::APE_DECOMPRESS_TOTAL_BLOCKS));
        strSummary += strLine + _T("\r\n");

        CAPETag * pTag = (CAPETag *) spAPEDecompress->GetInfo(IAPEDecompress::APE_INFO_TAG);
        if (pTag)
        {
            CString strTag = _T("None");
            if (pTag->GetHasAPETag())
                strTag.Format(_T("APE Tag v%.2f"), double(pTag->GetAPETagVersion()) / double(1000));
            else if (pTag->GetHasID3Tag())
                strTag = _T("ID3v1.1");
            strLine.Format(_T("Tag: %s (%d bytes)"),
                (LPCTSTR) strTag, pTag->GetTagBytes());
            strSummary += strLine + _T("\r\n");

            int nTagIndex = 0; CAPETagField * pTagField = NULL;
            while ((pTagField = pTag->GetTagField(nTagIndex++)) != NULL)
            {
                WCHAR cValue[1024] = { 0 }; int nValueBytes = 1023;
                pTag->GetFieldString(pTagField->GetFieldName(), &cValue[0], &nValueBytes);

                strLine.Format(_T("    %s: %s"),
                    pTagField->GetFieldName(),
                    (nValueBytes >= 1024) ? _T("<too large to display>") : cValue);
                strSummary += strLine + _T("\r\n");
            }
        }
    }
    else
    {
        APE::WAVEFORMATEX wfeInput; int64 nTotalBlocks = 0; int64 nHeaderBytes = 0; int64 nTerminatingBytes = 0; int nErrorCode = 0;
        APE::CWAVInputSource WAV(strFilename, &wfeInput, &nTotalBlocks, &nHeaderBytes, &nTerminatingBytes, &nErrorCode);
        
        CString strFormat;
        strFormat.Format(_T("%d samples per second\r\n%d channels\r\n%d bits per sample\r\n%d header bytes\r\n%d terminating bytes\r\n%I64d total blocks"), wfeInput.nSamplesPerSec, wfeInput.nChannels, wfeInput.wBitsPerSample, nHeaderBytes, nTerminatingBytes, nTotalBlocks);

        strSummary += strFormat;
    }

    strSummary.TrimRight();
    return strSummary;
}