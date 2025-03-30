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

    if (pInfo->m_Mode == MODE_COMPRESS)
    {
        nRetVal = CompressFileW(pInfo->m_strInputFilename, pInfo->m_strWorkingFilename, pInfo->m_nLevel, &pInfo->m_nStageProgress, APE_NULL, &pInfo->m_nKillFlag, pInfo->m_nThreads);
    }
    else if (pInfo->m_Mode == MODE_DECOMPRESS)
    {
        nRetVal = DecompressFileW(pInfo->m_strInputFilename, pInfo->m_strWorkingFilename, &pInfo->m_nStageProgress, APE_NULL, &pInfo->m_nKillFlag, pInfo->m_nThreads);
    }
    else if (pInfo->m_Mode == MODE_VERIFY)
    {
        nRetVal = VerifyFileW(pInfo->m_strInputFilename, &pInfo->m_nStageProgress, APE_NULL, &pInfo->m_nKillFlag,
            (theApp.GetSettings()->m_nProcessingVerifyMode == PROCESSING_VERIFY_MODE_QUICK) ? true : false, pInfo->m_nThreads);
    }
    else if (pInfo->m_Mode == MODE_CONVERT)
    {
        nRetVal = ConvertFileW(pInfo->m_strInputFilename, pInfo->m_strWorkingFilename, pInfo->m_nLevel,
            &pInfo->m_nStageProgress, APE_NULL, &pInfo->m_nKillFlag, pInfo->m_nThreads);
    }

    return nRetVal;
}

bool CFormatAPE::BuildMenu(CMenu * pMenu, int nBaseID)
{
    bool bCheck = (theApp.GetSettings()->GetFormat() == COMPRESSION_APE);

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

    return true;
}

bool CFormatAPE::ProcessMenuCommand(int nCommand)
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

    return true;
}

CString CFormatAPE::GetInputExtensions(APE_MODES Mode)
{
    CString strRetVal;

    if (Mode == MODE_COMPRESS)
    {
        strRetVal = _T(".wav;.aiff;.aif;.w64;.snd;.au;.caf;.rf64;.bw64");
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
