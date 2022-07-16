#pragma once

// environment
#include "WindowsEnvironment.h"

// defines for reducing size
#define VC_EXTRALEAN                              // exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS        // some CString constructors will be explicit

// MFC
#ifdef _MSC_VER
#include <afxwin.h>                                // MFC core and standard components
#include <afxext.h>                                // MFC extensions
#endif

// Monkey's Audio
#include "All.h"
#include "MACLib.h"
using namespace APE;

// resources
#include "resource.h"
