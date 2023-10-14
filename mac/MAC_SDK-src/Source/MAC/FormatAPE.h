#pragma once

#include "Format.h"

class CFormatAPE : public IFormat
{
public:
    CFormatAPE(int nIndex);
    virtual ~CFormatAPE();

    virtual CString GetName();

    virtual int Process(MAC_FILE * pInfo);

    virtual BOOL BuildMenu(CMenu * pMenu, int nBaseID);
    virtual BOOL ProcessMenuCommand(int nCommand);

    virtual CString GetInputExtensions(APE::APE_MODES Mode);
    virtual CString GetOutputExtension(APE::APE_MODES Mode, const CString & strInputFilename, int nLevel);

protected:
    int m_nIndex;
};
