#pragma once

#include "resource.h"
class CFormatArray;
class CMACSettings;
class CMACDlg;

#pragma warning(push)
// Clang warns on this in Win32 / Release about using memory after it is freed without the NOLINT marker
#include <gdiplus.h> // NOLINT
#pragma warning(pop)

class CMACApp : public CWinApp
{
public:
    // construction / destruction
    CMACApp();
    ~CMACApp();

    // initialize
    virtual BOOL InitInstance();
    virtual int ExitInstance();

    // data access
    CFormatArray * GetFormatArray();
    CMACSettings * GetSettings();
    enum EImageList
    {
        Image_Toolbar,
        Image_OptionsList,
        Image_OptionsPages
    };
    void DeleteImageLists();
    CImageList * GetImageList(EImageList Image);
    CSize GetSize(int x, int y, double dAdditional = 1.0);
    int GetSizeReverse(int nSize);
    double GetScale() { return m_dScale; }
    bool SetScale(double dScale);
    Gdiplus::Bitmap * GetMonkeyImage();

    // message map
    DECLARE_MESSAGE_MAP()

private:
    // helper objects
    APE::CSmartPtr<CFormatArray> m_sparyFormats;
    APE::CSmartPtr<CMACSettings> m_spSettings;
    CImageList m_ImageListToolbar;
    CImageList m_ImageListOptionsList;
    CImageList m_ImageListOptionsPages;
    APE::CSmartPtr<Gdiplus::Bitmap> m_spButtons;
    APE::CSmartPtr<Gdiplus::Bitmap> m_spMonkey;
    double m_dScale;
    HANDLE m_hSingleInstance;
    bool m_bAnotherInstanceRunning;
    CMACDlg * m_pMACDlg;
};

extern CMACApp theApp;
