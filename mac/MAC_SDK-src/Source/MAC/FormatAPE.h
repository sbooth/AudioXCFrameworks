#pragma once

#include "Format.h"

class CFormatAPE : public IFormat
{
public:
    CFormatAPE(int nIndex);
    virtual ~CFormatAPE();

    virtual bool GetValid() { return true; }
    virtual CString GetName();

    virtual int Process(MAC_FILE * pInfo);

    virtual bool BuildMenu(CMenu * pMenu, int nBaseID);
    virtual bool ProcessMenuCommand(int nCommand);

    virtual CString GetInputExtensions(APE::APE_MODES Mode);
    virtual CString GetOutputExtension(APE::APE_MODES Mode, const CString & strInputFilename, int nLevel);

protected:
    int m_nIndex;
};
