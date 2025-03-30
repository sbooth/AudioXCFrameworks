// MAC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MAC.h"
#include "MACDlg.h"
#include "FormatArray.h"
#include "MACSettings.h"
#include "APEButtons.h"

using namespace APE;

CMACApp theApp;

BEGIN_MESSAGE_MAP(CMACApp, CWinApp)
    ON_COMMAND(ID_HELP, &CMACApp::OnHelp)
END_MESSAGE_MAP()

CMACApp::CMACApp()
{
    // initialize
    m_dScale = -1.0; // default to an impossible value so the first SetScale(...) call returns that it changed
    m_hSingleInstance = CreateMutex(APE_NULL, false, _T("Mokey's Audio"));
    DWORD dwLastError = GetLastError();
    m_bAnotherInstanceRunning = (dwLastError == ERROR_ALREADY_EXISTS);
    m_pMACDlg = APE_NULL;

    // start GDI plus
    Gdiplus::GdiplusStartupInput gdiplusStartupInput; // filled in by the constructor
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, APE_NULL);
}

CMACApp::~CMACApp()
{
    // everything should have gotten cleaned up in ExitInstance
}

BOOL CMACApp::InitInstance()
{
    CWinApp::InitInstance();

    // don't run if we're already running
    if (m_bAnotherInstanceRunning)
    {
        HWND hwndMonkeysAudio = FindWindow(APE_NULL, APE_NAME);
        if (hwndMonkeysAudio != APE_NULL)
        {
            ShowWindow(hwndMonkeysAudio, SW_RESTORE);
            SetForegroundWindow(hwndMonkeysAudio);
        }
        return false;
    }

    // initialize
    AfxEnableControlContainer();
    AfxOleInit();
    InitCommonControls();
    AfxInitRichEdit();

    // parse command line
    CString strCommandLine = GetCommandLine();
    //AfxMessageBox(strCommandLine);
    strCommandLine = strCommandLine.Right(strCommandLine.GetLength() - (strCommandLine.Find(_T(".exe")) + 4));
    strCommandLine.TrimLeft(_T(" \"")); strCommandLine.TrimRight(_T(" \""));

    // files
    CStringArrayEx aryFiles;

    // uninstall if specified
    if (strCommandLine.CompareNoCase(_T("-uninstall")) == 0)
    {
        // launch the uninstall (this way we show as the icon for the uninstall instead of a non-descript one)
        TCHAR * pUninstall = new TCHAR [APE_MAX_PATH];
        pUninstall[0] = 0;
        _tcscat_s(pUninstall, APE_MAX_PATH, GetInstallPath());
        _tcscat_s(pUninstall, APE_MAX_PATH, _T("uninstall.exe"));

        ShellExecute(APE_NULL, APE_NULL, pUninstall, APE_NULL, APE_NULL, SW_SHOW);

        APE_SAFE_ARRAY_DELETE(pUninstall)

        return false;
    }
    else
    {
        // assume we got a list of files
        aryFiles.InitFromList(strCommandLine, _T("|"));
    }

    // show program
    CMACDlg dlg(&aryFiles);
    m_pMACDlg = &dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK)
    {
    }
    else if (nResponse == IDCANCEL)
    {
    }
    m_pMACDlg = APE_NULL;

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return false;
}

int CMACApp::ExitInstance()
{
    // delete images
    m_spbmpButtons.Delete();
    m_spbmpMonkey.Delete();

    // close single instance handle
    if (m_hSingleInstance != APE_NULL)
    {
        CloseHandle(m_hSingleInstance);
        m_hSingleInstance = APE_NULL;
    }

    // delete helpers
    m_sparyFormats.Delete();
    m_spSettings.Delete();

    return CWinApp::ExitInstance();
}

CFormatArray * CMACApp::GetFormatArray()
{
    if (m_sparyFormats == APE_NULL)
        m_sparyFormats.Assign(new CFormatArray(m_pMACDlg));
    return m_sparyFormats.GetPtr();
}

CMACSettings * CMACApp::GetSettings()
{
    if (m_spSettings == APE_NULL)
        m_spSettings.Assign(new CMACSettings);
    return m_spSettings.GetPtr();
}

void CMACApp::DeleteImageLists()
{
    m_ImageListToolbar.DeleteImageList();
    m_ImageListOptionsList.DeleteImageList();
    m_ImageListOptionsPages.DeleteImageList();
}

CImageList * CMACApp::GetImageList(EImageList Image)
{
    CImageList * pImageList = (Image == Image_Toolbar) ? &m_ImageListToolbar : (Image == Image_OptionsList) ? &m_ImageListOptionsList : (Image == Image_OptionsPages) ? &m_ImageListOptionsPages : APE_NULL;
    if ((pImageList != APE_NULL) && (pImageList->GetSafeHandle() == APE_NULL))
    {
        // load the image
        if (m_spbmpButtons == APE_NULL)
        {
            CString strImage = GetProgramPath() + _T("APE_Buttons.png");
            if (FileExists(strImage) == false)
                strImage = GetInstallPath() + _T("APE_Buttons.png");
            m_spbmpButtons.Assign(Gdiplus::Bitmap::FromFile(strImage));
        }
        if ((m_spbmpButtons == APE_NULL) || (m_spbmpButtons->GetLastStatus() != Gdiplus::Ok))
            return APE_NULL;

        // build the image list
        CSize sizeImageList = GetSize(32, 32);
        pImageList->Create(sizeImageList.cx, sizeImageList.cy, ILC_COLOR32, TBB_COUNT + 1, 10);

        // setup drawing
        CSize sizeBitmap = theApp.GetSize(32 * static_cast<int>(m_spbmpButtons->GetWidth() / m_spbmpButtons->GetHeight()), 32);
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
        int nSizeBitmap = static_cast<int>(m_spbmpButtons->GetHeight());
        Gdiplus::Bitmap bmpSingle(nSizeBitmap, nSizeBitmap, PixelFormat32bppPARGB);
        Gdiplus::Graphics graphicsSingle(&bmpSingle);
        for (int z = 0; z < TBB_COUNT; z++)
        {
            // copy the individual image to the single buffer
            // we do this because there were artifacts drawing from an image that had all the images next to each other
            // the artifacts were found 12/3/2021 and was a line next to the Make APL icon
            graphicsSingle.Clear(Gdiplus::Color(0));
            graphicsSingle.DrawImage(m_spbmpButtons, Gdiplus::Rect(0, 0, nSizeBitmap, nSizeBitmap), (nSizeBitmap * z), 0, nSizeBitmap, nSizeBitmap, Gdiplus::UnitPixel);

            // draw onto the graphics object
            Gdiplus::Rect rectSource(0, 0, nSizeBitmap, nSizeBitmap);
            Gdiplus::Rect rectDestination(z * sizeImageList.cy, 0, sizeImageList.cx, sizeImageList.cy);
            graphics.DrawImage(&bmpSingle, rectDestination, rectSource.X, rectSource.Y, rectSource.Width, rectSource.Height, Gdiplus::UnitPixel);
        }

        // get the bitmap
        HBITMAP hBitmap;
        newBmp.GetHBITMAP(Gdiplus::Color::Transparent, &hBitmap);

        // add to the image list
        ImageList_AddMasked(pImageList->GetSafeHandle(), hBitmap, 0);
    }

    return pImageList;
}

CSize CMACApp::GetSize(int x, int y, double dAdditional) const
{
    CSize sizeFinished(x, y);
    sizeFinished.cx = static_cast<int>(m_dScale * dAdditional * static_cast<double>(sizeFinished.cx));
    sizeFinished.cy = static_cast<int>(m_dScale * dAdditional * static_cast<double>(sizeFinished.cy));
    return sizeFinished;
}

int CMACApp::GetSizeReverse(int nSize) const
{
    int nNewSize = static_cast<int>(static_cast<double>(nSize) / m_dScale);
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

Gdiplus::Bitmap * CMACApp::GetMonkeyImage()
{
    if (m_spbmpMonkey == APE_NULL)
    {
        // load image
        CString strImage = GetProgramPath() + _T("Monkey.png");
        if (FileExists(strImage) == false)
            strImage = GetInstallPath() + _T("Monkey.png");
        m_spbmpMonkey.Assign(Gdiplus::Bitmap::FromFile(strImage));

        // lock bits and scale alpha
        Gdiplus::BitmapData bitmapData;
        Gdiplus::Rect rectLock(0, 0, static_cast<INT>(m_spbmpMonkey->GetWidth()), static_cast<INT>(m_spbmpMonkey->GetHeight()));
        m_spbmpMonkey->LockBits(&rectLock, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);
        uint32 * pPixel = static_cast<uint32 *>(bitmapData.Scan0);
        for (int nPixel = 0; nPixel < int(bitmapData.Width * bitmapData.Height); nPixel++)
        {
            uint32 nAlpha = pPixel[nPixel] >> 24;
            nAlpha /= 8; // scale alpha down so the image is faded
            pPixel[nPixel] = (nAlpha << 24) | (pPixel[nPixel] & 0x00FFFFFF);
        }
        m_spbmpMonkey->UnlockBits(&bitmapData);
    }
    return m_spbmpMonkey;
}
