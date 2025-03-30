#include "stdafx.h"
#include "MAC.h"
#include "AboutDlg.h"
#include "CPUFeatures.h"

using namespace APE;

CAboutDlg::CAboutDlg(CWnd * pParent)
    : CDialog(CAboutDlg::IDD, pParent)
{
    m_bFontsCreated = false;
}

void CAboutDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_MOVING()
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    if (sizeof(intn) == sizeof(int64))
        SetWindowText(APE_NAME _T(" (64-bit)"));
    else
        SetWindowText(APE_NAME _T(" (32-bit)"));

    return true;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

HBRUSH CAboutDlg::OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor)
{
    HBRUSH hBrush = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    //hBrush = CreateSolidBrush(RGB(255, 255, 255));
    return hBrush;
}

void CAboutDlg::OnPaint()
{
    CPaintDC dc(this);
    int nSavedDC = dc.SaveDC();

    CRect rectPaint; GetClientRect(&rectPaint);
    int nWindowBorder = theApp.GetSize(16, 0).cx;
    rectPaint.DeflateRect(nWindowBorder, nWindowBorder, nWindowBorder, nWindowBorder);

    dc.SetBkMode(TRANSPARENT);

    if (m_bFontsCreated == false)
    {
        LOGFONT LogFont;
        CFont * pfontGUI = CFont::FromHandle(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));

        pfontGUI->GetLogFont(&LogFont);
        LogFont.lfWidth = (LogFont.lfWidth * 180) / 100;
        LogFont.lfWidth = theApp.GetSize(LogFont.lfWidth, 0).cx;
        LogFont.lfHeight = theApp.GetSize(LogFont.lfHeight, 0).cx;
        LogFont.lfHeight = (LogFont.lfHeight * 180) / 100;
        LogFont.lfHeight = theApp.GetSize(LogFont.lfHeight, 0).cx;
        LogFont.lfWeight = FW_BOLD;
        m_fontLarge.CreateFontIndirect(&LogFont);

        pfontGUI->GetLogFont(&LogFont);
        LogFont.lfWidth = (LogFont.lfWidth * 120) / 100;
        LogFont.lfWidth = theApp.GetSize(LogFont.lfWidth, 0).cx;
        LogFont.lfHeight = (LogFont.lfHeight * 120) / 100;
        LogFont.lfHeight = theApp.GetSize(LogFont.lfHeight, 0).cx;
        m_fontSmall.CreateFontIndirect(&LogFont);
    }

    // get CPU
    CString strCPU = GetCPU();

    dc.SelectObject(&m_fontSmall);
    int nFontSmallHeight = dc.GetTextExtent(_T("Ay")).cy;
    dc.SelectObject(&m_fontLarge);
    int nFontLargeHeight = dc.GetTextExtent(_T("Ay")).cy;
    int nLineSpacing = theApp.GetSize(3, 0).cx;

    int nTop = rectPaint.top;

    // size title
    dc.SelectObject(&m_fontLarge);
    CRect rectSizeName;
    dc.DrawText(APE_NAME, &rectSizeName, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    int nWindowWidth = rectSizeName.Width() + theApp.GetSize(64, 0).cx;

    // size copyright
    dc.SelectObject(&m_fontSmall);
    CRect rectSizeCopyright;
    dc.DrawText(APE_RESOURCE_COPYRIGHT, &rectSizeCopyright, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    nWindowWidth = ape_max(nWindowWidth, rectSizeCopyright.Width() + theApp.GetSize(64, 0).cx);

    // size CPU
    CRect rectCPU;
    dc.DrawText(strCPU, &rectCPU, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    nWindowWidth = ape_max(nWindowWidth, rectCPU.Width() + theApp.GetSize(64, 0).cx);

    // paint rectangle
    rectPaint.right = rectPaint.left + nWindowWidth;

    // draw title
    dc.SelectObject(&m_fontLarge);
    dc.DrawText(APE_NAME, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSizeName.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontLargeHeight + nLineSpacing;

    // draw copyright
    dc.SelectObject(&m_fontSmall);
    dc.DrawText(APE_RESOURCE_COPYRIGHT, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSizeCopyright.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    // all rights reserved
    dc.SelectObject(&m_fontSmall);
    CRect rectSize;
    dc.DrawText(_T("All rights reserved."), &rectSize, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    dc.DrawText(_T("All rights reserved."), CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSize.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    // draw thanks to Robert Kausch (he's helped SO MUCH!)
    dc.SelectObject(&m_fontSmall);
    dc.DrawText(_T("Huge thanks to Robert Kausch for all his contributions!"), CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSizeCopyright.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    // CPU
    dc.DrawText(strCPU, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectCPU.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    // scale
    CString strScale; strScale.Format(_T("Scale: %.1f"), theApp.GetScale());
    dc.DrawText(strScale, &rectSize, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    dc.DrawText(strScale, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSize.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    dc.RestoreDC(nSavedDC);

    // size to height on first draw
    if (m_bFontsCreated == false)
    {
        CRect rectWindow; GetWindowRect(rectWindow);
        CRect rectClient; GetClientRect(rectClient);
        SetWindowPos(APE_NULL, 0, 0, nWindowWidth + (rectWindow.Width() - rectClient.Width()) + (2 * nWindowBorder), nTop + nWindowBorder + (rectWindow.Height() - rectClient.Height()) + (0 * nLineSpacing), SWP_NOMOVE);
    }
    m_bFontsCreated = true;
}

void CAboutDlg::OnMoving(UINT, LPRECT pRect)
{
    CapMoveToMonitor(GetSafeHwnd(), pRect);
}

CString CAboutDlg::GetCPU()
{
    CStringArrayEx aryCPU;

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    if (GetSSE2Available() && GetSSE2Supported())
        aryCPU.Add(_T("SSE2: yes"));
    else
        aryCPU.Add(_T("SSE2: no"));

    if (GetSSE41Available() && GetSSE41Supported())
        aryCPU.Add(_T("SSE4.1: yes"));
    else
        aryCPU.Add(_T("SSE4.1: no"));

    if (GetAVX2Available() && GetAVX2Supported())
        aryCPU.Add(_T("AVX2: yes"));
    else
        aryCPU.Add(_T("AVX2: no"));

    if (GetAVX512Available() && GetAVX512Supported())
        aryCPU.Add(_T("AVX-512: yes"));
    else
        aryCPU.Add(_T("AVX-512: no"));
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
    if (GetNeonAvailable() && GetNeonSupported())
        aryCPU.Add(_T("Neon: yes"));
    else
        aryCPU.Add(_T("Neon: no"));
#endif

    if (IsProcessElevated())
        aryCPU.Add(_T("elevated: yes"));
    else
        aryCPU.Add(_T("elevated: no"));

    CString strCPU = aryCPU.GetList(_T("; "));
    return strCPU;
}
