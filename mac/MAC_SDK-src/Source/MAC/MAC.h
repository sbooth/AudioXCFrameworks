#pragma once

#include "resource.h"
class CFormatArray;
class CMACSettings;
class CMACDlg;

class CMACApp : public CWinApp
{
public:
    // construction / destruction
    CMACApp();
    virtual ~CMACApp();

    // initialize
    virtual BOOL InitInstance();

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

    // message map
    DECLARE_MESSAGE_MAP()

private:
    // helper objects
    CSmartPtr<CFormatArray> m_sparyFormats;
    CSmartPtr<CMACSettings> m_spSettings;
    CSmartPtr<CImageList> m_spImageListToolbar;
    CSmartPtr<CImageList> m_spImageListOptionsList;
    CSmartPtr<CImageList> m_spImageListOptionsPages;
    double m_dScale;
    HANDLE m_hSingleInstance;
    bool m_bAnotherInstanceRunning;
    CMACDlg * m_pMACDlg;
};

extern CMACApp theApp;
