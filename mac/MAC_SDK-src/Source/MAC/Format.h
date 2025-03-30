#pragma once

#define ID_SET_COMPRESSION_FIRST    40000
#define ID_SET_COMPRESSION_LAST     50000

class MAC_FILE;

class IFormat
{
public:
    virtual ~IFormat() { }

    virtual bool GetValid() = 0;

    virtual CString GetName() = 0;

    virtual int Process(MAC_FILE * pInfo) = 0;

    virtual bool BuildMenu(CMenu * pMenu, int nBaseID) = 0;
    virtual bool ProcessMenuCommand(int nCommand) = 0;

    virtual CString GetInputExtensions(APE::APE_MODES Mode) = 0;
    virtual CString GetOutputExtension(APE::APE_MODES Mode, const CString & strInputFilename, int nLevel) = 0;
};
