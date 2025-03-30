#include "stdafx.h"
#include "MACDllApp.h"
#include "WinampSettingsDlg.h"

CMACDllApp g_Application;

BEGIN_MESSAGE_MAP(CMACDllApp, CWinApp)
END_MESSAGE_MAP()

CMACDllApp::CMACDllApp()
{
}

CMACDllApp::~CMACDllApp()
{
}

BOOL CMACDllApp::InitInstance()
{
    CWinApp::InitInstance();
    return true;
}
