#pragma once

class CWinampSettingsDlg;

class CMACDllApp : public CWinApp
{
public:
    CMACDllApp();
    virtual ~CMACDllApp();

    virtual BOOL InitInstance();

protected:
    DECLARE_MESSAGE_MAP();
};

extern CMACDllApp g_Application;
