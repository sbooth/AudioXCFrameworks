// MAC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MAC.h"
#include "MACDlg.h"
#include "FormatArray.h"
#include "MACSettings.h"
#include "APEButtons.h"
#include "../Shared/CGdiPlusBitmap.h"

CMACApp theApp;

BEGIN_MESSAGE_MAP(CMACApp, CWinApp)
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

CMACApp::CMACApp()
{
    m_dScale = -1.0; // default to an impossible value so the first SetScale(...) call returns that it changed
    m_hSingleInstance = CreateMutex(NULL, FALSE, _T("Mokey's Audio"));
    DWORD dwLastError = GetLastError();
    m_bAnotherInstanceRunning = (dwLastError == ERROR_ALREADY_EXISTS);
    m_pMACDlg = NULL;
}

CMACApp::~CMACApp()
{
    if (m_hSingleInstance != NULL)
    {
        CloseHandle(m_hSingleInstance);
        m_hSingleInstance = NULL;
    }
    m_sparyFormats.Delete();
    m_spSettings.Delete();
}

BOOL CMACApp::InitInstance()
{
    // don't run if we're already running
    if (m_bAnotherInstanceRunning)
    {
        HWND hwndMonkeysAudio = FindWindow(NULL, MAC_NAME);
        if (hwndMonkeysAudio != NULL)
        {
            ShowWindow(hwndMonkeysAudio, SW_RESTORE);
            SetForegroundWindow(hwndMonkeysAudio);
        }
        return FALSE;
    }

    // initialize
    AfxEnableControlContainer();
    AfxOleInit();
    InitCommonControls();
    AfxInitRichEdit();

    // parse command line
    CString strCommandLine = GetCommandLine();
    strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - (strCommandLine.Find(_T(".exe")) + 4));
    strCommandLine.TrimLeft(_T(" \"")); strCommandLine.TrimRight(_T(" \""));

    // uninstall if specified
    if (strCommandLine.CompareNoCase(_T("-uninstall")) == 0)
    {
        TCHAR cUninstall[MAX_PATH] = { 0 };
        _tcscat_s(cUninstall, MAX_PATH, GetInstallPath());
        _tcscat_s(cUninstall, MAX_PATH, _T("uninstall.exe"));

        ShellExecute(NULL, NULL, cUninstall, NULL, NULL, SW_SHOW);

        return FALSE;
    }
    
    // show program
    CMACDlg dlg;
    m_pMACDlg = &dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK)
    {
    }
    else if (nResponse == IDCANCEL)
    {
    }
    m_pMACDlg = NULL;

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}

CFormatArray * CMACApp::GetFormatArray()
{
    if (m_sparyFormats == NULL)
        m_sparyFormats.Assign(new CFormatArray(m_pMACDlg));
    return m_sparyFormats.GetPtr();
}

CMACSettings * CMACApp::GetSettings()
{
    if (m_spSettings == NULL)
        m_spSettings.Assign(new CMACSettings);
    return m_spSettings.GetPtr();
}

void CMACApp::DeleteImageLists()
{
    m_spImageListToolbar.Delete();
    m_spImageListOptionsList.Delete();
    m_spImageListOptionsPages.Delete();
}

CImageList * CMACApp::GetImageList(EImageList Image)
{
    CSmartPtr<CImageList> & rpImage = (Image == Image_Toolbar) ? m_spImageListToolbar : (Image == Image_OptionsList) ? m_spImageListOptionsList : m_spImageListOptionsPages;
    if (rpImage == NULL)
    {
        // start GDI plus
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        // load the image
        CString strImage = GetProgramPath() + _T("APE_Buttons.png"); 
        if (FileExists(strImage) == false)
            strImage = GetInstallPath() + _T("APE_Buttons.png");
        CSmartPtr<Gdiplus::Bitmap> spBitmap;
        spBitmap.Assign(Gdiplus::Bitmap::FromFile(strImage));
        if ((spBitmap == NULL) || (spBitmap->GetLastStatus() != Gdiplus::Ok))
            return NULL;

        // build the image list
        rpImage.Assign(new CImageList);
        
        CSize sizeImageList = GetSize(32, 32);
        rpImage->Create(sizeImageList.cx, sizeImageList.cy, ILC_COLOR32, TBB_COUNT + 1, 10);

        // setup drawing
        CSize sizeBitmap = theApp.GetSize(32 * (spBitmap->GetWidth() / spBitmap->GetHeight()), 32);
        Gdiplus::Bitmap newBmp(sizeBitmap.cx, sizeBitmap.cy, PixelFormat32bppPARGB);
        Gdiplus::Graphics graphics(&newBmp);
        graphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        // fill with the background color of the toolbar
        if ((Image == Image_Toolbar) || (Image == Image_OptionsPages))
        {
            Gdiplus::SolidBrush b(Gdiplus::Color(255, 240, 240, 240));
            graphics.FillRectangle(&b, 0, 0, TBB_COUNT * sizeImageList.cx, sizeImageList.cy);
        }
        else if (Image == Image_OptionsList)
        {
            Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255));
            graphics.FillRectangle(&b, 0, 0, TBB_COUNT * sizeImageList.cx, sizeImageList.cy);
        }
        
        // draw the image over the top
        int nSizeBitmap = spBitmap->GetHeight();
        for (int z = 0; z < TBB_COUNT; z++)
        {
            // copy the individual image to the temporary buffer
            // we do this because there were artifacts drawing from an image that had all the images next to each other
            // the artifacts were found 12/3/2021 and was a line next to the Make APL icon
            CSmartPtr<Gdiplus::Bitmap> spTemporary(spBitmap->Clone(nSizeBitmap * z, 0, nSizeBitmap, nSizeBitmap, PixelFormat32bppPARGB));
            
            // draw onto the graphics object
            Gdiplus::Rect rectSource(0, 0, nSizeBitmap, nSizeBitmap);
            Gdiplus::Rect rectDestination(z * sizeImageList.cy, 0, sizeImageList.cx, sizeImageList.cy);
            graphics.DrawImage(spTemporary, rectDestination, rectSource.X, rectSource.Y, rectSource.Width, rectSource.Height, Gdiplus::UnitPixel);
        }

        // get the bitmap
        HBITMAP hBitmap;
        newBmp.GetHBITMAP(Gdiplus::Color::Transparent, &hBitmap);

        // add to the image list
        ImageList_AddMasked(rpImage->GetSafeHandle(), hBitmap, 0);
    }

    return rpImage;
}

CSize CMACApp::GetSize(int x, int y, double dAdditional)
{
    CSize sizeFinished(x, y);
    sizeFinished.cx = int(m_dScale * dAdditional * double(sizeFinished.cx));
    sizeFinished.cy = int(m_dScale * dAdditional * double(sizeFinished.cy));
    return sizeFinished;
}

int CMACApp::GetSizeReverse(int nSize)
{
    int nNewSize = int(double(nSize) / m_dScale);
    return nNewSize;
}

bool CMACApp::SetScale(double dScale)
{
    bool bResult = false;
    if (fabs(dScale - m_dScale) > 0.01)
    {
        m_dScale = dScale;
        bResult = true;
    }
    return bResult;
}