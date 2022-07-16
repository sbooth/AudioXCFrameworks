#pragma once

#include "WindowsEnvironment.h"

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>        // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>            // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <atlbase.h>
#include <process.h>
#include <afxtempl.h>
#include <MMSystem.h>

#include "All.h"
#include "StringArrayEx.h"
#include "IntArray.h"
#include "Filename.h"
#include "SmartPtr.h"
#include "MACLib.h"
#include "MACSettings.h"
#include "CharacterHelper.h"
using namespace APE;

// global functions
void FixDirectory(CString & strDirectory);
CString GetInstallPath();
CString GetProgramPath(BOOL bAppendProgramName = FALSE);
CString GetUserDataPath();
void CreateDirectoryEx(CString strDirectory);
void ListFiles(CStringArray * pStringArray, CString strPath, BOOL bRecurse = FALSE);
BOOL FileExists(const CString & strFilename);
CString GetUniqueFilename(CString strFilename);
double GetFileBytes(const CString & strFilename);
CString GetExtension(LPCTSTR pFilename);
CString GetDirectory(LPCTSTR pFilename);
double GetDriveFreeMB(CString strDrive);
CString FormatDuration(double dSeconds, BOOL bAddDecimal = FALSE);
BOOL MoveFile(const CString & strExistingFilename, const CString & strNewFilename, BOOL bOverwrite);
BOOL CopyFileTime(const CString & strSourceFilename, const CString & strDestinationFilename);
BOOL RecycleFile(const CString & strFilename, BOOL bConfirm = FALSE);
BOOL ReadWholeFile(const CString & strFilename, CString & strBuffer);
BOOL ExecuteProgramBlocking(CString strApplication, CString strParameters, int * pnExitCode = NULL, BOOL bShowPopup = FALSE);
BOOL IsProcessElevated();
BOOL DeleteFileEx(LPCTSTR pFilename);

// global defines
#define cap(VALUE, MIN, MAX) (((VALUE) < (MIN)) ? (MIN) : ((VALUE) > (MAX)) ? (MAX) : (VALUE))

// unicode helpers
#ifdef _UNICODE
    #define INITIALIZE_COMMON_CONTROL(HWND) ::SendMessage(HWND, CCM_SETUNICODEFORMAT, TRUE, 0);
#else
    #define INITIALIZE_COMMON_CONTROL(HWND) ::SendMessage(HWND, CCM_SETUNICODEFORMAT, FALSE, 0);
#endif
