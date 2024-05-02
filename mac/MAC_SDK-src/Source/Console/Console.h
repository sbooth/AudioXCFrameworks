#pragma once

#include "All.h"

// functions
int Tag(const APE::str_utfn * pFilename, const APE::str_utfn * pTagString);
APE::str_utfn * GetParameterFromList(const APE::str_utfn * pList, const APE::str_utfn * pDelimiter, int nCount);
int RemoveTags(const APE::str_utfn * pFilename);
int ConvertTagsToLegacy(const APE::str_utfn * pFilename);
void HandleAuto(APE::CSmartPtr<wchar_t> & spInputFilename, APE::CSmartPtr<wchar_t> & spOutputFilename, bool bAPEType);

#ifndef PLATFORM_WINDOWS
int main(int argc, char * argv[]);
#else
int _tmain(int argc, TCHAR * argv[]);
#endif

// globals
extern TICK_COUNT_TYPE g_nInitialTickCount;
