#pragma once

/**************************************************************************************************
Includes
**************************************************************************************************/
#include "Warnings.h"
#include "MFCWarnings.h"
#include "WindowsEnvironment.h"

/**************************************************************************************************
Includes
**************************************************************************************************/
#define VC_EXTRALEAN        // exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <atlbase.h>
#include <process.h>
#include <afxtempl.h>
#include <mmsystem.h>

#include "All.h"
#include "StringArrayEx.h"
#include "IntArray.h"
#include "Filename.h"
#include "SmartPtr.h"
#include "MACLib.h"
#include "MACSettings.h"
#include "CharacterHelper.h"
#include "MFCGlobals.h"

/**************************************************************************************************
Global functions
**************************************************************************************************/
void FixDirectory(CString & strDirectory);
CString GetInstallPath();
CString GetProgramPath(bool bAppendProgramName = false);
CString GetUserDataPath();
void CreateDirectoryEx(CString strDirectory);
void ListFiles(CStringArray * pStringArray, CString strPath, bool bRecurse = false);
bool FileExists(const CString & strFilename);
CString GetUniqueFilename(CString strFilename);
double GetFileBytes(const CString & strFilename);
CString GetExtension(LPCTSTR pFilename);
CString GetDirectory(LPCTSTR pFilename);
double GetDriveFreeMB(CString strDrive);
bool MoveFile(const CString & strExistingFilename, const CString & strNewFilename, bool bOverwrite);
bool CopyFileTime(const CString & strSourceFilename, const CString & strDestinationFilename);
bool RecycleFile(const CString & strFilename, bool bConfirm = false);
bool ReadWholeFile(const CString & strFilename, CString & strBuffer);
bool ExecuteProgramBlocking(CString strApplication, CString strParameters, int * pnExitCode = APE_NULL, bool bShowPopup = false, CString * pstrReturnOutput = APE_NULL);
bool IsProcessElevated();
void DeleteFileEx(LPCTSTR pFilename);
void CapMoveToMonitor(HWND hWnd, LPRECT pRect);

/**************************************************************************************************
Global defines
**************************************************************************************************/
#define cap(VALUE, MIN, MAX) (((VALUE) < (MIN)) ? (MIN) : ((VALUE) > (MAX)) ? (MAX) : (VALUE))

/**************************************************************************************************
Unicode helpers
**************************************************************************************************/
#ifdef _UNICODE
    #define INITIALIZE_COMMON_CONTROL(HWND) ::SendMessage(HWND, CCM_SETUNICODEFORMAT, true, 0);
#else
    #define INITIALIZE_COMMON_CONTROL(HWND) ::SendMessage(HWND, CCM_SETUNICODEFORMAT, false, 0);
#endif
