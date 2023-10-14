#pragma once

// basic CRT definitions
#ifdef PLATFORM_WINDOWS
#include <corecrt.h>
#endif

// environment
#include "Warnings.h"
#include "MFCWarnings.h"
#include "WindowsEnvironment.h"

// defines for reducing size
#define VC_EXTRALEAN                              // exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS        // some CString constructors will be explicit

// MFC
#ifdef _MSC_VER
#pragma warning(push) // push and pop warnings because the MFC includes suppresses some
#include <afxwin.h>                                // MFC core and standard components
#include <afxext.h>                                // MFC extensions
#pragma warning(pop)
#endif

// Monkey's Audio
#include "All.h"
#include "MACLib.h"

// MFC globals
#ifdef _MSC_VER
#include "MFCGlobals.h"
#endif

// resources
#include "resource.h"
