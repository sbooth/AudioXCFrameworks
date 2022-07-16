#include "stdafx.h"
#include "MAC.h"
#include "AboutDlg.h"
#include "GlobalFunctions.h"

CAboutDlg::CAboutDlg(CWnd * pParent)
    : CDialog(CAboutDlg::IDD, pParent)
{
    m_bFontsCreated = false;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    if (sizeof(intn) == 8)
        SetWindowText(MAC_NAME _T(" (64-bit)"));
    else
        SetWindowText(MAC_NAME _T(" (32-bit)"));

    return TRUE;  // return TRUE unless you set the focus to a control
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
        CFont * pfontGUI = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));

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

    dc.SelectObject(&m_fontSmall);
    int nFontSmallHeight = dc.GetTextExtent(_T("Ay")).cy;
    dc.SelectObject(&m_fontLarge);
    int nFontLargeHeight = dc.GetTextExtent(_T("Ay")).cy;
    int nLineSpacing = theApp.GetSize(3, 0).cx;

    int nTop = rectPaint.top;

    // size title
    dc.SelectObject(&m_fontLarge);
    CRect rectSizeName;
    dc.DrawText(MAC_NAME, &rectSizeName, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    int nWindowWidth = rectSizeName.Width() + theApp.GetSize(64, 0).cx;
    
    // size copyright
    dc.SelectObject(&m_fontSmall);
    CRect rectSizeCopyright;
    dc.DrawText(MAC_RESOURCE_COPYRIGHT, &rectSizeCopyright, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    nWindowWidth = max(nWindowWidth, rectSizeCopyright.Width() + theApp.GetSize(64, 0).cx);
    rectPaint.right = rectPaint.left + nWindowWidth;

    // draw title
    dc.SelectObject(&m_fontLarge);
    dc.DrawText(MAC_NAME, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSizeName.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontLargeHeight + nLineSpacing;

    // draw copyright
    dc.SelectObject(&m_fontSmall);
    dc.DrawText(MAC_RESOURCE_COPYRIGHT, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSizeCopyright.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    dc.SelectObject(&m_fontSmall);
    CRect rectSize;
    dc.DrawText(_T("All rights reserved."), &rectSize, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    dc.DrawText(_T("All rights reserved."), CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSize.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;

    CStringArrayEx aryCPU;
    #ifdef ENABLE_SSE_ASSEMBLY
        if (GetSSEAvailable(false))
            aryCPU.Add(_T("SSE: yes"));
        else
            aryCPU.Add(_T("SSE: no"));
    #else
        aryCPU.Add(_T("SSE: no"));
    #endif

    #ifdef ENABLE_AVX_ASSEMBLY
        if (GetAVX2Available())
            aryCPU.Add(_T("AVX2: yes"));
        else
            aryCPU.Add(("AVX2: no"));
    #else
        aryCPU.Add(_T("AVX2: no"));
    #endif

    if (IsProcessElevated())
        aryCPU.Add(_T("elevated: yes"));
    else
        aryCPU.Add(_T("elevated: no"));

    CString strCPU = aryCPU.GetList(_T("; "));
    dc.DrawText(strCPU, &rectSize, DT_CENTER | DT_NOPREFIX | DT_CALCRECT);
    dc.DrawText(strCPU, CRect(rectPaint.left, nTop, rectPaint.right, nTop + rectSize.Height()), DT_CENTER | DT_NOPREFIX);
    nTop += nFontSmallHeight + nLineSpacing;
    
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
        SetWindowPos(NULL, 0, 0, nWindowWidth + (rectWindow.Width() - rectClient.Width()) + (2 * nWindowBorder), nTop + nWindowBorder + (rectWindow.Height() - rectClient.Height()) + (0 * nLineSpacing), SWP_NOMOVE);
    }
    m_bFontsCreated = true;
}
