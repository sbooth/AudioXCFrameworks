#include "stdafx.h"
#include "MAC.h"
#include "FormatAPE.h"
#include "MACFile.h"
#include "APEInfo.h"

using namespace APE;

CFormatAPE::CFormatAPE(int nIndex)
{
    m_nIndex = nIndex;
}

CFormatAPE::~CFormatAPE()
{

}

CString CFormatAPE::GetName()
{
    return COMPRESSION_APE;
}

int CFormatAPE::Process(MAC_FILE * pInfo)
{
    int nRetVal = ERROR_UNDEFINED;

    if (pInfo->Mode == MODE_COMPRESS)
    {
        nRetVal = CompressFileW(pInfo->strInputFilename, pInfo->strWorkingFilename, pInfo->nLevel, &pInfo->nStageProgress, NULL, &pInfo->nKillFlag);
    }
    else if (pInfo->Mode == MODE_DECOMPRESS)
    {
        nRetVal = DecompressFileW(pInfo->strInputFilename, pInfo->strWorkingFilename, &pInfo->nStageProgress, NULL, &pInfo->nKillFlag);
    }
    else if (pInfo->Mode == MODE_VERIFY)
    {
        nRetVal = VerifyFileW(pInfo->strInputFilename, &pInfo->nStageProgress, NULL, &pInfo->nKillFlag,
            (theApp.GetSettings()->m_nProcessingVerifyMode == PROCESSING_VERIFY_MODE_QUICK) ? TRUE : FALSE);
    }
    else if (pInfo->Mode == MODE_CONVERT)
    {
        nRetVal = ConvertFileW(pInfo->strInputFilename, pInfo->strWorkingFilename, pInfo->nLevel,
            &pInfo->nStageProgress, NULL, &pInfo->nKillFlag);
    }

    return nRetVal;
}

BOOL CFormatAPE::BuildMenu(CMenu * pMenu, int nBaseID)
{
    BOOL bCheck = (theApp.GetSettings()->GetFormat() == COMPRESSION_APE);

    pMenu->AppendMenu(MF_STRING |
        ((bCheck && (theApp.GetSettings()->GetLevel() == APE_COMPRESSION_LEVEL_FAST)) ? MF_CHECKED : MF_UNCHECKED),
        UINT_PTR(nBaseID) + 0, _T("&Fast"));
    pMenu->AppendMenu(MF_STRING |
        ((bCheck && (theApp.GetSettings()->GetLevel() == APE_COMPRESSION_LEVEL_NORMAL)) ? MF_CHECKED : MF_UNCHECKED),
        UINT_PTR(nBaseID) + 1, _T("&Normal"));
    pMenu->AppendMenu(MF_STRING |
        ((bCheck && (theApp.GetSettings()->GetLevel() == APE_COMPRESSION_LEVEL_HIGH)) ? MF_CHECKED : MF_UNCHECKED),
        UINT_PTR(nBaseID) + 2, _T("&High"));
    pMenu->AppendMenu(MF_STRING |
        ((bCheck && (theApp.GetSettings()->GetLevel() == APE_COMPRESSION_LEVEL_EXTRA_HIGH)) ? MF_CHECKED : MF_UNCHECKED),
        UINT_PTR(nBaseID) + 3, _T("&Extra High"));
    pMenu->AppendMenu(MF_STRING |
        ((bCheck && (theApp.GetSettings()->GetLevel() == APE_COMPRESSION_LEVEL_INSANE)) ? MF_CHECKED : MF_UNCHECKED),
        UINT_PTR(nBaseID) + 4, _T("&Insane"));

    return TRUE;
}

BOOL CFormatAPE::ProcessMenuCommand(int nCommand)
{
    int nCompressionLevel = APE_COMPRESSION_LEVEL_NORMAL;
    switch (nCommand)
    {
        case 0: nCompressionLevel = APE_COMPRESSION_LEVEL_FAST; break;
        case 1: nCompressionLevel = APE_COMPRESSION_LEVEL_NORMAL; break;
        case 2: nCompressionLevel = APE_COMPRESSION_LEVEL_HIGH; break;
        case 3: nCompressionLevel = APE_COMPRESSION_LEVEL_EXTRA_HIGH; break;
        case 4: nCompressionLevel = APE_COMPRESSION_LEVEL_INSANE; break;
    }
    theApp.GetSettings()->SetCompression(GetName(), nCompressionLevel);

    return TRUE;
}

CString CFormatAPE::GetInputExtensions(APE_MODES Mode)
{
    CString strRetVal;

    if (Mode == MODE_COMPRESS)
    {
        strRetVal = _T(".wav;.aiff;.aif;.w64;.snd;.au;.caf");
    }
    else if ((Mode == MODE_DECOMPRESS) || (Mode == MODE_VERIFY))
    {
        strRetVal = _T(".ape;.apl;.mac");
    }
    else if (Mode == MODE_CONVERT)
    {
        strRetVal = _T(".ape;.apl;.mac");
    }
    else if (Mode == MODE_MAKE_APL)
    {
        strRetVal = _T(".cue");
    }

    return strRetVal;
}

CString CFormatAPE::GetOutputExtension(APE_MODES Mode, const CString & strInputFilename, int)
{
    CString strExtension;

    if ((Mode == MODE_COMPRESS) || (Mode == MODE_CONVERT))
    {
        strExtension = _T(".ape");
    }
    else if (Mode == MODE_DECOMPRESS)
    {
        APE::str_ansi cFileType[8] = { 0 };
        GetAPEFileType(strInputFilename, cFileType);
        strExtension = cFileType;
    }
    else if (Mode == MODE_MAKE_APL)
    {
        strExtension = _T(".apl");
    }

    return strExtension;
}
